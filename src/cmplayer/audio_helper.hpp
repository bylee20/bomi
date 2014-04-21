#ifndef AUDIO_HELPER_HPP
#define AUDIO_HELPER_HPP

#include "stdafx.hpp"
#include "tmp.hpp"
#include <tuple>
#include "audiomixer.hpp"
#include <cmath>
extern "C" {
#include <audio/format.h>
#include <audio/audio.h>
}

template<int fmt_in>
struct AudioFormatTrait {
    static constexpr bool IsInt = (fmt_in & AF_FORMAT_POINT_MASK) == AF_FORMAT_I;
    static constexpr bool IsSigned = (fmt_in & AF_FORMAT_SIGN_MASK) == AF_FORMAT_SI;
    static constexpr int Bits = (fmt_in & AF_FORMAT_BITS_MASK) == AF_FORMAT_8BIT ? 8 :
                                (fmt_in & AF_FORMAT_BITS_MASK) == AF_FORMAT_16BIT ? 16 :
                                (fmt_in & AF_FORMAT_BITS_MASK) == AF_FORMAT_24BIT ? 24 :
                                (fmt_in & AF_FORMAT_BITS_MASK) == AF_FORMAT_32BIT ? 32 :
                                (fmt_in & AF_FORMAT_BITS_MASK) == AF_FORMAT_64BIT ? 64 : 0;
    static constexpr int Bytes = Bits/8;
    static constexpr bool IsPlanar = fmt_in & AF_FORMAT_PLANAR;
    using SampleType = typename std::conditional<IsInt, typename tmp::integer<Bits, IsSigned>::type, typename tmp::floating_point<Bits>::type>::type;
    static constexpr ClippingMethod AutoClipping = IsInt ? ClippingMethod::Hard : ClippingMethod::Soft;
};

template<class SampleType>
struct AudioSampleHelper {
    using S = SampleType;
    template<class T = S, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
    static constexpr inline S max() { return _Max<S>(); }
    template<class T = S, typename std::enable_if<!std::is_integral<T>::value, int>::type = 0>
    static constexpr inline S max() { return 1.0; }

    template<class T = S, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
    static constexpr inline double toLevel(T t) { return (double)qAbs(t)/max(); }
    template<class T = S, typename std::enable_if<!std::is_integral<T>::value, int>::type = 0>
    static constexpr inline double toLevel(T t) { return (double)qAbs(t); }

    static constexpr inline S hardclip(S p) { return qBound<S>(-max(), p, max()); }
    template<class T = S, typename std::enable_if<!std::is_integral<T>::value, int>::type = 0>
    static constexpr inline T softclip(T p) { return (p >= M_PI*0.5) ? 1.0 : ((p <= -M_PI*0.5) ? -1.0 : std::sin(p)); }
    template<class T = S, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
    static constexpr inline T softclip(T p) { return max()*softclip<float>((float)p/max()); }

    template<ClippingMethod method, typename T> struct Clip { };
    template<typename T> struct Clip<ClippingMethod::Hard, T> {
        static constexpr inline T apply (T p) { return hardclip(p); }
        constexpr inline T operator() (T p) const { return apply(p); }
    };
    template<typename T> struct Clip<ClippingMethod::Soft, T> {
        static constexpr inline T apply (T p) { return softclip(p); }
        constexpr inline T operator() (T p) const { return apply(p); }
    };
    template<ClippingMethod method>
    static constexpr inline S clip(S p) { return Clip<method, S>::apply(p); }

    template<int s, class T = S, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
    static constexpr inline T rshift(const T &t) { return t >> s; }
    template<int s, class T = S, typename std::enable_if<!std::is_integral<T>::value, int>::type = 0>
    static constexpr inline T rshift(const T &t) { return t; }

    template<class T, class U, bool same = std::is_same<U, T>::value> struct Conv { };
    template<class T> struct Conv<T, T, true> {
        static constexpr inline T apply(T p) { static_assert(std::is_same<T, T>::value, "false"); return p; }
    };
    template<class T, class U> struct Conv<T, U, false> { static constexpr inline T apply(U p) { return (float)p*max<T>()/max<U>(); } };
    template<class T>
    static constexpr inline T conv(S s) { return Conv<T, S>::apply(s); }
};

template<int fmt, bool planar = !!(fmt & AF_FORMAT_PLANAR)> struct to_interleaving;
template<int fmt> struct to_interleaving<fmt, false> { static constexpr int value = fmt; };
template<> struct to_interleaving<AF_FORMAT_S16P, true> { static constexpr int value = AF_FORMAT_S16; };
template<> struct to_interleaving<AF_FORMAT_S32P, true> { static constexpr int value = AF_FORMAT_S32; };
template<> struct to_interleaving<AF_FORMAT_FLOATP, true> { static constexpr int value = AF_FORMAT_FLOAT; };
template<> struct to_interleaving<AF_FORMAT_DOUBLEP, true> { static constexpr int value = AF_FORMAT_DOUBLE; };


