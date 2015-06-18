#include "audiocontroller.hpp"
#include "visualizer.hpp"
#include "audioformat.hpp"
#include "audiomixer.hpp"
#include "audioscaler.hpp"
#include "audioanalyzer.hpp"
#include "audioconverter.hpp"
#include "audioresampler.hpp"
#include "audioequalizer.hpp"
#include "player/mpv_helper.hpp"
#include "enum/channellayout.hpp"
#include "misc/log.hpp"
#include "misc/speedmeasure.hpp"
extern "C" {
#include <audio/filter/af.h>
}

DECLARE_LOG_CONTEXT(Audio)

af_info create_info();
af_info af_info_dummy = create_info();

struct bomi_af_priv {
    AudioController *ac;
    char *address;
    int use_scaler, layout, use_normalizer;
};

static auto priv(af_instance *af) -> AudioController*
    { return static_cast<bomi_af_priv*>(af->priv)->ac; }

static auto isSupported(int type) -> bool
{
    switch (type) {
    case AF_FORMAT_S16:     case AF_FORMAT_S16P:
    case AF_FORMAT_S32:     case AF_FORMAT_S32P:
    case AF_FORMAT_FLOAT:   case AF_FORMAT_FLOATP:
    case AF_FORMAT_DOUBLE:  case AF_FORMAT_DOUBLEP:
        return true;
    default:
        return false;
    }
}

enum FilterDirty : quint32 {
    Normalizer = 1,
    Muted = 2,
    ChMap = 8,
    Format = 16,
    Scale = 32,
    Resample = 64,
    Clip = 128,
    Equalizer = 256
};

struct AudioController::Data {
    quint32 dirty = 0;
    int fmt_conv = AF_FORMAT_UNKNOWN, outrate = 0;
    SpeedMeasure<quint64> measure{10, 30};
    int srate = 0;
    quint64 samples = 0;
    bool normalizerActivated = false, tempoScalerActivated = false, eof = false;
    double scale = 1.0, amp = 1.0, gain = 1.0;
    mp_chmap chmap;
    af_instance *af = nullptr;
    AudioNormalizerOption normalizerOption;
    bool softClip = false;
    ChannelLayoutMap map = ChannelLayoutMap::default_();
    ChannelLayout layout = ChannelLayoutInfo::default_();
    AudioEqualizer eq;
    AudioFormat from, to;
    AudioVisualizer vis;

    static constexpr af_format fmt_interm = AF_FORMAT_FLOAT;
    af_format fmt_to = AF_FORMAT_UNKNOWN;

    AudioResampler resampler;
    AudioAnalyzer analyzer;
    AudioScaler scaler;
    AudioMixer mixer;
    AudioConverter converter;
    AudioBufferPtr input;
    QVector<AudioBufferPtr> forFft;
    QVector<AudioFilter*> filters;
    QVector<AudioFilter*> chain;

    QMutex mutex;
};

AudioController::AudioController(QObject *parent)
    : QObject(parent)
    , d(new Data)
{
    d->measure.setTimer([=] () {
        if (_Change(d->srate, qRound(d->measure.get())))
            emit samplerateChanged(d->srate);
        if (_Change<double>(d->gain, d->normalizerActivated ? d->analyzer.gain() : -1))
            emit gainChanged(d->gain);
    }, 100000);

    d->chain << &d->scaler << &d->mixer << &d->converter;
    d->filters << &d->resampler << &d->analyzer << d->chain;
}

AudioController::~AudioController()
{
    delete d;
}

auto AudioController::setSoftClip(bool soft) -> void
{
    d->softClip = soft;
    d->dirty |= Clip;
}

auto AudioController::test(int fmt_in, int fmt_out) -> bool
{
    return AudioResampler::canAccept(fmt_in) && isSupported(fmt_out);
}

auto AudioController::open(af_instance *af) -> int
{
    auto p = static_cast<bomi_af_priv*>(af->priv);
    p->ac = address_cast<AudioController*>(p->address);
    auto d = p->ac->d;
    d->af = af;
    d->tempoScalerActivated = p->use_scaler;
    d->normalizerActivated = p->use_normalizer;
//    d->layout = ChannelLayoutInfo::from(priv->layout);

    af->control = [] (af_instance *af, int cmd, void *arg) -> int
        { return priv(af)->control(cmd, arg); };
    af->uninit = [] (af_instance *af) -> void { priv(af)->uninit(); };
    af->filter_frame = [] (af_instance *af, mp_audio *data) -> int
        { return priv(af)->filter(data); };
    af->filter_out = [] (af_instance *af) -> int { return priv(af)->output(); };
    af->delay = 0.0;

    return AF_OK;
}

