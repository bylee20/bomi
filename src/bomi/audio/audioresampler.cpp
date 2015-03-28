#include "audioresampler.hpp"
extern "C" {
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <audio/fmt-conversion.h>
}

struct AudioResampler::Data {
    SwrContext *swr = nullptr;
    AudioBufferFormat in, out;
    int fps = 1;
    bool resample = false;
    double delay = 0.0, scale = 1.0;
    auto updateFps() -> void { fps = lrint(in.fps() * scale); }
};

AudioResampler::AudioResampler()
    : d(new Data)
{

}

AudioResampler::~AudioResampler()
{
    if (d->swr)
        swr_free(&d->swr);
    delete d;
}

auto AudioResampler::canAccept(int format) -> bool
{
    return af_to_avformat(format) != AV_SAMPLE_FMT_NONE;
}

auto AudioResampler::reconfigure() -> void
{
    d->updateFps();
    if (d->swr)
        swr_free(&d->swr);
    if (!d->resample)
        return;
    d->swr = swr_alloc();
    const auto nch = d->in.channels().num;
    av_opt_set_int(d->swr,  "in_channel_count", nch, 0);
    av_opt_set_int(d->swr, "out_channel_count", nch, 0);
    av_opt_set_int(d->swr,  "in_sample_rate", d->in.fps(), 0);
    av_opt_set_int(d->swr, "out_sample_rate", d->out.fps(), 0);
    av_opt_set_sample_fmt(d->swr,  "in_sample_fmt", af_to_avformat(d->in.type()), 0);
    av_opt_set_sample_fmt(d->swr, "out_sample_fmt", af_to_avformat(d->out.type()), 0);
    reset();
}

auto AudioResampler::setFormat(const AudioBufferFormat &in, const AudioBufferFormat &out) -> void
{
    d->delay = 0.0;
    if (!(_Change(d->in, in) | _Change(d->out, out)))
        return;
    Q_ASSERT(d->in.channels().num == d->out.channels().num);
    d->resample = d->in != d->out;

    reconfigure();
}

auto AudioResampler::delay() const -> double
{
    return d->delay;
}

auto AudioResampler::passthrough(const AudioBufferPtr &/*in*/) const -> bool
{
    return !d->resample;
}

auto AudioResampler::run(AudioBufferPtr &in) -> AudioBufferPtr
{
    d->delay = 0;
    if (!d->resample)
        return in;
    const int frames_delay = swr_get_delay(d->swr, d->in.fps());
    int frames = av_rescale_rnd(frames_delay + in->frames(),
                                d->out.fps(), d->in.fps(), AV_ROUND_UP);
    auto dst = newBuffer(d->out, frames);
    d->delay = (double)frames/d->in.fps();
    if (frames > 0) {
        frames = swr_convert(d->swr, dst->data(), frames,
                             in->constData(), in->frames());
        dst->expand(frames);
    }
    return dst;
}

auto AudioResampler::setScale(double scale) -> void
{
    d->scale = scale;
}

auto AudioResampler::reset() -> void
{
    if (!d->swr)
        return;
    swr_close(d->swr);
    swr_init(d->swr);
}
