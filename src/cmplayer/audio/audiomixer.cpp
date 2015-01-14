#include "audiomixer.hpp"

static auto LambertW1(const double z) -> double {
    const double eps=4.0e-16, em1=0.3678794411714423215955237701614608;
    double p = 1.0, e, t, w, l1, l2;
    Q_ASSERT(-em1 <= z && z <0.0);
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

static const auto compressInfo = CompressInfo::create();

AudioMixer::AudioMixer()
{
}

static auto softclip(float p) -> float
{
    return (p >= M_PI*0.5) ? 1.0 : ((p <= -M_PI*0.5) ? -1.0 : std::sin(p));
}

static auto hardclip(float p) -> float
{
    return p < -1.0 ? -1.0 : p > 1.0 ? 1.0 : p;
}

auto AudioMixer::run(AudioBuffer *in) -> AudioBuffer*
{
    auto &src = *in;
    const int frames = src.frames();
    m_dst.expand(frames);
    if (m_dst.isEmpty())
        return in;
    const double gain = m_amp;
    auto clip = m_realClip == ClippingMethod::Soft ? softclip : hardclip;
    if (m_amp < 1e-8)
        m_dst.fill(0);
    else if (!m_mix) {
        auto iit = in->begin(), oit = m_dst.begin();
        while (iit != in->end())
            *oit++ = *iit++ * gain;
    } else {
        auto dit = m_dst.begin();
        for (auto sit = in->begin(); sit != in->end(); sit += in->channels()) {
            for (int dch = 0; dch < m_dst.channels(); ++dch) {
                const auto spk = m_out.channels.speaker[dch];
                auto &map = m_ch_man.sources(spk);
                double v = 0;
                for (int i=0; i<map.size(); ++i)
                    v += sit[m_ch_index_src[map[i]]]*gain;
                if (map.size() > 1) {
                    // ref: http://www.voegler.eu/pub/audio/
                    //      digital-audio-mixing-and-normalization.html
                    const auto &info = compressInfo[map.size()];
                    if (v < 0)
                        v = -log(1.0 - info.c1*v)*info.c2;
                    else
                        v = +log(1.0 + info.c1*v)*info.c2;
                }
                *dit++ = clip(v);
            }
        }
    }
    return &m_dst;
}

auto AudioMixer::setFormat(const AudioBufferFormat &in, const AudioBufferFormat &out) -> void
{
    if (!(_Change(m_in, in) | _Change(m_out, out)))
        return;
    m_in = in; m_out = out;
    for (int i=0; i<out.channels.num; ++i)
        m_ch_index_dst[out.channels.speaker[i]] = i;
    for (int i=0; i<in.channels.num; ++i)
        m_ch_index_src[in.channels.speaker[i]] = i;
    m_mix = !m_ch_man.isIdentity();
    m_updateChmap = !mp_chmap_equals(&in.channels, &out.channels);
    m_updateFormat = in.type != out.type;
    setClippingMethod(m_clip);
    setChannelLayoutMap(m_map);
    m_dst = {out.channels.num};
}

auto AudioMixer::setClippingMethod(ClippingMethod method) -> void
{
    m_realClip = m_clip = method;
    if (m_realClip == ClippingMethod::Auto)
        m_realClip = ClippingMethod::Soft;
}