auto AudioController::uninit() -> void
{
    d->af = nullptr;
    d->layout = ChannelLayoutInfo::default_();
    d->input = AudioBufferPtr();
}

auto AudioController::reinitialize(mp_audio *from) -> int
{
    if (!from)
        return AF_ERROR;
    d->measure.reset();
    d->samples = 0;
    if (_Change(d->srate, 0))
        emit samplerateChanged(d->srate);
    if (_Change(d->gain, 1.0))
        emit samplerateChanged(d->gain);

    auto makeFormat = [] (const mp_audio *audio) {
        AudioFormat format;
        format.m_samplerate = audio->rate;
        format.m_bitrate = audio->rate * audio->nch * audio->bps * 8;
        format.m_bits = audio->bps * 8;
        const auto layout = ChannelLayoutMap::toLayout(audio->channels);
        format.m_channels = ChannelLayoutInfo::description(layout);
        format.m_nch = audio->nch;
        format.m_type = _L(af_fmt_to_str(audio->format));
        return format;
    };
    auto to = d->af->data;
    to->rate = from->rate;
    const auto supported = AudioResampler::canAccept(from->format);
    if (!supported)
        mp_audio_set_format(from, AF_FORMAT_FLOAT);
    int fmt_to = d->fmt_conv;
    if (!fmt_to)
        fmt_to = AF_FORMAT_FLOAT;
    mp_audio_set_format(to, fmt_to);
    d->fmt_conv = AF_FORMAT_UNKNOWN;
    d->chmap = from->channels;
    if (!_ChmapFromLayout(&d->chmap, d->layout))
        _Error("Cannot find matched channel layout for '%%'",
               ChannelLayoutInfo::description(d->layout));
    mp_audio_set_channels(to, &d->chmap);
    if (d->outrate != 0)
        to->rate = d->outrate;
    if (!supported)
        return false;

    if (_Change(d->from, makeFormat(from)))
        emit inputFormatChanged();
    if (_Change(d->to, makeFormat(to)))
        emit outputFormatChanged();

    const AudioBufferFormat buf_from(from);
    const AudioBufferFormat buf_mixer_in(d->fmt_interm, from->channels, to->rate);
    const AudioBufferFormat buf_mixer_out(d->fmt_interm, to->channels, to->rate);
    const AudioBufferFormat buf_to(to);

    d->resampler.setFormat(buf_from, buf_mixer_in);
    d->analyzer.setFormat(buf_mixer_in);
    d->scaler.setFormat(buf_mixer_in);
    d->mixer.setFormat(buf_mixer_in, buf_mixer_out);
    d->mixer.setChannelLayoutMap(d->map);
    d->mixer.setSoftClip(d->softClip);
    d->converter.setFormat(buf_to);

    d->fmt_to = (af_format)to->format;
    d->dirty = 0xffffffff;
    d->eof = false;

    for (auto filter : d->filters) {
        filter->setPool(d->af->out_pool);
        filter->reset();
    }
    d->vis.reset();
    emit gainChanged(d->gain = d->normalizerActivated ? d->analyzer.gain() : -1);
    return true;
}

auto AudioController::control(int cmd, void *arg) -> int
{
    switch(cmd){
    case AF_CONTROL_REINIT:
        return reinitialize(static_cast<mp_audio*>(arg));
    case AF_CONTROL_SET_VOLUME:
        d->amp = *((float*)arg);
        return AF_OK;
    case AF_CONTROL_GET_VOLUME:
        *((float*)arg) = d->amp;
        return AF_OK;
    case AF_CONTROL_SET_PLAYBACK_SPEED:
        d->scale = *(double*)arg;
        d->dirty |= Scale;
        return d->tempoScalerActivated;
    case AF_CONTROL_SET_FORMAT:
        d->fmt_conv = *(int*)arg;
        if (!isSupported(d->fmt_conv))
            d->fmt_conv = AF_FORMAT_UNKNOWN;
        return !!d->fmt_conv;
    case AF_CONTROL_SET_RESAMPLE_RATE:
        d->outrate = *(int *)arg;
        d->dirty |= Resample;
        return AF_OK;
    case AF_CONTROL_SET_CHANNELS:
        d->layout = ChannelLayoutMap::toLayout(*(mp_chmap*)arg);
        return AF_OK;
    case AF_CONTROL_RESET:
        for (auto filter : d->filters)
            filter->reset();
        return AF_OK;
    default:
        return AF_UNKNOWN;
    }
}

