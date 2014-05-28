#include "audiomixer.hpp"
#include "audio_helper.hpp"

template<int fmt>
using sample_type = typename AudioFormatTrait<fmt>::SampleType;

namespace detail {
template<class S>
class AudioScalerData {
    static constexpr int IsInt = tmp::is_integral<S>();
    static constexpr int fmt_out = AudioFormatMpv<S>::interleaving;
    using Helper = AudioSampleHelper<S>;
    using MaxType = tmp::conditional_t<IsInt, qint64, double>;
    using CorrType = MaxType;
    using TableType = MaxType;
public:
    virtual ~AudioScalerData() = default;
    auto delay() const -> double { return m_delay; }
    auto setScale(bool on, double scale) -> void
    {
        m_scale = scale;
        m_frames_stride_error = 0;
        m_frames_stride_scaled = m_scale * m_frames_stride;
        m_frames_to_slide = m_frames_queued = 0;
        m_overlap.fill(0);
        m_enabled = on && scale != 1.0;
    }
    auto output() const -> const AudioDataBuffer<S, false>& { return m_dst; }
protected:
    auto fill_queue(int frames_offset, int frames_in) -> int
    {
        const int offset_unchanged = frames_offset;

        if (m_frames_to_slide > 0) {
            if (m_frames_to_slide < m_frames_queued) {
                const int frames_move = m_frames_queued - m_frames_to_slide;
                m_queue.move(0, m_queue, m_frames_to_slide, frames_move);
                m_frames_to_slide = 0;
                m_frames_queued = frames_move;
            } else {
                m_frames_to_slide -= m_frames_queued;
                const int frames_skip = qMin(m_frames_to_slide, frames_in);
                m_frames_queued = 0;
                m_frames_to_slide -= frames_skip;
                frames_offset += frames_skip;
                frames_in -= frames_skip;
            }
        }

        if (frames_in > 0) {
            const int left = m_queue.frames() - m_frames_queued;
            const int frames_copy = qMin(left, frames_in);
            copySource(&m_queue, m_frames_queued, frames_offset, frames_copy);
            m_frames_queued += frames_copy;
            frames_offset += frames_copy;
        }
        return frames_offset - offset_unchanged;
    }
    auto updateFormat(const AudioDataFormat &format) -> void
    {
        m_format = format;
        const int nch = m_format.channels.num;
        const double frames_per_ms = m_format.fps / 1000.0;
        m_frames_stride = frames_per_ms * m_ms_stride;
        m_frames_overlap = qMax<int>(0, m_frames_stride * m_percent_overlap);

        m_frames_search = 0;
        if (m_frames_overlap > 1)
            m_frames_search = frames_per_ms * m_ms_search;
        m_frames_standing = m_frames_stride - m_frames_overlap;
        m_overlap.setData(m_frames_overlap, nch);
        if (m_overlap.isEmpty())
            return;

        m_table_blend.setData(m_frames_overlap, nch);
        const MaxType blend = IsInt ? 65536 : 1.0;
        for (int i=0; i<m_frames_overlap; ++i)
            m_table_blend.fill(i, blend*i/m_frames_overlap);

        m_table_window.setData(m_frames_overlap-1, nch);
        const MaxType t = m_frames_overlap;
        const MaxType n = IsInt ? 8589934588LL / (t * t) : 1.0;
        for (int i=1; i<m_frames_overlap; ++i)
            m_table_window.fill(i-1, rshift<15, TableType>(i*(t - i)*n));

        m_buf_pre_corr.setData(m_frames_overlap, nch);
        const int frames = m_frames_search + m_frames_overlap + m_frames_stride;
        m_queue.setData(frames, nch);

        m_dst = {nch};
    }
    auto run(int samples_in) -> bool
    {
        m_delay = 0;
        if (!m_enabled || samples_in <= 0)
            return false;
        const int frames_scaled = samples_in / m_frames_stride_scaled + 1;
        const int max_frames_out = frames_scaled * m_frames_stride;
        if (!m_dst.expand(max_frames_out))
            m_dst.adjust(max_frames_out);
        int frames_offset_in = fillQueue(0);
        int frames_out = 0;
        while (m_frames_queued >= m_queue.frames()) {
            int frames_off = 0;

            // output stride
            if (m_frames_overlap> 0) {
                if (m_frames_search > 0)
                    frames_off = best_overlap_frames_offset();
                output_overlap(frames_out, frames_off);
            }

            m_dst.copy(frames_out + m_frames_overlap, m_queue,
                       frames_off + m_frames_overlap, m_frames_standing);
            frames_out += m_frames_stride;

            // input stride
            m_overlap.copy(0, m_queue,
                           frames_off + m_frames_stride, m_frames_overlap);
            const auto target_frames = m_frames_stride_scaled
                                       + m_frames_stride_error;
            const int target_frames_integer = target_frames;
            m_frames_stride_error = target_frames - target_frames_integer;
            m_frames_to_slide = target_frames_integer;
            frames_offset_in += fillQueue(frames_offset_in);
        }
        m_delay = (m_frames_queued - m_frames_to_slide)/m_scale/m_format.fps;
        m_dst.adjust(frames_out);
        return true;
    }
private:
    virtual auto copySource(AudioDataBuffer<S, false> *queue,
                                   int frames_queued, int frames_offset,
                                   int frames_copy) -> void = 0;
    virtual auto fillQueue(int frames_offset) -> int = 0;
    template<int s, class T>
    CIA rshift(const T &t) const -> T
    { return AudioSampleHelper<T>::template rshift<s>(t); }
    auto calculate_correlations() -> void
    {
        _AudioManipulate([&] (CorrType &c, TableType w, S o)
                         { c = rshift<15, MaxType>(w*o); },
                         m_buf_pre_corr, 0, m_overlap.frames()-1,
                         m_table_window, 0, m_overlap, 1);
    }
    auto best_overlap_frames_offset() -> int
    {
        calculate_correlations();
        int best_off = 0; MaxType best_corr = _Min<qint64>(), corr;
        for (int off=0; off<m_frames_search; ++off) {
            corr = 0;
            _AudioManipulate([&] (CorrType c, S q) { corr += c*q; },
                             _C(m_buf_pre_corr), 0, m_overlap.frames()-1,
                             _C(m_queue), 1+off);
            if (corr > best_corr) {
                best_corr = corr;
                best_off  = off;
            }
        }
        return best_off;
    }
    auto output_overlap(int pos, int frames_off) -> void
    {
        _AudioManipulate([&] (S &d, TableType b, S o, S q)
                         { d = o - rshift<16, MaxType>(b*(o - q)); },
                         m_dst, pos, m_overlap.frames(), m_table_blend, 0,
                         m_overlap, 0, m_queue, frames_off);
    }
    static constexpr const double m_ms_stride = 60.0;
    static constexpr const double m_percent_overlap = 0.20;
    static constexpr const double m_ms_search = 14.0;

