#include "audiomixer.hpp"

static auto LambertW1(const double z) -> double {
    const double eps=4.0e-16, em1=0.3678794411714423215955237701614608;
    double p = 1.0, e, t, w, l1, l2;
    Q_ASSERT(-em1 <= z && z <0.0); Q_UNUSED(em1);
    /* initial approx for iteration... */
    if (z < -1e-6) { /* series about -1/e */
        p = -sqrt(2.0 * (2.7182818284590452353602874713526625 * z + 1.0));
        w = -1.0 + p * (1.0 + p * (-0.333333333333333333333
                                   + p * 0.152777777777777777777777));
    } else { /* asymptotic near zero */
        l1 = log(-z);
        l2 = log(-l1);
        w = l1 - l2 + l2 / l1;
    }
    if (fabs(p) < 1e-4)
        return w;
    for (int i = 0; i < 10; ++i) { /* Halley iteration */
        e = exp(w);
        t = w * e - z;
        p = w + 1.0;
        t /= e * p - 0.5 * (p + 1.0) * t / p;
        w -= t;
        if (fabs(t) < eps * (1.0 + fabs(w)))
            return w; /* rel-abs error */
    }
    Q_ASSERT(false);
    return 0.0;
}

SIA alpha(double t, int N) -> double {
    const double a = (N - t)/(1.0 - t);
    const double v = -exp(-1.0/a)/a;
    return -a*LambertW1(v) - 1.0;
}

struct CompressInfo {
    double alpha = 0.0, c1 = 0.0, c2 = 1.0;
    static auto create(double t = 0.0,
                       int count = 10) -> std::vector<CompressInfo>
    {
        std::vector<CompressInfo> list(count);
        for (int i = 2; i < count; ++i) {
            auto &info = list[i];
            info.alpha = ::alpha(t, i);
            info.c1 = info.alpha/(i - t);
            info.c2 = 1.0/log(1.0 + info.alpha);
        }
        return list;
    }
};

static auto softclip(float p) -> float
{
    return (p >= M_PI*0.5) ? 1.0 : ((p <= -M_PI*0.5) ? -1.0 : std::sin(p));
}

static auto hardclip(float p) -> float
{
    return p < -1.0 ? -1.0 : p > 1.0 ? 1.0 : p;
}

static constexpr int Bands = AudioEqualizer::bands();

struct AudioMixer::Data {
    AudioBufferFormat in, out;
    float amp = 1.0;
    bool softClip = false;
    bool mix = true;
    std::array<int, MP_SPEAKER_ID_COUNT> ch_index_src, ch_index_dst;
    ChannelManipulation ch_man;
    ChannelLayoutMap map;
    AudioEqualizer eq;
    bool eq_zero = true;

    const std::vector<CompressInfo> compressInfo = CompressInfo::create();

    struct { float a = 0, b = 0, c = 0, amp = 0; } coefs[Bands];
    struct { float x[2], y[Bands][2]; } xys[MP_NUM_CHANNELS];
};

auto AudioMixer::delay() const -> double
{
    // follow the estimation in af_equalizer.c of mpv
    return d->eq_zero ? 0.0 : 2.0 / d->out.fps();
}

AudioMixer::AudioMixer()
    : d(new Data)
{
}

AudioMixer::~AudioMixer()
{
    delete d;
}

auto AudioMixer::setAmplifier(float level) -> void
{
    d->amp = level;
}

auto AudioMixer::setChannelLayoutMap(const ChannelLayoutMap &map) -> void
{
    d->map = map;
    d->ch_man = map(d->in.channels(), d->out.channels());
    d->mix = d->in != d->out || !d->map.isIdentity(d->in.channels(), d->out.channels());
}

auto AudioMixer::setEqualizer(const AudioEqualizer &eq) -> void
{
    d->eq = eq;
    d->eq_zero = eq.isZero();
    if (d->eq_zero) {
        for (auto &c : d->coefs)
            c.amp = 0.0;
    } else {
        for (int i = 0; i < eq.size(); ++i) {
            const auto db = qBound(eq.min(), eq[i], eq.max());
            d->coefs[i].amp = std::pow(10., db / 20.) - 1.;
        }
    }
}

