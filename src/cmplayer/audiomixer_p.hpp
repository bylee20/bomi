#ifndef AUDIOFILTER_P_HPP
#define AUDIOFILTER_P_HPP

#include "stdafx.hpp"
#include "tmp.hpp"
extern "C" {
#include <audio/format.h>
}

template<int fmt_in>
struct Help {
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
	static constexpr inline T softclip(S p) { return (p >= M_PI*0.5) ? 1.0 : ((p <= -M_PI*0.5) ? -1.0 : std::sin(p)); }
	template<class T = S, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
	static constexpr inline T softclip(S p) { return max()*softclip<float>((float)p/max()); }
	template<class T = S, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
	static constexpr inline T autoclip(S p) { return hardclip(p); }
	template<class T = S, typename std::enable_if<!std::is_integral<T>::value, int>::type = 0>
	static constexpr inline T autoclip(S p) { return softclip<S>(p); }

	template<ClippingMethod method, typename T> struct Clip { };
	template<typename T> struct Clip<ClippingMethod::Auto, T> {
		static constexpr inline T apply (double p) { return autoclip(p); }
		constexpr inline T operator() (double p) const { return apply(p); }
	};
	template<typename T> struct Clip<ClippingMethod::Hard, T> {
		static constexpr inline T apply (double p) { return hardclip(p); }
		constexpr inline T operator() (double p) const { return apply(p); }
	};
	template<typename T> struct Clip<ClippingMethod::Soft, T> {
		static constexpr inline T apply (double p) { return softclip(p); }
		constexpr inline T operator() (double p) const { return apply(p); }
	};
	template<ClippingMethod method>
	static constexpr inline S clip(S p) { return Clip<method, S>::apply(p); }

	template<int s, class T = S, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
	static constexpr inline void rshift(T &t) { t >>= s; }
	template<int s, class T = S, typename std::enable_if<!std::is_integral<T>::value, int>::type = 0>
	static constexpr inline void rshift(T &) { }

	template<class T, class U, bool same = std::is_same<U, T>::value> struct Conv { };
	template<class T> struct Conv<T, T, true> {
		static constexpr inline T apply(T p) { static_assert(std::is_same<T, T>::value, "false"); return p; }
	};
	template<class T, class U> struct Conv<T, U, false> { static constexpr inline T apply(U p) { return (float)p*max<T>()/max<U>(); } };
	template<class T>
	static constexpr inline T conv(S s) { return Conv<T, S>::apply(s); }
};

template<int fmt, bool planar = fmt & AF_FORMAT_PLANAR> struct ToInterleaving { };
template<int fmt> struct ToInterleaving<fmt, false> { static constexpr int value = fmt; };
template<> struct ToInterleaving<AF_FORMAT_S16P, true> { static constexpr int value = AF_FORMAT_S16; };
template<> struct ToInterleaving<AF_FORMAT_S32P, true> { static constexpr int value = AF_FORMAT_S32; };
template<> struct ToInterleaving<AF_FORMAT_FLOATP, true> { static constexpr int value = AF_FORMAT_FLOAT; };
template<> struct ToInterleaving<AF_FORMAT_DOUBLEP, true> { static constexpr int value = AF_FORMAT_DOUBLE; };

template<typename T>
struct AudioFrame {
	AudioFrame() {}
	AudioFrame(int nch, T t = T()): m_samples(nch, t) { }
	int channels() const { return m_samples.size(); }
	T at(int ch) const { return m_samples[ch]; }
	T &operator [] (int ch) { return m_samples[ch]; }
	T operator [] (int ch) const { return m_samples.at(ch); }
	void fill(T t) { std::fill(m_samples.begin(), m_samples.end(), t); }
private:
	QVector<T> m_samples;
};

template<class Buffer>
class AudioFrameIteratorBase {
public:
	AudioFrameIteratorBase(int nch = 0): m_channels(nch) { }
	AudioFrameIteratorBase(const mp_audio *data) {
		m_channels = data->channels.num;
		m_size = data->samples;
	}
	AudioFrameIteratorBase(const AudioFormat &fmt, int frames) {
		m_channels = fmt.channels.num;
		m_size = frames;
	}
	virtual ~AudioFrameIteratorBase() {}
	int pos() const { return m_pos; }
	void start() const { m_pos= 0; m_buffer = m_start; }
	bool ok() const { return m_pos < m_size; }
	int channels() const { return m_channels; }
	int size() const { return m_size; }
	void resize(int size) { m_size = size; }
	bool isEmpty() const { return m_size <= 0; }
	const Buffer &buffer() const { return m_start; }
protected:
	mutable Buffer m_buffer{};
	mutable int m_pos = 0;
	Buffer m_start{};
	int m_channels = 0, m_size = 0;
};

template<int fmt_in, bool planar = Help<fmt_in>::IsPlanar>
class AudioFrameIterator : public AudioFrameIteratorBase<int> {
	using H = Help<fmt_in>;
	using S = typename H::SampleType;
	AudioFrameIterator(int nch = 0);
	AudioFrameIterator(const mp_audio *data);
	AudioFrameIterator(const AudioFormat &fmt, int frames, S *s);
	S &get(int ch);	int pos() const;	void start();	bool ok() const;
	int channels() const;	void next();	int size() const;
	template<typename F>	void for_ch(F f);	void fill(S s);
	void setTo(mp_audio *data);
	void setBuffer(const mp_audio *data);
};