    AudioDataFormat m_format;
    bool m_enabled = false;
    double m_frames_stride_scaled = 0.0, m_frames_stride_error = 0.0;
    int m_frames_stride = 0, m_frames_overlap = 0, m_frames_queued = 0;
    int m_frames_search = 0, m_frames_standing = 0, m_frames_to_slide = 0;

    AudioDataBuffer<TableType, false> m_table_blend, m_table_window;
    AudioDataBuffer<CorrType, false> m_buf_pre_corr;
    AudioDataBuffer<S, false> m_queue, m_overlap;
    double m_delay = 0.0, m_scale = 1.0;

    AudioDataBuffer<S, false> m_dst;
};
}

template<int fmt_in>
class AudioScaler : public detail::AudioScalerData<sample_type<fmt_in>> {
public:
    using S = sample_type<fmt_in>;
    using Super = detail::AudioScalerData<S>;
    using InBuffer = AudioDataBuffer<S, AudioFormatTrait<fmt_in>::IsPlanar>;
    auto setFormat(const AudioDataFormat &format) -> void
    { Super::updateFormat(format); }
    auto adjusted(const InBuffer &in) -> bool
    { m_src = &in; return Super::run(in.frames()); }
private:
    auto copySource(AudioDataBuffer<S, false> *queue, int frames_queued,
                    int frames_offset, int frames_copy) -> void final
    { queue->copy(frames_queued, *m_src, frames_offset, frames_copy); }
    auto fillQueue(int frames_offset) -> int final
    { return Super::fill_queue(frames_offset, m_src->frames()-frames_offset); }
    const InBuffer *m_src = nullptr;
};

