#ifndef AUDIOSCALER_HPP
#define AUDIOSCALER_HPP

#include "audiofilter.hpp"

class AudioScaler : public AudioFilter {
public:
    auto setActive(bool active) -> void;
    auto isActive() const -> bool { return m_enabled && m_scale != 1.0; }
    auto setFormat(const AudioBufferFormat &format) -> void;
    auto delay() const -> double override { return m_delay; }
    auto setScale(double scale) -> void final;
    auto run(AudioBufferPtr &in) -> AudioBufferPtr override;
    auto reset() -> void override;
    auto passthrough(const AudioBufferPtr &in) const -> bool override;
private:
    struct Vector {
        auto data() -> float* { return buffer.data(); }
        auto data() const -> const float* { return buffer.data(); }
        auto isEmpty() const -> bool { return frames <= 0; }
        std::vector<float> buffer;
        int frames = 0;
    };
    auto f2s(int frames) const -> int { return frames * m_format.channels().num; }
    auto f2b(int frames) const -> int { return f2s(frames) * sizeof(float); }
    auto best_overlap_frames_offset() -> int;
    auto copy(float *dst, int to, const float *src, int from, int frames) const -> void;
    auto move(float *dst, int to, int from, int frames) const -> void;
    auto expand(Vector &vec, int frames) -> void;
    AudioBufferFormat m_format;
    bool m_enabled = false;
    double m_frames_stride_scaled = 0.0, m_frames_stride_error = 0.0;
    int m_frames_stride = 0, m_frames_queued = 0;
    int m_frames_search = 0, m_frames_standing = 0, m_frames_to_slide = 0;
    Vector m_table_blend, m_table_window;
    Vector m_buf_pre_corr, m_queue, m_overlap;
    double m_delay = 0.0, m_scale = 1.0;
};

#endif // AUDIOSCALER_HPP