template<class S, bool IsPlanar> class AudioDataBuffer;
template<class S, bool IsPlanar> class AudioDataRange;

template<typename T>
struct AudioDataInfo { int frames = 0, nch = 0; QVector<T*> planes; };

template<typename T, bool planar> class AudioDataBufferIterator;

template<typename T>
class AudioDataBufferIterator<T, true> {
    using Data = const AudioDataInfo<typename std::remove_const<T>::type>;
public:
    using value_type = T;
    AudioDataBufferIterator() {}
    bool operator == (const AudioDataBufferIterator &rhs) const { return m_sample == rhs.m_sample; }
    bool operator != (const AudioDataBufferIterator &rhs) const { return m_sample != rhs.m_sample; }
    T& operator*() { return *m_sample; }
    T operator*() const { return *m_sample; }
    AudioDataBufferIterator &operator ++ () {
        ++m_sample; if (++m_frame == d->frames) { m_sample = d->planes[++m_ch]; m_frame = 0; } return *this;
    }
    AudioDataBufferIterator operator ++ (int) const { AudioDataBufferIterator it(*this); return ++it; }
private:
    static T *get(Data *d, int frame, int ch) { return d->planes[ch] + frame; }
    AudioDataBufferIterator(Data *d, int frame, int ch)
    : d(d), m_ch(ch), m_frame(frame), m_sample(get(d, frame, ch)) {}
    Data *d = nullptr; int m_ch = 0, m_frame = 0; T *m_sample = nullptr;
    template<class S, bool IsPlanar> friend class AudioDataBuffer;
    template<class S, bool IsPlanar> friend class AudioDataRange;
};

template<typename T>
class AudioDataBufferIterator<T, false> {
    using Data = const AudioDataInfo<typename std::remove_const<T>::type>;
public:
    using value_type = T;
    AudioDataBufferIterator() {}
    bool operator == (const AudioDataBufferIterator &rhs) const { return m_sample == rhs.m_sample; }
    bool operator != (const AudioDataBufferIterator &rhs) const { return m_sample != rhs.m_sample; }
    T& operator*() { return *m_sample; }
    T operator*() const { return *m_sample; }
    AudioDataBufferIterator &operator ++ () { ++m_sample; return *this; }
    AudioDataBufferIterator operator ++ (int) const { AudioDataBufferIterator it(*this); return ++it; }
private:
    static T *get(Data *d, int frame, int ch) { return d->planes[0] + frame*d->nch + ch; }
    AudioDataBufferIterator(Data *d, int frame, int ch): m_sample(get(d, frame, ch)) {}
    T *m_sample = nullptr;
    template<class S, bool IsPlanar> friend class AudioDataBuffer;
    template<class S, bool IsPlanar> friend class AudioDataRange;
};

template<typename T, bool planar> class AudioChannelsOneFrame;

template<typename T>
class AudioChannelsOneFrame<T, true> {
    using Data = const AudioDataInfo<typename std::remove_const<T>::type>;
    Data *d = nullptr; int m_frame = 0;
public:
    AudioChannelsOneFrame(Data *info, int frame): d(info), m_frame(frame) {}
    int size() const { return d->nch; }
    int frame() const { return m_frame; }
    T &operator [] (int ch) { return *get(ch); }
    T operator [] (int ch) const { return *get(ch); }
    T *get(int ch = 0) const { return d->planes[ch] + m_frame; }
    struct iterator {
        using value_type = T;
        iterator() {}
        bool operator == (const iterator &rhs) const { return m_sample == rhs.m_sample; }
        bool operator != (const iterator &rhs) const { return m_sample != rhs.m_sample; }
        T& operator*() { return *m_sample; }
        T operator*() const { return *m_sample; }
        iterator &operator ++ () { m_sample = d->planes[++m_ch] + m_frame; return *this; }
        iterator operator ++ (int) const { iterator it(*this); return ++it; }
        int channel() const { return m_ch; }
    private:
        iterator(const AudioChannelsOneFrame *p, int ch)
        : d(p->d), m_frame(p->m_frame), m_ch(ch), m_sample(p->get(ch)) {}
        Data *d = nullptr; int m_frame = 0, m_ch = 0; T *m_sample = nullptr;
        friend class AudioChannelsOneFrame;
    };
    iterator begin() const { return iterator(this, 0); }
    iterator end() const { return iterator(this, d->nch); }
};