namespace detail {

template<int fmt>
class AudioMixerPre {
    using S = typename AudioFormatTrait<fmt>::SampleType;
    SCA planar = AudioFormatTrait<fmt>::IsPlanar;
    AudioMixer::Data *d = nullptr;
public:
    AudioMixerPre(AudioMixer *mixer): d(&mixer->d) { }
    auto process(const mp_audio *in) -> bool
    {
        m_src.setData(in);
        if (m_src.isEmpty())
            return false;
        if (d->normalizer)
            prepare(m_src, d->normalizerOption);
        m_scaled = m_scaler.adjusted(m_src);
        d->delay = m_scaler.delay();
        return true;
    }
    auto scaledBuffer() const -> const AudioDataBuffer<S, false>&
        { return m_scaler.output(); }
    auto sourceBuffer() const -> const AudioDataBuffer<S, planar>&
        { return m_src; }
    auto configure(const AudioDataFormat &in) -> void
        { m_scaler.setFormat(in); }
    auto setScale(bool on, double scale) -> void
        { m_scaler.setScale(on, scale); d->scale = on ? scale : 1.0;}
    auto isScaled() const -> bool { return m_scaled; }
private:
    auto prepare(const AudioDataBuffer<S, planar> &src,
                 const AudioNormalizerOption &option) -> void
    {
        const int frames = src.frames();
        LevelInfo input(frames);
        _AudioManipulate([&] (S s)
                         { input.level += AudioSampleHelper<S>::toLevel(s); },
                         src, 0, src.frames());
        input.level /= src.samples();
        const auto avg = LevelInfo::average(d->history, input);
        const double targetGain = option.gain(avg.level);
        if (targetGain < 0)
            d->gain = 1.0;
        else {
            const double rate = targetGain/d->gain;
            if (rate > 1.05) {
                d->gain *= 1.05;
            } else if (rate < 0.95)
                d->gain *= 0.95;
            else
                d->gain = targetGain;
        }
        const auto secs = avg.frames/static_cast<double>(d->in.fps);
        if (secs >= option.bufferLengthInSeconds) {
            if (++d->historyIt == d->history.end())
                d->historyIt = d->history.begin();
            *d->historyIt = input;
        } else {
            d->history.push_back(input);
            d->historyIt = --d->history.end();
        }
    }
    AudioScaler<fmt> m_scaler;
    AudioDataBuffer<S, planar> m_src;
    bool m_scaled = false;
};

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
            info.alpha = detail::alpha(t, i);
            info.c1 = info.alpha/(i - t);
            info.c2 = 1.0/log(1.0 + info.alpha);
        }
        return list;
    }
};

static const auto compressInfo = CompressInfo::create();

template<int fmt_dst>
class AudioMixerPost {
    using D = sample_type<fmt_dst>;
public:
    AudioMixerPost(AudioMixer *mixer): d(&mixer->d) { }
    template<class S, bool planar>
    auto process(const AudioDataBuffer<S, planar> &src) -> void
    {
        const int frames = src.frames();
        mp_audio_realloc_min(d->output, frames);
        d->output->samples = frames;
        m_dst.setData(d->output);
        if (m_dst.isEmpty())
            return;
        const double gain = d->gain*d->amp;
        using Helper = AudioSampleHelper<S>;
        auto trans = d->realClip == ClippingMethod::Soft
                ? detail::clip_conv<S, D, ClippingMethod::Soft>
                : detail::clip_conv<S, D, ClippingMethod::Hard>;
        if (d->amp < 1e-8)
            m_dst.fill(0);
        else if (!d->mix) {
            _AudioManipulate([&] (D &out, S in) { out = trans(in*gain); },
                             m_dst, 0, m_dst.frames(), src, 0);
        } else {
            for (int frame = 0; frame<m_dst.frames(); ++frame) {
                const auto dchannels = m_dst.channels(frame);
                const auto schannels = _C(src).channels(frame);
                for (auto it = dchannels.begin(); it != dchannels.end(); ++it) {
                    const int dch = it.channel();
                    const auto spk = d->out.channels.speaker[dch];
                    auto &map = d->ch_man.sources(spk);
                    double v = 0;
                    for (int i=0; i<map.size(); ++i)
                        v += schannels[d->ch_index_src[map[i]]]*gain;
                    if (map.size() > 1) {
                        // ref: http://www.voegler.eu/pub/audio/
                        //      digital-audio-mixing-and-normalization.html
                        v /= Helper::max();
                        const auto &info = detail::compressInfo[map.size()];
                        if (v < 0)
                            v = -log(1.0 - info.c1*v)*info.c2;
                        else
                            v = +log(1.0 + info.c1*v)*info.c2;
                        v *= Helper::max();
                    }
                    *it = trans(v);
                }
            }
        }
    }
    auto setOutput(mp_audio *output) -> void
    { d->output = output; m_dst.setData(output); }
private:
    AudioMixer::Data *d = nullptr;
    mutable AudioDataBuffer<D, AudioFormatTrait<fmt_dst>::IsPlanar> m_dst;
};

}