auto AudioController::filter(mp_audio *data) -> int
{
    if (d->dirty) {
        d->mutex.lock();
        if (d->dirty & Normalizer) {
            d->analyzer.setNormalizerActive(d->normalizerActivated);
            d->analyzer.setNormalizerOption(d->normalizerOption);
        }
        if (d->dirty & Scale) {
            d->scaler.setActive(d->tempoScalerActivated);
            for (auto filter : d->filters)
                filter->setScale(d->scale);
        }
        if (d->dirty & ChMap)
            d->mixer.setChannelLayoutMap(d->map);
        if (d->dirty & Clip)
            d->mixer.setSoftClip(d->softClip);
        if (d->dirty & Equalizer)
            d->mixer.setEqualizer(d->eq);
        d->dirty = 0;
        d->mutex.unlock();
    }

    d->eof = !data;
    if (d->eof)
        return 0;
    d->measure.push(d->samples += data->samples);
    d->input = AudioBuffer::fromMpAudio(data);
    return 0;
}

auto AudioController::output() -> int
{
    if (d->input) {
        auto buffer = d->resampler.run(d->input);
        d->input = AudioBufferPtr();
        d->analyzer.push(buffer);
    }
    do {
        auto buffer = d->analyzer.pull(d->eof);
        if (!buffer || buffer->isEmpty())
            break;
        if (d->vis.isActive())
            d->vis.analyze(buffer);
        d->mixer.setAmplifier(d->amp * d->analyzer.gain());
        for (auto filter : d->chain) {
            if (!filter->passthrough(buffer))
                buffer = filter->run(buffer);
        }
        auto audio = buffer->take();
        Q_ASSERT(mp_audio_config_equals(&d->af->fmt_out, audio));
        af_add_output_frame(d->af, audio);
    } while (false);

    d->af->delay = 0;
    for (auto filter : d->filters)
        d->af->delay += filter->delay();
    return 0;
}

auto AudioController::setAnalyzeSpectrum(bool on) -> void
{
    d->vis.setActive(on);
}

auto AudioController::samplerate() const -> int
{
    return d->srate;
}

auto AudioController::inputFormat() const -> AudioFormat
{
    return d->from;
}

auto AudioController::outputFormat() const -> AudioFormat
{
    return d->to;
}

auto AudioController::gain() const -> double
{
    return d->gain;
}

auto AudioController::isTempoScalerActivated() const -> bool
{
    return d->tempoScalerActivated;
}

auto AudioController::setNormalizerOption(const AudioNormalizerOption &option)
-> void
{
    d->normalizerOption = option;
    d->dirty |= Normalizer;
}

auto AudioController::isNormalizerActivated() const -> bool
{
    return d->normalizerActivated;
}

auto AudioController::setChannelLayoutMap(const ChannelLayoutMap &map) -> void
{
    d->map = map;
    d->dirty |= ChMap;
}

auto AudioController::setOutputChannelLayout(ChannelLayout layout) -> void
{
    d->layout = layout;
    d->dirty |= ChMap;
}

af_info create_info() {
#define MPV_OPTION_BASE bomi_af_priv
    static m_option options[] = {
        MPV_OPTION(address),
        MPV_OPTION(use_scaler),
        MPV_OPTION(use_normalizer),
        MPV_OPTION(layout),
        mpv::null_option
    };
    static af_info info = {
        "bomi audio controller",
        "dummy",
        AF_FLAGS_NOT_REENTRANT,
        AudioController::open,
        sizeof(bomi_af_priv),
        nullptr,
        options
    };
    return info;
}

auto AudioController::setEqualizer(const AudioEqualizer &eq) -> void
{
    d->mutex.lock();
    d->eq = eq;
    d->dirty |= Equalizer;
    d->mutex.unlock();
}

auto AudioController::visualizer() const -> AudioVisualizer*
{
    return &d->vis;
}
