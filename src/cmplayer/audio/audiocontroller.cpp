#include "audiocontroller.hpp"
#include "audiomixer.hpp"
#include "audioformat.hpp"
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
    case AF_FORMAT_S16P:
    case AF_FORMAT_S32P:
    case AF_FORMAT_FLOATP:
    case AF_FORMAT_DOUBLEP:
    case AF_FORMAT_S16:
    case AF_FORMAT_S32:
    case AF_FORMAT_FLOAT:
    case AF_FORMAT_DOUBLE:
        return true;
    default:
        return false;
    }
}

enum FilterDirty : quint32 {
    Normalizer = 1,
    Muted = 2,
    Amp = 4,
    ChMap = 8,
    Format = 16,
    Scale = 32,
    Resample = 64,
    Clip = 128
};

struct AudioController::Data {
    quint32 dirty = 0;
    AudioMixer *mixer = nullptr;
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
    AudioFormat input, output;
};

AudioController::AudioController(QObject *parent)
    : QObject(parent)
    , d(new Data)
{
    d->measure.setTimer([=] () {
        if (_Change(d->srate, qRound(d->measure.get())))
            emit samplerateChanged(d->srate);
        if (_Change<double>(d->gain, d->normalizerActivated ? d->mixer->gain() : -1))
            emit gainChanged(d->gain);
    }, 100000);
}

AudioController::~AudioController()
{
    delete d->mixer;
    delete d;
}

auto AudioController::setClippingMethod(ClippingMethod method) -> void
{
    d->clip = method;
    d->dirty |= Clip;
}

auto AudioController::test(int fmt_in, int fmt_out) -> bool
{
    return isSupported(fmt_in) && isSupported(fmt_out);
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
    af->mul = 1;
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

auto AudioController::reinitialize(mp_audio *in) -> int
{
    if (!in)
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
    if (_Change(d->input, makeFormat(in)))
        emit inputFormatChanged();
    auto out = d->af->data;
    out->rate = in->rate;
    bool ret = true;
    if (!isSupported(in->format)) {
        ret = false;
        mp_audio_set_format(in, af_fmt_is_planar(in->format) ? AF_FORMAT_FLOATP
                                                             : AF_FORMAT_FLOAT);
    }
    int fmtout = d->fmt_conv;
    if (!fmtout)
        fmtout = af_fmt_is_planar(in->format) ? AF_FORMAT_FLOATP : AF_FORMAT_FLOAT;
    mp_audio_set_format(out, fmtout);
    d->fmt_conv = AF_FORMAT_UNKNOWN;
    d->chmap = in->channels;
    if (!_ChmapFromLayout(&d->chmap, d->layout))
        _Error("Cannot find matched channel layout for '%%'",
               ChannelLayoutInfo::description(d->layout));
    mp_audio_set_channels(out, &d->chmap);
    if (d->outrate != 0)
        out->rate = d->outrate;
    if (!ret)
        return false;

    d->af->mul = (double)out->channels.num/in->channels.num;
    if (d->tempoScalerActivated)
        d->af->mul /= d->scale;
    if ((d->resample = out->rate != in->rate)) {
        d->af->mul *= (double)out->rate/in->rate;
        const auto nch = in->channels.num;
        /*mp_chmap_to_lavc_unchecked(&in->channels);*/
        const auto fmt = af_to_avformat(in->format);
        if (!d->swr)
            d->swr = swr_alloc();
        av_opt_set_int(d->swr,  "in_channel_count", nch, 0);
        av_opt_set_int(d->swr, "out_channel_count", nch, 0);
        av_opt_set_int(d->swr,  "in_sample_rate", in->rate, 0);
        av_opt_set_int(d->swr, "out_sample_rate", out->rate, 0);
        av_opt_set_sample_fmt(d->swr,  "in_sample_fmt", fmt, 0);
        av_opt_set_sample_fmt(d->swr, "out_sample_fmt", fmt, 0);
        swr_init(d->swr);
        if (!d->resampled)
            d->resampled = talloc_zero(nullptr, mp_audio);
        *d->resampled = *in;
        d->resampled->rate = out->rate;
        in = d->resampled;
    }
    if (_Change(d->output, makeFormat(out)))
        emit outputFormatChanged();
    const AudioDataFormat fmt_in(*in), fmt_out(*out);
    if (!d->mixer || !d->mixer->configure(fmt_in, fmt_out)) {
        delete d->mixer;
        d->mixer = AudioMixer::create(fmt_in, fmt_out);
    }
    Q_ASSERT(d->mixer);
    d->mixer->setOutput(out);
    d->mixer->setChannelLayoutMap(d->map);
    d->mixer->setClippingMethod(d->clip);
    d->dirty = 0xffffffff;
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
        d->dirty |= Amp;
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
    auto ac = priv(af); auto d = ac->d;
    if (data->samples <= 0 && flags & AF_FILTER_FLAG_EOF) {
        d->af->data->samples = 0;
        *data = *d->af->data;
        return 0;
    }

    d->af->delay = 0.0;

    Q_ASSERT(d->mixer != nullptr);
    if (d->dirty) {
        if (d->dirty & Amp)
            d->mixer->setAmp(d->amp);
        if (d->dirty & Normalizer)
            d->mixer->setNormalizer(d->normalizerActivated,
                                    d->normalizerOption);
        if (d->dirty & ChMap)
            d->mixer->setChannelLayoutMap(d->map);
        if (d->dirty & Scale)
            d->mixer->setScaler(d->tempoScalerActivated, d->scale);
        if (d->dirty & Clip)
            d->mixer->setClippingMethod(d->clip);
        d->dirty = 0;
    }

    const mp_audio *in = data;
    if (d->resample) {
        const int frames_delay = swr_get_delay(d->swr, data->rate);
        const int frames = av_rescale_rnd(frames_delay + data->samples,
                                          d->resampled->rate,
                                          data->rate, AV_ROUND_UP);
        mp_audio_realloc_min(d->resampled, frames);
        d->af->delay = (double)frames/data->rate;
        if ((d->resampled->samples = frames))
            d->resampled->samples = swr_convert(d->swr,
                                                (uchar**)d->resampled->planes,
                                                d->resampled->samples,
                                                (const uchar**)data->planes,
                                                data->samples);
        in = d->resampled;
    }
    d->mixer->apply(in);
    *data = *d->af->data;
    af->delay += d->mixer->delay();
    d->measure.push(d->samples += data->samples);
    return 0;
}

auto AudioController::samplerate() const -> int
{
    return d->srate;
}

auto AudioController::inputFormat() const -> AudioFormat
{
    return d->input;
}

auto AudioController::outputFormat() const -> AudioFormat
{
    return d->output;
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