template<int fmt_src, int fmt_dst>
class AudioMixerImpl : public AudioMixer {
public:
    using S = sample_type<fmt_src>;
    using D = sample_type<fmt_dst>;
    AudioMixerImpl(const AudioDataFormat &in, const AudioDataFormat &out)
        : AudioMixer(in, out), m_pre(this), m_post(this) { }
    auto apply(const mp_audio *in) -> void override
    {
        if (!m_pre.process(in))
            return;
        if (m_pre.isScaled())
            m_post.process(m_pre.scaledBuffer());
        else
            m_post.process(m_pre.sourceBuffer());
    }
    auto setOutput(mp_audio *output) -> void override
        { m_post.setOutput(output); }
    auto configured() -> void { m_pre.configure(d.in); }
    auto setScaler(bool on, double scale) -> void override
        { m_pre.setScale(on, scale); }
private:
    detail::AudioMixerPre<fmt_src> m_pre;
    detail::AudioMixerPost<fmt_dst> m_post;
};

/******************************************************************************/

template<int fmt_in>
static auto createImpl(const AudioDataFormat &in,
                       const AudioDataFormat &out) -> AudioMixer*
{
    switch (out.type) {
    case AF_FORMAT_S16:
        return new AudioMixerImpl<fmt_in, AF_FORMAT_S16>(in, out);
    case AF_FORMAT_S32:
        return new AudioMixerImpl<fmt_in, AF_FORMAT_S32>(in, out);
    case AF_FORMAT_FLOAT:
        return new AudioMixerImpl<fmt_in, AF_FORMAT_FLOAT>(in, out);
    case AF_FORMAT_DOUBLE:
        return new AudioMixerImpl<fmt_in, AF_FORMAT_DOUBLE>(in, out);
    case AF_FORMAT_S16P:
        return new AudioMixerImpl<fmt_in, AF_FORMAT_S16P>(in, out);
    case AF_FORMAT_S32P:
        return new AudioMixerImpl<fmt_in, AF_FORMAT_S32P>(in, out);
    case AF_FORMAT_FLOATP:
        return new AudioMixerImpl<fmt_in, AF_FORMAT_FLOATP>(in, out);
    case AF_FORMAT_DOUBLEP:
        return new AudioMixerImpl<fmt_in, AF_FORMAT_DOUBLEP>(in, out);
    default:
        return nullptr;
    }
}

auto AudioMixer::create(const AudioDataFormat &in,
                        const AudioDataFormat &out) -> AudioMixer*
{
    auto make = [&in, &out] () -> AudioMixer * {
        switch (in.type) {
        case AF_FORMAT_S16:
            return createImpl<AF_FORMAT_S16>(in, out);
        case AF_FORMAT_S32:
            return createImpl<AF_FORMAT_S32>(in, out);
        case AF_FORMAT_FLOAT:
            return createImpl<AF_FORMAT_FLOAT>(in, out);
        case AF_FORMAT_DOUBLE:
            return createImpl<AF_FORMAT_DOUBLE>(in, out);
        case AF_FORMAT_S16P:
            return createImpl<AF_FORMAT_S16P>(in, out);
        case AF_FORMAT_S32P:
            return createImpl<AF_FORMAT_S32P>(in, out);
        case AF_FORMAT_FLOATP:
            return createImpl<AF_FORMAT_FLOATP>(in, out);
        case AF_FORMAT_DOUBLEP:
            return createImpl<AF_FORMAT_DOUBLEP>(in, out);
        default:
            return nullptr;
        }
    };
    auto mixer = make();
    Q_ASSERT(mixer != nullptr);
    auto ok = mixer->configure(in, out);
    Q_ASSERT(ok);
    return mixer;
}

auto AudioMixer::configure(const AudioDataFormat &in,
                           const AudioDataFormat &out) -> bool
{
    if (d.in.type != in.type || d.out.type != out.type)
        return false;
    d.in = in; d.out = out;
    for (int i=0; i<out.channels.num; ++i)
        d.ch_index_dst[out.channels.speaker[i]] = i;
    for (int i=0; i<in.channels.num; ++i)
        d.ch_index_src[in.channels.speaker[i]] = i;
    d.mix = !d.ch_man.isIdentity();
    d.updateChmap = !mp_chmap_equals(&in.channels, &out.channels);
    d.updateFormat = in.type != out.type;
    resetNormalizer();
    setClippingMethod(d.clip);
    setChannelLayoutMap(d.map);
    configured();
    return true;
}

auto AudioMixer::setNormalizer(bool on,
                               const AudioNormalizerOption &option) -> void
{
    d.normalizer = on;
    d.normalizerOption = option;
    resetNormalizer();
}

auto AudioMixer::setClippingMethod(ClippingMethod method) -> void
{
    d.realClip = d.clip = method;
    if (d.realClip == ClippingMethod::Auto)
        d.realClip = d.in.type & AF_FORMAT_I ? ClippingMethod::Hard
                                             : ClippingMethod::Soft;
}