template<typename T>
class AudioChannelsOneFrame<T, false> {
    using Data = const AudioDataInfo<typename std::remove_const<T>::type>;
    Data *d = nullptr; int m_frame = 0;
public:
    AudioChannelsOneFrame(Data *info, int frame): d(info), m_frame(frame) {}
    int size() const { return d->nch; }
    int frame() const { return m_frame; }
    T &operator [] (int ch) { return *get(ch); }
    T operator [] (int ch) const { return *get(ch); }
    T *get(int ch = 0) const { return d->planes[0] + m_frame*d->nch + ch; }
    struct iterator {
        using value_type = T;
        iterator() {}
        bool operator == (const iterator &rhs) const { return m_sample == rhs.m_sample; }
        bool operator != (const iterator &rhs) const { return m_sample != rhs.m_sample; }
        T& operator*() { return *m_sample; }
        T operator*() const { return *m_sample; }
        iterator &operator ++ () { ++m_sample; ++m_ch; return *this; }
        iterator operator ++ (int) const { iterator it(*this); return ++it; }
        int channel() const { return m_ch; }
    private:
        iterator(const AudioChannelsOneFrame *p, int ch): m_sample(p->get(ch)), m_ch(ch) {}
        T *m_sample = nullptr; int m_ch = 0;
        friend class AudioChannelsOneFrame;
    };
    iterator begin() const { return iterator(this, 0); }
    iterator end() const { return iterator(this, d->nch); }
};

namespace detail {
template<class S, bool planar_out, bool planar_in> struct Increment {
    Increment(int = 0) {}
    template<class... Args>    void operator() (S *&output, const Args*&... inputs) const { ++output; tmp::pass(++inputs...); }
};
template<class S> struct Increment<S, false, true> {
    Increment(int nch): nch(nch) {} int nch = 0;
    template<class... Args> void operator() (S *&output, const Args*&... inputs) const { output += nch; tmp::pass(++inputs...); }
};
template<class S> struct Increment<S, true, false> {
    Increment(int nch): nch(nch) {} int nch = 0;
    template<class... Args> void operator() (S *&output, const Args*&... inputs) const { ++output; tmp::pass(inputs += nch...); }
};

template<class F, class... Args, int... I>
static inline void call(F &&func, const std::tuple<Args...> &tuple, tmp::index_list<I...>) {
    func(*std::get<I>(tuple)...);
}
template<class... Args, int... I>
static inline auto extract(int ch, const std::tuple<Args...> &args, tmp::index_list<I...>)
        -> decltype(std::make_tuple(std::get<I>(args).get(std::get<I+1>(args), ch)...)) {
    return std::make_tuple(std::get<I>(args).get(std::get<I+1>(args), ch)...);
}
template<class F, class S, bool planar_out, class S1, bool planar_in, class... Args>
static inline void run(const F &func, AudioDataBuffer<S, planar_out> &output, int begin, int frames, const AudioDataBuffer<S1, planar_in> &in0, const Args&... inputs) {
    const auto tuple = std::tie(output, begin, in0, inputs...);
    const auto index_for_extraction = tmp::make_tuple_index<2>(tuple);
    const int nch = in0.channels();
    const detail::Increment<S, planar_out, planar_in> inc{nch};
    for (int ch=0; ch<nch; ++ch) {
        auto pointers = extract(ch, tuple, index_for_extraction);
        const auto index = tmp::make_tuple_index(pointers);
        for (int i=0; i<frames; ++i, tmp::call_with_tuple(inc, pointers, index))
            call(func, pointers, index);
    }
}
template<class F, class S, class S1, class... Args>
static inline void run(const F &func, AudioDataBuffer<S, false> &output, int begin, int frames, const AudioDataBuffer<S1, false> &in0, const Args&... inputs) {
    auto pointers = extract(0, std::tie(output, begin, in0, inputs...), tmp::make_tuple_index<3+sizeof...(Args), 2>());
    const auto for_call = tmp::make_tuple_index(pointers);
    const int samples = frames*output.channels();
    const detail::Increment<S, false, false> inc;
    for (int i=0; i<samples; ++i, tmp::call_with_tuple(inc, pointers, for_call))
        call(func, pointers, for_call);
}
template<class F, class S>
static inline void run(const F &func, AudioDataBuffer<S, true> &output, int begin, int frames) {
    for (int ch=0; ch<output.channels(); ++ch) {
        auto p2 = output.get(begin, ch);
        for (int i=0; i<frames; ++i)
            func(*p2++);
    }
}
template<class F, class S>
static inline void run(const F &func, AudioDataBuffer<S, false> &output, int begin, int frames) {
    auto p2 = output.get(begin);
    const int samples = frames*output.channels();
    for (int i=0; i<samples; ++i)
        func(*p2++);
}

}

