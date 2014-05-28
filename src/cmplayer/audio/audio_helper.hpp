#ifndef AUDIO_HELPER_HPP
#define AUDIO_HELPER_HPP

#include "stdafx.hpp"
#include "audiomixer.hpp"
#include "misc/tmp.hpp"
#include "tmp/type_test.hpp"
#include "tmp/arithmetic_type.hpp"
extern "C" {
#include <audio/format.h>
#include <audio/audio.h>
}

#define SCONST static constexpr
#define SCIA static constexpr inline auto
#define CIA constexpr inline auto

template<int fmt_in>
struct AudioFormatTrait {
    SCONST bool IsInt = (fmt_in & AF_FORMAT_POINT_MASK) == AF_FORMAT_I;
    SCONST bool IsSigned = (fmt_in & AF_FORMAT_SIGN_MASK) == AF_FORMAT_SI;
    SCONST int Bits = (fmt_in & AF_FORMAT_BITS_MASK) == AF_FORMAT_8BIT ? 8 :
                      (fmt_in & AF_FORMAT_BITS_MASK) == AF_FORMAT_16BIT ? 16 :
                      (fmt_in & AF_FORMAT_BITS_MASK) == AF_FORMAT_24BIT ? 24 :
                      (fmt_in & AF_FORMAT_BITS_MASK) == AF_FORMAT_32BIT ? 32 :
                      (fmt_in & AF_FORMAT_BITS_MASK) == AF_FORMAT_64BIT ? 64 :0;
    SCONST int Bytes = Bits/8;
    SCONST bool IsPlanar = fmt_in & AF_FORMAT_PLANAR;
    SCONST ClippingMethod AutoClipping = IsInt ? ClippingMethod::Hard
                                               : ClippingMethod::Soft;
    using SampleType = tmp::conditional_t<IsInt, tmp::integer_t<Bits, IsSigned>,
                                                 tmp::floating_point_t<Bits>>;
};

template<class S>
struct AudioFormatMpv;
template<>
struct AudioFormatMpv<qint8> {
    SCONST int interleaving = AF_FORMAT_S8;
    SCONST int planar = AF_FORMAT_S8;
};
template<>
struct AudioFormatMpv<qint16> {
    SCONST int interleaving = AF_FORMAT_S16;
    SCONST int planar = AF_FORMAT_S16P;
};
template<>
struct AudioFormatMpv<qint32> {
    SCONST int interleaving = AF_FORMAT_S32;
    SCONST int planar = AF_FORMAT_S32P;
};
//template<>
//struct AudioFormatMpv<qint64> {
//    SCONST int interleaving = AF_FORMAT_S64;
//    SCONST int planar = AF_FORMAT_S64P;
//};
template<>
struct AudioFormatMpv<float> {
    SCONST int interleaving = AF_FORMAT_FLOAT;
    SCONST int planar = AF_FORMAT_FLOATP;
};
template<>
struct AudioFormatMpv<double> {
    SCONST int interleaving = AF_FORMAT_DOUBLE;
    SCONST int planar = AF_FORMAT_DOUBLEP;
};


template<class S, bool integer = tmp::is_integral<S>()>
struct AudioSampleHelper;

