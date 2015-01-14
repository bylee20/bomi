#include "audiocontroller.hpp"
#include "audioformat.hpp"
#include "audiomixer.hpp"
#include "audioscaler.hpp"
#include "audioanalyzer.hpp"
#include "audioconverter.hpp"
#include "audioresampler.hpp"
#include "player/mpv_helper.hpp"
#include "enum/channellayout.hpp"
#include "misc/log.hpp"
#include "misc/speedmeasure.hpp"
extern "C" {
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <audio/filter/af.h>
#include <audio/fmt-conversion.h>
}

DECLARE_LOG_CONTEXT(Audio)

af_info create_info();
af_info af_info_dummy = create_info();

struct cmplayer_af_priv {
    AudioController *ac;
    char *address;
    int use_scaler, layout;
};

static auto priv(af_instance *af) -> AudioController*
    { return static_cast<cmplayer_af_priv*>(af->priv)->ac; }

static auto isSupported(int type) -> bool
{
    switch (type) {
    case AF_FORMAT_S8:
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
    Clip = 128
};

struct AudioController::Data {
    quint32 dirty = 0;
    SwrContext *swr = nullptr;
    int fmt_conv = AF_FORMAT_UNKNOWN, outrate = 0;
    SpeedMeasure<quint64> measure{10, 30};
    int srate = 0;
    quint64 samples = 0;
    bool normalizerActivated = false, tempoScalerActivated = false;
    bool resample = false;
    double scale = 1.0, amp = 1.0, gain = 1.0;
    mp_chmap chmap;
    mp_audio *resampled = nullptr;
    af_instance *af = nullptr;
    AudioNormalizerOption normalizerOption;
    ClippingMethod clip = ClippingMethod::Auto;
    ChannelLayoutMap map = ChannelLayoutMap::default_();
    ChannelLayout layout = ChannelLayoutInfo::default_();
    AudioFormat from, to;

    static constexpr af_format fmt_interm = AF_FORMAT_FLOAT;
    af_format fmt_to = AF_FORMAT_UNKNOWN;

    AudioResampler resampler;
    AudioScaler scaler;
    AudioAnalyzer analyzer;
    AudioMixer mixer;
    AudioConverter converter;
    AudioBuffer src;
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
}

AudioController::~AudioController()
{
    delete d;
}

auto AudioController::setClippingMethod(ClippingMethod method) -> void
{
    d->clip = method;
    d->dirty |= Clip;
}

auto AudioController::test(int fmt_in, int fmt_out) -> bool
{
    return fmt_in && isSupported(fmt_out);
}

auto AudioController::open(af_instance *af) -> int
{
    auto priv = static_cast<cmplayer_af_priv*>(af->priv);
    priv->ac = address_cast<AudioController*>(priv->address);
    auto d = priv->ac->d;
    d->af = af;
    d->tempoScalerActivated = priv->use_scaler;
//    d->layout = ChannelLayoutInfo::from(priv->layout);

    af->control = AudioController::control;
    af->uninit = AudioController::uninit;
    af->filter = AudioController::filter;
    af->delay = 0.0;

    return AF_OK;
}

auto AudioController::uninit(af_instance *af) -> void
{
    auto ac = priv(af); auto d = ac->d;
    Q_ASSERT(ac != nullptr);
    d->af = nullptr;
    if (d->swr)
        swr_free(&d->swr);
    talloc_free(d->resampled);
    d->layout = ChannelLayoutInfo::default_();
    d->resampled = nullptr;
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
    const auto supported = isSupported(from->format);
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
    d->mixer.setClippingMethod(d->clip);
    d->converter.setFormat(buf_to);

    d->fmt_to = (af_format)to->format;
    d->dirty = 0xffffffff;
    d->src.setForRawData(buf_from.type, from->nch);
    return true;
}

auto AudioController::control(af_instance *af, int cmd, void *arg) -> int
{
    auto ac = priv(af);
    auto d = ac->d;
    switch(cmd){
    case AF_CONTROL_REINIT:
        return ac->reinitialize(static_cast<mp_audio*>(arg));
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
        if (d->swr)
            while (swr_drop_output(d->swr, 1000) > 0) ;
        return AF_OK;
    default:
        return AF_UNKNOWN;
    }
}

auto AudioController::filter(af_instance *af, mp_audio *data, int flags) -> int
{
    auto ac = priv(af); AudioController::Data *d = ac->d;
    if (data->samples <= 0 && flags & AF_FILTER_FLAG_EOF) {
        d->af->data->samples = 0;
        *data = *d->af->data;
        return 0;
    }

    d->af->delay = 0.0;

    if (d->dirty) {
        if (d->dirty & Normalizer) {
            d->analyzer.setNormalizerActive(d->normalizerActivated);
            d->analyzer.setNormalizerOption(d->normalizerOption);
        }
        if (d->dirty & Scale)
            d->scaler.setScale(d->tempoScalerActivated, d->scale);
        if (d->dirty & ChMap)
            d->mixer.setChannelLayoutMap(d->map);
        if (d->dirty & Clip)
            d->mixer.setClippingMethod(d->clip);
        d->dirty = 0;
    }

    Q_ASSERT(data->num_planes == d->src.planes());

    d->src.setRawFrames(data->samples);
    for (int i = 0; i < data->num_planes; ++i)
        d->src.setRawData((uchar*)data->planes[i], i);
    auto buffer = d->resampler.run(&d->src);
    buffer = d->analyzer.run(buffer);
    buffer = d->scaler.run(buffer);
    d->mixer.setAmplifier(d->amp * d->analyzer.gain());
    buffer = d->mixer.run(buffer);
    buffer = d->converter.run(buffer);

    *data = *d->af->data;
    Q_ASSERT(data->num_planes == buffer->planes());
    for (int i = 0; i < data->num_planes; ++i)
        data->planes[i] = buffer->plane(i);
    data->samples = buffer->frames();
    af->delay += d->scaler.delay();
    d->measure.push(d->samples += data->samples);
    return 0;
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

auto AudioController::setNormalizerActivated(bool on) -> void
{
    if (_Change(d->normalizerActivated, on))
        d->dirty |= Normalizer;
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
#define MPV_OPTION_BASE cmplayer_af_priv
    static m_option options[] = {
        MPV_OPTION(address),
        MPV_OPTION(use_scaler),
        MPV_OPTION(layout),
        mpv::null_option
    };
    static af_info info = {
        "CMPlayer audio controller",
        "dummy",
        AF_FLAGS_NOT_REENTRANT,
        AudioController::open,
        AudioController::test,
        sizeof(cmplayer_af_priv),
        nullptr,
        options
    };
    return info;
}
