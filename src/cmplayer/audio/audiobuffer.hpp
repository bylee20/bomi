#ifndef AUDIOBUFFER_HPP
#define AUDIOBUFFER_HPP

extern "C" {
#include <audio/audio.h>
}

struct AudioBufferFormat {
    AudioBufferFormat() { }
    AudioBufferFormat(const mp_audio *mpv)
        : fps(mpv->rate), type((af_format)mpv->format), channels(mpv->channels) { }
    AudioBufferFormat(af_format type, const mp_chmap &ch, int fps)
        : fps(fps), type(type), channels(ch) { }
    auto operator == (const AudioBufferFormat &rhs) const -> bool
    {
        return type == rhs.type && fps == rhs.fps
                && mp_chmap_equals(&channels, &rhs.channels);
    }
    auto operator != (const AudioBufferFormat &rhs) const -> bool
        { return !operator == (rhs); }
    int fps = 0;
    af_format type = AF_FORMAT_UNKNOWN;
    mp_chmap channels;
};

class AudioBuffer {
public:
    AudioBuffer(int nch = 0) { m_nch = nch; }
    AudioBuffer(const AudioBufferFormat &format, int frames)
        { m_nch = format.channels.num; expand(frames); }
    auto p(int frame = 0) -> float* { return get(frame); }
    auto p(int frame = 0) const -> const float* { return get(frame); }
    auto samples() const -> int { return m_frames*m_nch; }
    auto expand(int frames) -> void;
    auto expand(int frames, int nch) -> void { m_nch = nch; expand(frames); }
    auto channels() const -> int { return m_nch; }
    auto frames() const -> int { return m_frames; }
    auto isEmpty() const -> bool { return m_nch <= 0 || m_frames <= 0; }
    auto fill(float s) -> void;
    auto fill(int frame, float s) -> void;
    auto begin() const -> const float* { return m_begin; }
    auto end() const -> const float* { return m_end; }
    auto begin() ->  float* { return m_begin; }
    auto end() -> float* { return m_end; }
    auto copy(int to, const AudioBuffer &from, int begin, int count) -> void
        { memcpy(p(to), from.p(begin), count * m_nch * sizeof(float)); }
    auto move(int to, const AudioBuffer &from, int begin, int count) -> void
        { memmove(p(to), from.p(begin), count * m_nch * sizeof(float)); }
    auto setForRawData(af_format format, int nch) -> void;
    auto setRawFrames(int frames) -> void { m_frames = frames; }
    auto setRawData(uchar *p, int plane = 0) -> void { m_raw[plane] = p; }
    auto plane(int ch) const -> uchar* { return m_raw[ch]; }
    auto bps() const -> int { return m_bps; }
    auto planes() const -> int { return m_raw.size(); }
    auto rawData() const -> uchar*const * { return m_raw.data(); }
    auto rawData() -> uchar** { return m_raw.data(); }
private:
    auto get(int frame) const -> float* { return m_begin + frame*m_nch; }
    auto canPut(int frames) const -> bool { return frames * m_nch <= (int)m_data.size(); }
    bool m_allocated = false;
    std::vector<float> m_data;
    int m_frames = 0, m_nch = 0;
    float *m_begin = nullptr, *m_end = nullptr;
    int m_bps = 4; // bytes per sample
    std::vector<uchar*> m_raw;
};

#endif // AUDIOBUFFER_HPP