auto AudioMixer::setFormat(const AudioBufferFormat &in, const AudioBufferFormat &out) -> void
{
    if (!(_Change(d->in, in) | _Change(d->out, out)))
        return;
    d->in = in; d->out = out;
    for (int i=0; i<out.channels().num; ++i)
        d->ch_index_dst[out.channels().speaker[i]] = i;
    for (int i=0; i<in.channels().num; ++i)
        d->ch_index_src[in.channels().speaker[i]] = i;
    setChannelLayoutMap(d->map);

    const float fps = out.fps();
    const float f_max = 0.5f * fps;
    const float w_band = 1; // bandwidth in octave
    for (int i = 0; i < Bands; ++i) {
        const float f_center = AudioEqualizer::freqeuncy(i);
        auto &c = d->coefs[i];
        if (f_center < f_max) {
            const float theta = 2.0f * M_PI * f_center / fps;
            const float alpha = sin(theta) * sinh(log(2.0)*0.5 * w_band * theta/sin(theta));
            c.a = alpha / (alpha + 1.f);
            c.b = 2.0 * cos(theta) / (alpha + 1.f);
            c.c = (alpha - 1.f) / (alpha + 1.f);
        } else
            c.a = c.b = c.c = 0.f;
    }
    memset(d->xys, 0, sizeof(d->xys));
    setEqualizer(d->eq);
}

auto AudioMixer::passthrough(const AudioBufferPtr &/*in*/) const -> bool
{
    return false;
}

auto AudioMixer::run(AudioBufferPtr &src) -> AudioBufferPtr
{
    const int frames = src->frames();
    if (src->isEmpty())
        return newBuffer(d->out, frames);
    AudioBufferPtr dest;
    if (d->mix)
        dest = newBuffer(d->out, frames);
    else
        dest = src;
    auto dview = dest->view<float>();
    auto sview = src->constView<float>();
    auto clip = d->softClip ? softclip : hardclip;
    auto equalize = [=] (float v, int ch) {
        if (!d->eq_zero) {
            const float x = v;
            auto &h = d->xys[ch];
            for(int b = 0; b < Bands; ++b) {
                const auto &c = d->coefs[b];
                const float y = c.a * (x - h.x[1])
                        + c.b * h.y[b][0] + c.c * h.y[b][1];
                h.y[b][1] = h.y[b][0];
                h.y[b][0] = y;
                v += y * c.amp;
            }
            h.x[1] = h.x[0];
            h.x[0] = x;
        }
        return v;
    };

    if (d->amp < 1e-8)
        std::fill(dview.begin(), dview.end(), 0);
    else if (!d->mix) {
        auto it = dview.begin();
        while (it != dview.end()) {
            for (int ch = 0; ch < src->channels(); ++ch, ++it)
                *it = clip(equalize(*it * d->amp, ch));
        }
    } else {
        auto dit = dview.begin();
        for (auto sit = sview.begin(); sit != sview.end(); sit += src->channels()) {
            for (int dch = 0; dch < dest->channels(); ++dch) {
                const auto spk = d->out.channels().speaker[dch];
                auto &map = d->ch_man.sources(spk);
                double v = 0;
                for (int i=0; i<map.size(); ++i)
                    v += sit[d->ch_index_src[map[i]]]*d->amp;
                if (map.size() > 1) {
                    // ref: http://www.voegler.eu/pub/audio/
                    //      digital-audio-mixing-and-normalization.html
                    const auto &info = d->compressInfo[map.size()];
                    if (v < 0)
                        v = -log(1.0 - info.c1*v)*info.c2;
                    else
                        v = +log(1.0 + info.c1*v)*info.c2;
                }
                *dit++ = clip(equalize(v, dch));
            }
        }
    }
    return dest;
}

auto AudioMixer::setSoftClip(bool soft) -> void
{
    d->softClip = soft;
}