namespace detail {

template<class T>
SCIA audio_max() -> T { return AudioSampleHelper<T>::max(); }

template<class T>
SCIA softclip(T p) -> tmp::enable_if_t<tmp::is_floating_point<T>(), T>
{ return (p >= M_PI*0.5) ? 1.0 : ((p <= -M_PI*0.5) ? -1.0 : std::sin(p)); }

template<ClippingMethod method, class T, bool integer = std::is_integral<T>::value>
struct Clip;
template<class T, bool integer>
struct Clip<ClippingMethod::Hard, T, integer> {
    SCIA apply (T p) -> T
    { return qBound<T>(-audio_max<T>(), p, audio_max<T>()); }
};
template<class T>
struct Clip<ClippingMethod::Soft, T, true> {
    SCIA apply (T p) -> T
    { return audio_max<T>()*detail::softclip((float)p/audio_max<T>()); }
};
template<class T>
struct Clip<ClippingMethod::Soft, T, false> {
    SCIA apply (T p) -> T { return detail::softclip<T>(p); }
};

template<class T, class U, bool same = tmp::is_same<U, T>()>
struct Conv;
template<class T>
struct Conv<T, T, true> {
    SCIA apply(T p) -> T { return p; }
};
template<class T, class U>
struct Conv<T, U, false> {
    SCIA apply(U p) -> T
    { return (float)p*AudioSampleHelper<T>::max()/AudioSampleHelper<U>::max(); }
};

template<ClippingMethod method, class S>
SCIA clip(S p) -> S { return detail::Clip<method, S>::apply(p); }
template<class S, class T>
SCIA conv(S s) -> T { return detail::Conv<T, S>::apply(s); }

template<class S, class D, ClippingMethod method>
SCIA clip_conv(S s) -> D { return conv<S, D>(clip<method, S>(s)); }

}

template<class ST>
struct AudioSampleHelper<ST, true> {
    using S = ST;
    SCIA max() -> S { return _Max<S>(); }
    SCIA toLevel(S t) -> double { return (double)qAbs(t)/max(); }
    template<int s>
    SCIA rshift(const S &t) -> S { return t >> s; }
};

template<class ST>
struct AudioSampleHelper<ST, false> {
    using S = ST;
    SCIA max() -> S { return 1.0; }
    SCIA toLevel(S t) -> double { return (double)qAbs(t); }
    template<int>
    SCIA rshift(const S &t) -> S { return t; }
};

template<int fmt, bool planar = !!(fmt & AF_FORMAT_PLANAR)>
struct to_interleaving;
template<int fmt>
struct to_interleaving<fmt, false> { SCONST int value = fmt; };
template<>
struct to_interleaving<AF_FORMAT_S16P, true> {
    SCONST int value = AF_FORMAT_S16;
};
template<>
struct to_interleaving<AF_FORMAT_S32P, true> {
    SCONST int value = AF_FORMAT_S32;
};
template<>
struct to_interleaving<AF_FORMAT_FLOATP, true> {
    SCONST int value = AF_FORMAT_FLOAT;
};
template<>
struct to_interleaving<AF_FORMAT_DOUBLEP, true> {
    SCONST int value = AF_FORMAT_DOUBLE;
};

/******************************************************************************/

template<class S, bool IsPlanar> class AudioDataBuffer;
template<class S, bool IsPlanar> class AudioDataRange;

template<class T>
struct AudioDataInfo { int frames = 0, nch = 0; std::vector<T*> planes; };

template<class T, bool planar>
class AudioDataBufferIterator;

template<class T>
class AudioDataBufferIterator<T, true> {
    using Data = const AudioDataInfo<typename std::remove_const<T>::type>;
public:
    using value_type = T;
    AudioDataBufferIterator() {}
    auto operator == (const AudioDataBufferIterator &rhs) const -> bool
        { return m_sample == rhs.m_sample; }
    auto operator != (const AudioDataBufferIterator &rhs) const -> bool
        { return m_sample != rhs.m_sample; }
    auto operator*() -> T& { return *m_sample; }
    auto operator*() const -> T { return *m_sample; }
    auto operator ++ (int) const -> AudioDataBufferIterator
        { AudioDataBufferIterator it(*this); return ++it; }
    auto operator ++ () -> AudioDataBufferIterator&
    {
        ++m_sample;
        if (++m_frame == d->frames) {
            m_sample = d->planes[++m_ch];
            m_frame = 0;
        }
        return *this;
    }
private:
    AudioDataBufferIterator(Data *d, int frame, int ch)
        : d(d), m_ch(ch), m_frame(frame), m_sample(get(d, frame, ch)) { }
    static auto get(Data *d, int frame, int ch) -> T*
        { return d->planes[ch] + frame; }
    Data *d = nullptr;
    int m_ch = 0, m_frame = 0;
    T *m_sample = nullptr;
    template<class S, bool IsPlanar> friend class AudioDataBuffer;
    template<class S, bool IsPlanar> friend class AudioDataRange;
};

