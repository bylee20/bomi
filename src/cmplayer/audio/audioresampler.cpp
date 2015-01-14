#include "audioresampler.hpp"
extern "C" {
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <audio/fmt-conversion.h>
}

struct AudioResampler::Data {
    SwrContext *swr = nullptr;
    AudioBufferFormat in, out;
    bool resample = false;
    double delay = 0.0;
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

auto AudioResampler::setFormat(const AudioBufferFormat &in, const AudioBufferFormat &out) -> void
{
    if (!(_Change(d->in, in) | _Change(d->out, out)))
        return;
    d->resample = d->in != d->out;
    if (!d->resample)
        return;
    if (!d->swr)
        d->swr = swr_alloc();
    Q_ASSERT(in.channels().num == out.channels().num);
    const auto nch = in.channels().num;
    av_opt_set_int(d->swr,  "in_channel_count", nch, 0);
    av_opt_set_int(d->swr, "out_channel_count", nch, 0);
    av_opt_set_int(d->swr,  "in_sample_rate", d->in.fps(), 0);
    av_opt_set_int(d->swr, "out_sample_rate", d->out.fps(), 0);
    av_opt_set_sample_fmt(d->swr,  "in_sample_fmt", af_to_avformat(d->in.type()), 0);
    av_opt_set_sample_fmt(d->swr, "out_sample_fmt", af_to_avformat(d->out.type()), 0);
    swr_init(d->swr);
}

auto AudioResampler::run(AudioBufferPtr in) -> AudioBufferPtr
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
