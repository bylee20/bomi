#include "audioscaler.hpp"

static constexpr const double m_ms_stride = 60.0;
static constexpr const double m_percent_overlap = 0.20;
static constexpr const double m_ms_search = 14.0;

auto AudioScaler::expand(Vector &vec, int frames) -> void
{
    if ((int)vec.buffer.size() < f2s(frames))
        vec.buffer.resize(f2s(frames) * 1.2 + 1);
    vec.frames = frames;
}

auto AudioScaler::copy(float *dst, int to, const float *src, int from, int frames) const -> void
{
    memcpy(dst + f2s(to), src + f2s(from), f2b(frames));
}

auto AudioScaler::move(float *dst, int to, int from, int frames) const -> void
{
    memmove(dst + f2s(to), dst + f2s(from), f2b(frames));
}

auto AudioScaler::setFormat(const AudioBufferFormat &format) -> void
{
    m_delay = 0.0;
    m_format = format;
    const double frames_per_ms = m_format.fps() / 1000.0;
    m_frames_stride = frames_per_ms * m_ms_stride;
    expand(m_overlap, qMax<int>(0, m_frames_stride * m_percent_overlap));
    m_frames_search = 0;
    if (m_overlap.frames > 1)
        m_frames_search = frames_per_ms * m_ms_search;
    m_frames_standing = m_frames_stride - m_overlap.frames;
    if (m_overlap.isEmpty())
        return;

    float *p = nullptr;
    expand(m_table_blend, m_overlap.frames);
    p = m_table_blend.data();
    for (int i=0; i<m_overlap.frames; ++i) {
        for (int ch = 0; ch < m_format.channels().num; ++ch)
            *p++ = i/(double)m_overlap.frames;
    }

    expand(m_table_window, m_overlap.frames - 1);
    const float t = m_overlap.frames;
    p = m_table_window.data();
    for (int i=1; i<m_overlap.frames; ++i) {
        for (int ch = 0; ch < m_format.channels().num; ++ch)
            *p++ = i*(t - i);
    }

    expand(m_buf_pre_corr, m_overlap.frames);
    expand(m_queue, m_frames_search + m_overlap.frames + m_frames_stride);

    reset();
}

auto AudioScaler::passthrough(const AudioBufferPtr &in) const -> bool
{
    return !isActive() || in->isEmpty();
}

auto AudioScaler::run(AudioBufferPtr &in) -> AudioBufferPtr
{
    m_delay = 0;
    const int frames_in = in->frames();
    if (!isActive() || frames_in <= 0)
        return in;

    auto fill_queue = [&in, this](int frames_offset) -> int
    {
        int frames_in = in->frames() - frames_offset;
        const int offset_unchanged = frames_offset;

        if (m_frames_to_slide > 0) {
            if (m_frames_to_slide < m_frames_queued) {
                const int frames_move = m_frames_queued - m_frames_to_slide;
                move(m_queue.data(), 0, m_frames_to_slide, frames_move);
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
            auto sview = in->view<float>();
            const int left = m_queue.frames - m_frames_queued;
            const int frames_copy = qMin(left, frames_in);
            copy(m_queue.data(), m_frames_queued, sview.begin(), frames_offset, frames_copy);
            m_frames_queued += frames_copy;
            frames_offset += frames_copy;
        }
        return frames_offset - offset_unchanged;
    };

    const int frames_scaled = frames_in / m_frames_stride_scaled + 1;
    const int max_frames_out = frames_scaled * m_frames_stride;
    auto dest = newBuffer(m_format, max_frames_out);
    int frames_offset_in = fill_queue(0);
    int frames_out = 0;
    auto dview = dest->view<float>();

    auto output_overlap = [this, &dview](int pos, int frames_off) -> void
    {
        const int samples = f2s(m_overlap.frames);
        auto dit = dview.begin() + f2s(pos);
        auto bit = _C(m_table_blend).data();
        auto oit = _C(m_overlap).data();
        auto qit = _C(m_queue).data() + f2s(frames_off);
        for (int i = 0; i < samples; ++i, ++oit)
            *dit++ = *oit - (*bit++ * (*oit - *qit++));
    };

    while (m_frames_queued >= m_queue.frames) {
        int frames_off = 0;

        // output stride
        if (m_overlap.frames> 0) {
            if (m_frames_search > 0)
                frames_off = best_overlap_frames_offset();
            output_overlap(frames_out, frames_off);
        }

        copy(dview.begin(), frames_out + m_overlap.frames, m_queue.data(),
             frames_off + m_overlap.frames, m_frames_standing);
        frames_out += m_frames_stride;

        // input stride
        copy(m_overlap.data(), 0, m_queue.data(), frames_off + m_frames_stride, m_overlap.frames);
        const auto target_frames = m_frames_stride_scaled + m_frames_stride_error;
        const int target_frames_integer = target_frames;
        m_frames_stride_error = target_frames - target_frames_integer;
        m_frames_to_slide = target_frames_integer;
        frames_offset_in += fill_queue(frames_offset_in);
    }
    m_delay = (m_frames_queued - m_frames_to_slide)/m_scale/m_format.fps();
    dest->expand(frames_out);
    return dest;
}

auto AudioScaler::setActive(bool active) -> void
{
    if (_Change(m_enabled, active))
        reset();
}

auto AudioScaler::setScale(double scale) -> void
{
    m_scale = scale;
    m_frames_stride_scaled = m_scale * m_frames_stride;
    reset();
}

auto AudioScaler::best_overlap_frames_offset() -> int
{
    const int samples = f2s(m_overlap.frames - 1);
    {
        auto cit = m_buf_pre_corr.data();
        auto wit = m_table_window.data();
        auto oit = m_overlap.data() + f2s(1);
        for (int i = 0; i < samples; ++i)
            *cit++ = *wit++ * *oit++;
    }

    int best_off = 0;
    float best_corr = _Min<qint64>(), corr;
    for (int off = 0; off < m_frames_search; ++off) {
        corr = 0;
        auto cit = _C(m_buf_pre_corr).data();
        auto qit = _C(m_queue).data() + f2s(1 + off);
        for (int i = 0; i < samples; ++i)
            corr += *cit++ * *qit++;
        if (corr > best_corr) {
            best_corr = corr;
            best_off  = off;
        }
    }
    return best_off;
}

auto AudioScaler::reset() -> void
{
    m_frames_stride_error = 0;
    m_frames_to_slide = m_frames_queued = 0;
    std::fill_n(m_overlap.data(), m_overlap.frames, 0);
}