template<class T>
class AudioDataBufferIterator<T, false> {
    using Data = const AudioDataInfo<typename std::remove_const<T>::type>;
public:
    using value_type = T;
    AudioDataBufferIterator() {}
    auto operator == (const AudioDataBufferIterator &rhs) const -> bool
        { return m_sample == rhs.m_sample; }
    auto operator != (const AudioDataBufferIterator &rhs) const -> bool
        { return m_sample != rhs.m_sample; }
    auto operator*() -> T& { return *m_sample; }
    auto operator*() const -> T { return *m_sample; }
    auto operator ++ () -> AudioDataBufferIterator&
        { ++m_sample; return *this; }
    auto operator ++ (int) const -> AudioDataBufferIterator
        { AudioDataBufferIterator it(*this); return ++it; }
private:
    AudioDataBufferIterator(Data *d, int frame, int ch)
        : m_sample(get(d, frame, ch)) { }
    static auto get(Data *d, int frame, int ch) -> T*
        { return d->planes[0] + frame*d->nch + ch; }
    T *m_sample = nullptr;
    template<class S, bool IsPlanar> friend class AudioDataBuffer;
    template<class S, bool IsPlanar> friend class AudioDataRange;
};

/******************************************************************************/

template<class T, bool planar>
class AudioChannelsOneFrame;

template<class T>
class AudioChannelsOneFrame<T, true> {
    using Data = const AudioDataInfo<typename std::remove_const<T>::type>;
    Data *d = nullptr; int m_frame = 0;
public:
    struct iterator {
        using value_type = T;
        iterator() {}
        auto operator == (const iterator &rhs) const -> bool
            { return m_sample == rhs.m_sample; }
        auto operator != (const iterator &rhs) const -> bool
            { return m_sample != rhs.m_sample; }
        auto operator*() -> T& { return *m_sample; }
        auto operator*() const -> T { return *m_sample; }
        auto operator ++ () -> iterator&
            { m_sample = d->planes[++m_ch] + m_frame; return *this; }
        auto operator ++ (int) const -> iterator
            { iterator it(*this); return ++it; }
        auto channel() const -> int { return m_ch; }
    private:
        iterator(const AudioChannelsOneFrame *p, int ch)
            : d(p->d), m_frame(p->m_frame), m_ch(ch), m_sample(p->get(ch)) { }
        Data *d = nullptr;
        int m_frame = 0, m_ch = 0;
        T *m_sample = nullptr;
        friend class AudioChannelsOneFrame;
    };

    AudioChannelsOneFrame(Data *info, int frame)
        : d(info), m_frame(frame) { }
    auto size() const -> int { return d->nch; }
    auto frame() const -> int { return m_frame; }
    auto operator [] (int ch) -> T& { return *get(ch); }
    auto operator [] (int ch) const -> T { return *get(ch); }
    auto get(int ch = 0) const -> T* { return d->planes[ch] + m_frame; }
    auto begin() const -> iterator { return iterator(this, 0); }
    auto end() const -> iterator { return iterator(this, d->nch); }
};

template<class T>
class AudioChannelsOneFrame<T, false> {
    using Data = const AudioDataInfo<typename std::remove_const<T>::type>;
    Data *d = nullptr; int m_frame = 0;
public:
    struct iterator {
        using value_type = T;
        iterator() {}
        auto operator == (const iterator &rhs) const -> bool
            { return m_sample == rhs.m_sample; }
        auto operator != (const iterator &rhs) const -> bool
            { return m_sample != rhs.m_sample; }
        auto operator * () -> T& { return *m_sample; }
        auto operator*() const -> T { return *m_sample; }
        auto operator ++ () -> iterator& { ++m_sample; ++m_ch; return *this; }
        auto operator ++ (int) const -> iterator
            { iterator it(*this); return ++it; }
        auto channel() const -> int { return m_ch; }
    private:
        iterator(const AudioChannelsOneFrame *p, int ch)
            : m_sample(p->get(ch)), m_ch(ch) { }
        T *m_sample = nullptr; int m_ch = 0;
        friend class AudioChannelsOneFrame;
    };