template<int fmt_in>
class AudioFrameIterator<fmt_in, false> : public AudioFrameIteratorBase<typename Help<fmt_in>::SampleType*> {
public:
	using P = AudioFrameIteratorBase<typename Help<fmt_in>::SampleType*>;
	using H = Help<fmt_in>;
	using S = typename H::SampleType;
	AudioFrameIterator(int nch = 0): P(nch) {}
	AudioFrameIterator(const mp_audio *data): P(data) { Q_ASSERT(fmt_in == data->format); setBuffer(data); }
	AudioFrameIterator(const AudioFormat &fmt, int frames, S *s): P(fmt, frames) { P::m_start = s; P::start(); }
	const S &get(int ch) const { return P::m_buffer[ch]; }
	S &get(int ch) { return P::m_buffer[ch]; }
	void next() const { ++P::m_pos; P::m_buffer += P::m_channels; }
	void seek(int pos) const { P::m_pos = pos; P::m_buffer = P::m_start + pos*P::m_channels; }
	template<typename F>
	void for_ch(F f) const { for(P::start(); P::ok(); next()) { for (int ch=0; ch<P::m_channels; ++ch) f(ch); } }
	void fill(S s) { std::fill_n(P::m_start, P::m_size*P::m_channels, s); }
	void setTo(mp_audio *data) { data->planes[0] = P::m_start; data->samples = P::m_size; }
	void setBuffer(const mp_audio *data) { P::m_start = (S*)data->planes[0]; P::m_size = data->samples; P::start(); }
};

template<int fmt_in>
class AudioFrameIterator<fmt_in, true> : public AudioFrameIteratorBase<QVector<typename Help<fmt_in>::SampleType*>> {
public:
	using P = AudioFrameIteratorBase<QVector<typename Help<fmt_in>::SampleType*>>;
	using S = typename Help<fmt_in>::SampleType;
	AudioFrameIterator(int nch = 0): P(nch) { P::m_start.resize(nch); P::start(); }
	AudioFrameIterator(const mp_audio *data): P(data) {
		Q_ASSERT(fmt_in == data->format);
		P::m_start.resize(P::m_channels);
		for (int ch=0; ch<P::m_channels; ++ch) P::m_start[ch] = (S*)data->planes[ch];
		P::start();
	}
	AudioFrameIterator(const AudioFormat &fmt, int frames, S *s): P(fmt, frames) {
		P::m_start.resize(P::m_channels);
		for (int ch=0; ch<P::m_channels; ++ch, s += frames)
			P::m_start[ch] = s;
		P::start();
	}
	void seek(int pos) const { P::m_pos = pos; }
	S &get(int ch) { return P::m_buffer[ch][P::m_pos]; }
	const S &get(int ch) const { return P::m_buffer[ch][P::m_pos]; }
	void next() const { ++P::m_pos; }
	template<typename F>
	void for_ch(F f) const { for(P::start(); P::ok(); next()) { for (int ch=0; ch<P::m_channels; ++ch) f(ch); } }
	void fill(S s) {
		for (int ch=0; ch<P::m_channels; ++ch)
			std::fill_n(P::m_start[ch], P::m_size, s);
	}
	void setTo(mp_audio *data) {
		for (int ch=0; ch<P::m_channels; ++ch)
			data->planes[ch] = P::m_start[ch];
		data->samples = P::m_size;
	}
	void setBuffer(const mp_audio *data) {
		Q_ASSERT(P::m_start.size() == data->nch);
		for (int ch=0; ch<P::m_channels; ++ch) P::m_start[ch] = (S*)data->planes[ch];
		P::m_size = data->samples; P::start();
	}
};

template<typename S>
class AudioFrameBuffer {
public:
	AudioFrameBuffer(int nch = 0): m_nch(nch) { }
	AudioFrameBuffer(int frames, int nch): m_nch(nch) { resize(frames); }
	S *data() { return m_data.data(); }
	const S *data() const { return m_data.data(); }
	int channels() const { return m_nch; }
	S *frame(int i) { return &m_data[m_nch*i]; }
	const S *frame(int i) const { return &m_data[m_nch*i]; }
	int frames() const { return m_frames; }
	int size() const { return m_frames; }
	bool expand(int frames, int nch) { m_frames = frames; m_nch = nch; return _Expand(m_data, frames*nch); }
	void resize(int frames) { m_frames = frames; m_data.resize(samples()); }
	void resize(int frames, int nch) { m_nch = nch; m_frames = frames; m_data.resize(samples()); }
	int samples() const { return m_frames*m_nch; }
	void fill(S s) { m_data.fill(s); }
	void fill(int frame, S s) { std::fill_n(this->frame(frame), m_nch, s); }
	bool isEmpty() const { return samples() <= 0; }
	void copyTo(S *s, int from, int n) const { std::copy_n(frame(from), n*m_nch, s); }
private:
	QVector<S> m_data;
	int m_nch = 0, m_frames = 0;
};

#endif // AUDIOFILTER_P_HPP