template<class F, class S, bool planar_out, class... Args>
static inline void _AudioManipulate(const F &func, AudioDataBuffer<S, planar_out> &output, int begin, int frames, const Args &... args) {
    detail::run(func, output, begin, frames, args...);
}
template<class F, class S, bool planar, class... Args>
static inline void _AudioManipulate(const F &func, const AudioDataBuffer<S, planar> &output, int begin, int frames, const Args&... inputs) {
    detail::run(func, const_cast<AudioDataBuffer<S, planar>&>(output), begin, frames, inputs...);
}

template<class S, bool IsPlanar>
class AudioDataBuffer {
    template<typename F>
    void setup(const F &f) {
        if (IsPlanar) { d.planes.resize(d.nch+1); for (int i=0; i<d.nch; ++i) {d.planes[i] = f(i);} }
        else { d.planes.resize(1); d.planes[0] = f(0); }
    }
    typedef AudioDataBufferIterator<S, IsPlanar> iterator;
    typedef AudioDataBufferIterator<const S, IsPlanar> const_iterator;
public:
    static constexpr bool isPlanar() { return IsPlanar; }
    AudioDataBuffer(int nch = 0) { d.nch = nch; }
    AudioDataBuffer(const mp_audio *data) { setData(data); }
    AudioDataBuffer(const AudioDataFormat &format, int frames) {
        d.nch = format.channels.num; expand(frames, 1);
    }
    AudioDataBuffer(const AudioDataFormat &format, int frames, S *s) {
        d.nch = format.channels.num; m_capacity = d.frames = frames;
        setup([&] (int i) { return s + frames*i; });
    }
    S *get(int frame = 0, int ch = 0) { return iterator::get(&d, frame, ch); }
    const S *get(int frame = 0, int ch = 0) const { return const_iterator::get(&d, frame, ch); }
    int samples() const { return d.frames*d.nch; }
    bool expand(int frames, double over = 1.2) {
        if (m_allocated && frames <= m_capacity)
            return false;
        m_allocated = true;
        d.frames = frames;
        m_capacity = frames*over + 0.5;
        m_data.resize(m_capacity*d.nch);
        setup([&] (int i) { return m_data.data() + m_capacity*i; });
        return true;
    }
    void adjust(int frames) { Q_ASSERT(frames <= m_capacity); d.frames = frames; }
    int channels() const { return d.nch; }
    int frames() const { return d.frames; }
    int capacity() const { return m_capacity; }
    void setData(const mp_audio *data) {
        d.nch = data->nch; m_capacity = d.frames = data->samples;
        setup([&] (int i) { return (S*)data->planes[i]; });
        m_data.clear();
    }
    void setData(int frames, int nch) {
        d.frames = frames;
        if (!m_allocated || nch != d.nch || frames > m_capacity) {
            m_allocated = true;
            d.nch = nch;
            _Expand(m_data, frames*nch);
            m_capacity = m_data.size()/nch;
            setup([&] (int i) { return m_data.data() + m_capacity*i; });
        }
    }

    bool isEmpty() const { return d.nch <= 0 || d.frames <= 0; }
    AudioChannelsOneFrame<S, IsPlanar> channels(int frame) { return AudioChannelsOneFrame<S, IsPlanar>(&d, frame); }
    AudioChannelsOneFrame<const S, IsPlanar> channels(int frame) const { return AudioChannelsOneFrame<const S, IsPlanar>(&d, frame); }
    void fill(S s) { _AudioManipulate([&] (S &out) { out = s; }, *this, 0, d.frames); }
    void fill(int frame, S s) { _AudioManipulate([&] (S &out) { out = s; }, *this, frame, 1); }
    template<bool planar>
    typename std::enable_if<IsPlanar && planar>::type
    copy(int to, const AudioDataBuffer<S, planar> &from, int begin, int count) {
        for (int i=0; i<d.nch; ++i)
            memcpy(get(to), from.get(begin), count*sizeof(S));
    }
    template<bool planar>
    typename std::enable_if<!IsPlanar && !planar>::type
    copy(int to, const AudioDataBuffer<S, planar> &from, int begin, int count) {
        memcpy(get(to), from.get(begin), count*d.nch*sizeof(S));
    }
    template<bool planar>
    typename std::enable_if<IsPlanar != planar>::type
    copy(int to, const AudioDataBuffer<S, planar> &from, int begin, int count) {
        _AudioManipulate([] (S &out, S in) { out = in; }, *this, to, count, from, begin);
    }
    void move(int to, const AudioDataBuffer<S, IsPlanar> &from, int begin, int count) {
        if (IsPlanar) {
            for (int i=0; i<d.nch; ++i)
                memmove(get(to), from.get(begin), count*sizeof(S));
        } else
            memmove(get(to), from.get(begin), count*d.nch*sizeof(S));
    }
private:
    bool m_allocated = false;
    QVector<S> m_data;
    AudioDataInfo<S> d;
    int m_capacity = 0;
};

#endif // AUDIO_HELPER_HPP