    AudioChannelsOneFrame(Data *info, int frame)
        : d(info), m_frame(frame) { }
    auto size() const -> int { return d->nch; }
    auto frame() const -> int { return m_frame; }
    auto operator [] (int ch) -> T& { return *get(ch); }
    auto operator [] (int ch) const -> T { return *get(ch); }
    auto get(int ch = 0) const -> T*
        { return d->planes[0] + m_frame*d->nch + ch; }
    auto begin() const -> iterator { return iterator(this, 0); }
    auto end() const -> iterator { return iterator(this, d->nch); }
};

/******************************************************************************/

namespace tmp {
template<class... Args> static inline auto pass(const Args &...) -> void { }
}

namespace detail {

template<class S, bool planar_out, bool planar_in>
struct Increment {
    Increment(int = 0) {}
    template<class... Args>
    auto operator() (S *&output, const Args*&... inputs) const -> void
        { ++output; tmp::pass(++inputs...); }
};

template<class S>
struct Increment<S, false, true> {
    Increment(int nch): nch(nch) {} int nch = 0;
    template<class... Args>
    auto operator() (S *&output, const Args*&... inputs) const -> void
        { output += nch; tmp::pass(++inputs...); }
};

template<class S>
struct Increment<S, true, false> {
    Increment(int nch): nch(nch) {} int nch = 0;
    template<class... Args>
    auto operator() (S *&output, const Args*&... inputs) const -> void
        { ++output; tmp::pass(inputs += nch...); }
};

template<class F, class... Args, int... I>
SIA call(F &&func, const std::tuple<Args...> &tuple,
                        tmp::index_list<I...>) -> void {
    func(*std::get<I>(tuple)...);
}

template<class... Args, int... I>
SIA extract(int ch, const std::tuple<Args...> &args,
                           tmp::index_list<I...>)
-> decltype(std::make_tuple(std::get<I>(args).get(std::get<I+1>(args), ch)...))
{
    return std::make_tuple(std::get<I>(args).get(std::get<I+1>(args), ch)...);
}

template<class F, class S, bool pout, class S1, bool pin, class... Args>
SIA run(const F &func, AudioDataBuffer<S, pout> &output, int begin, int frames,
        const AudioDataBuffer<S1, pin> &in0, const Args&... inputs) -> void
{
    const auto tuple = std::tie(output, begin, in0, inputs...);
    const auto index_for_extraction = tmp::make_tuple_index<2>(tuple);
    const int nch = in0.channels();
    const detail::Increment<S, pout, pin> inc{nch};
    for (int ch=0; ch<nch; ++ch) {
        auto pointers = extract(ch, tuple, index_for_extraction);
        const auto index = tmp::make_tuple_index(pointers);
        for (int i=0; i<frames; ++i, tmp::call_with_tuple(inc, pointers, index))
            call(func, pointers, index);
    }
}

template<class F, class S, class S1, class... Args>
SIA run(const F &func, AudioDataBuffer<S, false> &output, int begin, int frames,
        const AudioDataBuffer<S1, false> &in0, const Args&... inputs) -> void
{
    auto pointers = extract(0, std::tie(output, begin, in0, inputs...),
                            tmp::make_tuple_index<3+sizeof...(Args), 2>());
    const auto for_call = tmp::make_tuple_index(pointers);
    const int samples = frames*output.channels();
    const detail::Increment<S, false, false> inc;
    for (int i=0; i<samples; ++i, tmp::call_with_tuple(inc, pointers, for_call))
        call(func, pointers, for_call);
}

template<class F, class S>
SIA run(const F &func, AudioDataBuffer<S, true> &output,
        int begin, int frames) -> void
{
    for (int ch=0; ch<output.channels(); ++ch) {
        auto p2 = output.get(begin, ch);
        for (int i=0; i<frames; ++i)
            func(*p2++);
    }
}

template<class F, class S>
SIA run(const F &func, AudioDataBuffer<S, false> &output,
        int begin, int frames) -> void
{
    auto p2 = output.get(begin);
    const int samples = frames*output.channels();
    for (int i=0; i<samples; ++i)
        func(*p2++);
}

}

