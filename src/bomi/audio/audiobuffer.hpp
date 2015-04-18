#ifndef AUDIOBUFFER_HPP
#define AUDIOBUFFER_HPP

extern "C" {
#include <audio/audio.h>
}

#ifdef bool
#undef bool
#endif

class AudioBufferFormat {
public:
    AudioBufferFormat() { memset(&m_audio, 0, sizeof(m_audio)); }
    AudioBufferFormat(const mp_audio *mpv): m_audio(*mpv) { }
    AudioBufferFormat(af_format type, const mp_chmap &ch, int fps)
        :AudioBufferFormat()
    {
        mp_audio_set_format(&m_audio, type);
        mp_audio_set_channels(&m_audio, &ch);
        m_audio.rate = fps;
    }
    auto operator == (const AudioBufferFormat &rhs) const -> bool
    {
        return type() == rhs.type() && fps() == rhs.fps()
                && mp_chmap_equals(&channels(), &rhs.channels());
    }
    auto operator != (const AudioBufferFormat &rhs) const -> bool
        { return !operator == (rhs); }
    auto fps() const -> int { return m_audio.rate; }
    auto channels() const -> const mp_chmap& { return m_audio.channels; }
    auto type() const -> af_format { return (af_format)m_audio.format; }
    auto planes() const -> int { return m_audio.num_planes; }
    auto mpAudio() const -> const mp_audio& { return m_audio; }
    auto toSeconds(int frames) const -> double { return double(frames) / fps(); }
    auto secToFrames(double s) const -> int { return fps() * s; }
private:
    mp_audio m_audio;
};

class AudioBuffer;
using AudioBufferPtr = QSharedPointer<AudioBuffer>;

template<class T>
class AudioBufferConstView;

template<class T>
class AudioBufferView;

class AudioBuffer {
public:
    ~AudioBuffer() { talloc_free(m_audio); }
    auto expand(int frames) -> void;
    auto isWritable() const -> bool { return m_writable; }
    auto take() -> mp_audio* { auto p = m_audio; m_audio = nullptr; return p; }
    auto detach() -> void { if (!m_writable) mp_audio_make_writeable(m_audio); }
    auto type() const -> af_format { return (af_format)m_audio->format; }
    auto samples() const -> int { return frames() * channels(); }
    auto frames() const -> int { return m_audio->samples; }
    auto channels() const -> int { return m_audio->channels.num; }
    auto isEmpty() const -> bool { return !m_audio || frames() <= 0; }
    auto planes() const -> int { return m_audio->num_planes; }
    auto bps() const -> int { return m_audio->bps; }
    auto fps() const -> int { return m_audio->rate; }
    auto seconds() const -> double { return double(m_audio->samples) / m_audio->rate; }
    auto fstride() const -> int { return m_audio->sstride; }
    auto pstride() const -> int { return fstride() * frames(); }
    auto isPlanar() const -> bool { return af_fmt_is_planar(m_audio->format); }
    auto data() const -> const uchar** { return (const uchar**)m_audio->planes; }
    auto constData() const -> const uchar** { return data(); }
    auto data() -> uchar** { detach(); return (uchar**)m_audio->planes; }
    template<class T>
    auto view() -> AudioBufferView<T>;
    template<class T>
    auto view() const -> AudioBufferConstView<T> { return constView<T>(); }
    template<class T>
    auto constView() const -> AudioBufferConstView<T>;
    static auto fromMpAudio(mp_audio *mp) -> AudioBufferPtr;
private:
    auto makeEnds() -> void;
    AudioBuffer() { }
    mp_audio *m_audio = nullptr;
    bool m_writable = false;
    std::vector<void*> m_ends;
    template<class T> friend class AudioBufferConstView;
    template<class T> friend class AudioBufferView;
};

template<class T>
class AudioBufferConstView {
public:
    auto operator ->() const -> const AudioBuffer* { return m_buffer; }
    auto plane(int n = 0) const -> const T*
    { return (const T*)m_buffer->m_audio->planes[n]; }
    auto begin(int n = 0) const -> const T* { return plane(n); }
    auto end(int n = 0) const -> const T* { return (const T*)m_buffer->m_ends[n]; }
protected:
    AudioBufferConstView(const AudioBuffer *buffer)
        : m_buffer(buffer) { }
    const AudioBuffer *m_buffer = nullptr;
    friend class AudioBuffer;
};

template<class T>
class AudioBufferView : public AudioBufferConstView<T>
{
public:
    auto plane(int n = 0) -> T* { return (T*)this->m_buffer->m_audio->planes[n]; }
    auto begin(int n = 0) -> T* { return plane(n); }
    auto end(int n = 0) -> T* { return (T*)this->m_buffer->m_ends[n]; }
    auto plane(int n = 0) const -> const T* { return (const T*)this->m_buffer->m_audio->planes[n]; }
    auto begin(int n = 0) const -> const T* { return plane(n); }
    auto end(int n = 0) const -> const T* { return (const T*)this->m_buffer->m_ends[n]; }
private:
    AudioBufferView(AudioBuffer *buffer): AudioBufferConstView<T>(buffer) { }
    friend class AudioBuffer;
};

template<class T>
auto AudioBuffer::view() -> AudioBufferView<T>
{ detach(); return AudioBufferView<T>(this); }

template<class T>
auto AudioBuffer::constView() const -> AudioBufferConstView<T>
{ return AudioBufferConstView<T>(this); }

class Gaussian {
public:
    Gaussian() { setRadius(1); }
    template<class Iter>
    auto apply(Iter begin, Iter end) const -> double
    {
        Q_ASSERT(std::distance(begin, end) == (int)m_weights.size());
        int i = 0; double ret = 0.0;
        for (auto it = begin; it != end; ++it)
            ret += *it * m_weights[i++];
        return ret;
    }
    auto apply(const std::deque<double> &data) const -> double
    {
        return apply(data.begin(), data.end());
    }
    auto setRadius(int radius) -> void
    {
        if (_Change(m_radius, radius))
            m_weights = create(radius);
    }
    auto radius() const -> int { return m_radius; }
    auto size() const -> int { return m_weights.size(); }
    static auto create(int radius) -> std::vector<double>
    {
        const int size = radius * 2 + 1, shift = radius;
        std::vector<double> weights(size);
        const double sigma = radius / 3.0;
        const double c = 2.0 * pow(sigma, 2);
        auto func = [&] (int i) { return exp(-(pow(i - shift, 2) / c)); };
        double sum = 0.0;
        for(int i = 0; i < size; ++i)
            sum += (weights[i] = func(i));
        for(int i = 0; i < size; ++i)
            weights[i] /= sum; // normalize
        return weights;
    }
private:
    int m_radius = 0;
    std::vector<double> m_weights;
};

#endif // AUDIOBUFFER_HPP
