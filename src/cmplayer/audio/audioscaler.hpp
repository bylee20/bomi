#ifndef AUDIOSCALER_HPP
#define AUDIOSCALER_HPP

#include "audiobuffer.hpp"

class AudioScaler {
public:
    auto delay() const -> double { return m_delay; }
    auto setScale(bool on, double scale) -> void;
    auto setFormat(const AudioBufferFormat &format) -> void;
    auto run(AudioBuffer *in) -> AudioBuffer*;
    auto isActive() const -> bool { return m_enabled; }
private:
    auto f2s(int frames) const -> int { return frames * m_format.channels.num; }
    auto f2b(int frames) const -> int { return f2s(frames) * sizeof(float); }
    auto fill_queue(int frames_offset) -> int;
    auto best_overlap_frames_offset() -> int;
    auto output_overlap(int pos, int frames_off) -> void;

    static constexpr const double m_ms_stride = 60.0;
    static constexpr const double m_percent_overlap = 0.20;
    static constexpr const double m_ms_search = 14.0;

    AudioBufferFormat m_format;
    bool m_enabled = false;
    double m_frames_stride_scaled = 0.0, m_frames_stride_error = 0.0;
    int m_frames_stride = 0, m_frames_overlap = 0, m_frames_queued = 0;
    int m_frames_search = 0, m_frames_standing = 0, m_frames_to_slide = 0;

    AudioBuffer m_table_blend, m_table_window;
    AudioBuffer m_buf_pre_corr;
    AudioBuffer m_queue, m_overlap;
    double m_delay = 0.0, m_scale = 1.0;

    const AudioBuffer *m_src = nullptr;
    AudioBuffer m_dst;
};

#endif // AUDIOSCALER_HPP