template<class F, class S, bool planar_out, class... Args>
SIA _AudioManipulate(const F &func, AudioDataBuffer<S, planar_out> &output,
                     int begin, int frames, const Args &... args) -> void
    { detail::run(func, output, begin, frames, args...); }
template<class F, class S, bool planar, class... Args>
SIA _AudioManipulate(const F &func, const AudioDataBuffer<S, planar> &output,
                     int begin, int frames, const Args&... inputs) -> void
{
    detail::run(func, const_cast<AudioDataBuffer<S, planar>&>(output),
                begin, frames, inputs...);
}

/******************************************************************************/

template<class S, bool IsPlanar>
class AudioDataBuffer {
    template<class F>
    auto setup(const F &f) -> void;
    using iterator = AudioDataBufferIterator<S, IsPlanar>;
    using const_iterator = AudioDataBufferIterator<const S, IsPlanar>;
public:
    SCONST auto isPlanar() -> bool { return IsPlanar; }
    AudioDataBuffer(int nch = 0) { d.nch = nch; }
    AudioDataBuffer(const mp_audio *data) { setData(data); }
    AudioDataBuffer(const AudioDataFormat &format, int frames)
        { d.nch = format.channels.num; expand(frames, 1); }
    AudioDataBuffer(const AudioDataFormat &format, int frames, S *s);
    auto get(int frame = 0, int ch = 0) -> S*
        { return iterator::get(&d, frame, ch); }
    auto get(int frame = 0, int ch = 0) const -> const S*
        { return const_iterator::get(&d, frame, ch); }
    auto samples() const -> int { return d.frames*d.nch; }
    auto expand(int frames, double over = 1.2) -> bool;
    auto adjust(int frames) -> void
        { Q_ASSERT(frames <= m_capacity); d.frames = frames; }
    auto channels() const -> int { return d.nch; }
    auto frames() const -> int { return d.frames; }
    auto capacity() const -> int { return m_capacity; }
    auto setData(const mp_audio *data) -> void;
    auto setData(int frames, int nch) -> void;
    auto isEmpty() const -> bool { return d.nch <= 0 || d.frames <= 0; }
    auto channels(int frame) -> AudioChannelsOneFrame<S, IsPlanar>
        { return AudioChannelsOneFrame<S, IsPlanar>(&d, frame); }
    auto channels(int frame) const -> AudioChannelsOneFrame<const S, IsPlanar>
        { return AudioChannelsOneFrame<const S, IsPlanar>(&d, frame); }
    auto fill(S s) -> void
        { _AudioManipulate([&] (S &out) { out = s; }, *this, 0, d.frames); }
    auto fill(int frame, S s) -> void
        { _AudioManipulate([&] (S &out) { out = s; }, *this, frame, 1); }
    template<bool planar>
    auto copy(int to, const AudioDataBuffer<S, planar> &from, int begin,
              int count) -> tmp::enable_if_t<IsPlanar && planar, void>;
    template<bool planar>
    auto copy(int to, const AudioDataBuffer<S, planar> &from, int begin,
              int count) -> tmp::enable_if_t<!IsPlanar && !planar, void>;
    template<bool planar>
    auto copy(int to, const AudioDataBuffer<S, planar> &from, int begin,
              int count) -> tmp::enable_if_t<IsPlanar != planar, void>;
    auto move(int to, const AudioDataBuffer<S, IsPlanar> &from, int begin,
              int count) -> void;
private:
    bool m_allocated = false;
    std::vector<S> m_data;
    AudioDataInfo<S> d;
    int m_capacity = 0;
};

