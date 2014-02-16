#include "audio_helper.hpp"

// these classes are kept for test
#if 0

template<typename T, bool planar>
class AudioDataRange {
	using Data = const AudioDataInfo<typename std::remove_const<T>::type>;
public:
	AudioDataRange(Data *d, int begin, int frames) {
		Q_ASSERT(frames <= d->frames);
		this->d.frames = frames;
		this->d.nch = d->nch;
		this->d.planes.resize(d->planes.size());
		if (planar) {
			for (int i=0; i<d->planes.size(); ++i)
				this->d.planes[i] = iterator::get(d, begin, i);
		} else
			this->d.planes[0] = iterator::get(d, begin, 0);
	}
	int samples() const { return d.nch*d.frames; }
	int frames() const { return d.frames; }
	int channels() const { return d.nch; }
	using iterator = AudioDataBufferIterator<T, planar>;
	iterator begin() const { return iterator(&d, 0, 0); }
	iterator end() const { return iterator(&d, d.frames, d.nch); }
private:
	Data d;
};

template<typename T, bool isPlanar> class AudioFramesOneChannel;

template<typename T>
class AudioFramesOneChannel<T, true> {
	using Data = const AudioDataInfo<typename std::remove_const<T>::type>;
	Data *d = nullptr; int m_ch = 0;
public:
	AudioFramesOneChannel(Data *info, int ch): d(info), m_ch(ch) { }
	int size() const { return d->frames; }
	int channel() const { return m_ch; }
	T &operator [] (int frame) { return *get(frame); }
	T operator [] (int frame) const { return *get(frame); }
	T *get(int frame = 0) const { return d->planes[m_ch] + frame; }
	struct iterator {
		using value_type = T;
		iterator() {}
		bool operator == (const iterator &rhs) const { return m_sample == rhs.m_sample; }
		bool operator != (const iterator &rhs) const { return m_sample != rhs.m_sample; }
		T& operator*() { return *m_sample; }
		T operator*() const { return *m_sample; }
		iterator &operator ++ () { ++m_sample; return *this; }
		iterator operator ++ (int) const { iterator it(*this); return ++it; }
	private:
		iterator(T *t): m_sample(t) {}
		T *m_sample = nullptr;
	};
	iterator begin() const { return iterator(get(0)); }
	iterator end() const { return iterator(get(d->frames)); }
};

template<typename T>
class AudioFramesOneChannel<T, false> {
	using Data = const AudioDataInfo<typename std::remove_const<T>::type>;
	Data *d = nullptr; int m_ch = 0;
public:
	AudioFramesOneChannel(const Data *info, int ch): d(info), m_ch(ch) { }
	int size() const { return d->frames; }
	int channel() const { return m_ch; }
	T &operator [] (int frame) { return *get(frame); }
	T operator [] (int frame) const { return *get(frame); }
	T *get(int frame = 0) const { return d->planes[0] + d->nch*frame + m_ch; }
	struct iterator {
		using value_type = T;
		iterator() {}
		bool operator == (const iterator &rhs) const { return m_sample == rhs.m_sample; }
		bool operator != (const iterator &rhs) const { return m_sample != rhs.m_sample; }
		T& operator*() { return *m_sample; }
		T operator*() const { return *m_sample; }
		iterator &operator ++ () { m_sample += m_nch; return *this; }
		iterator operator ++ (int) const { iterator it(*this); return ++it; }
	private:
		iterator(T *t, int nch): m_sample(t), m_nch(nch) {}
		T *m_sample = nullptr; int m_nch = 0;
	};
	iterator begin() const { return iterator(get(0), d->nch); }
	iterator end() const { return iterator(get(d->frames), d->nch); }
};

#endif

#if 0



#endif