template<class S, bool IsPlanar>
template<class F>
inline auto AudioDataBuffer<S, IsPlanar>::setup(const F &f) -> void
{
    if (IsPlanar) {
        d.planes.resize(d.nch+1);
        for (int i=0; i<d.nch; ++i)
            d.planes[i] = f(i);
    } else {
        d.planes.resize(1);
        d.planes[0] = f(0);
    }
}

template<class S, bool IsPlanar>
AudioDataBuffer<S, IsPlanar>::AudioDataBuffer(const AudioDataFormat &format,
                                              int frames, S *s)
{
    d.nch = format.channels.num; m_capacity = d.frames = frames;
    setup([&] (int i) { return s + frames*i; });
}

template<class S, bool IsPlanar>
auto AudioDataBuffer<S, IsPlanar>::expand(int frames, double over) -> bool
{
    if (m_allocated && frames <= m_capacity)
        return false;
    m_allocated = true;
    d.frames = frames;
    m_capacity = frames*over + 0.5;
    m_data.resize(m_capacity*d.nch);
    setup([&] (int i) { return m_data.data() + m_capacity*i; });
    return true;
}

template<class S, bool IsPlanar>
auto AudioDataBuffer<S, IsPlanar>::setData(const mp_audio *data) -> void
{
    d.nch = data->nch;
    m_capacity = d.frames = data->samples;
    setup([&] (int i) { return (S*)data->planes[i]; });
    m_data.clear();
}
template<class S, bool IsPlanar>
auto AudioDataBuffer<S, IsPlanar>::setData(int frames, int nch) -> void
{
    d.frames = frames;
    if (!m_allocated || nch != d.nch || frames > m_capacity) {
        m_allocated = true;
        d.nch = nch;
        _Expand(m_data, frames*nch);
        m_capacity = m_data.size()/nch;
        setup([&] (int i) { return m_data.data() + m_capacity*i; });
    }
}

template<class S, bool IsPlanar>
template<bool planar>
auto AudioDataBuffer<S, IsPlanar>::copy(int to,
                                        const AudioDataBuffer<S, planar> &from,
                                        int begin, int count)
-> tmp::enable_if_t<IsPlanar && planar, void>
{
    for (int i=0; i<d.nch; ++i)
        memcpy(get(to), from.get(begin), count*sizeof(S));
}

template<class S, bool IsPlanar>
template<bool planar>
auto AudioDataBuffer<S, IsPlanar>::copy(int to,
                                        const AudioDataBuffer<S, planar> &from,
                                        int begin, int count)
-> tmp::enable_if_t<!IsPlanar && !planar, void>
{
    memcpy(get(to), from.get(begin), count*d.nch*sizeof(S));
}

template<class S, bool IsPlanar>
template<bool planar>
auto AudioDataBuffer<S, IsPlanar>::copy(int to,
                                        const AudioDataBuffer<S, planar> &from,
                                        int begin, int count)
-> tmp::enable_if_t<IsPlanar != planar, void>
{
    _AudioManipulate([] (S &out, S in) { out = in; },
                     *this, to, count, from, begin);
}

template<class S, bool p>
auto AudioDataBuffer<S, p>::move(int to, const AudioDataBuffer<S, p> &from,
                                 int begin, int count) -> void
{
    if (p) {
        for (int i=0; i<d.nch; ++i)
            memmove(get(to), from.get(begin), count*sizeof(S));
    } else
        memmove(get(to), from.get(begin), count*d.nch*sizeof(S));
}

#endif // AUDIO_HELPER_HPP
