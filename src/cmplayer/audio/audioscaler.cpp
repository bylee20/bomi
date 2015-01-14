#include "audioscaler.hpp"

auto AudioScaler::run(AudioBuffer *in) -> AudioBuffer*
{
    m_delay = 0;
    m_src = in;
    const int frames_in = in->frames();
    if (!m_enabled || frames_in <= 0)
        return in;
    const int frames_scaled = frames_in / m_frames_stride_scaled + 1;
    const int max_frames_out = frames_scaled * m_frames_stride;
    m_dst.expand(max_frames_out);
    int frames_offset_in = fill_queue(0);
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
        frames_offset_in += fill_queue(frames_offset_in);
    }
    m_delay = (m_frames_queued - m_frames_to_slide)/m_scale/m_format.fps;
    m_dst.expand(frames_out);
    return &m_dst;
}

auto AudioScaler::setScale(bool on, double scale) -> void
{
    m_scale = scale;
    m_frames_stride_error = 0;
    m_frames_stride_scaled = m_scale * m_frames_stride;
    m_frames_to_slide = m_frames_queued = 0;
    m_overlap.fill(0);
    m_enabled = on && scale != 1.0;
}

auto AudioScaler::setFormat(const AudioBufferFormat &format) -> void
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
    m_overlap.expand(m_frames_overlap, nch);
    if (m_overlap.isEmpty())
        return;

    m_table_blend.expand(m_frames_overlap, nch);
    for (int i=0; i<m_frames_overlap; ++i)
        m_table_blend.fill(i, i/(double)m_frames_overlap);

    m_table_window.expand(m_frames_overlap - 1, nch);
    const float t = m_frames_overlap;
    for (int i=1; i<m_frames_overlap; ++i)
        m_table_window.fill(i-1, (i*(t - i)));

    m_buf_pre_corr.expand(m_frames_overlap, nch);
    const int frames = m_frames_search + m_frames_overlap + m_frames_stride;
    m_queue.expand(frames, nch);

    m_dst = {nch};
}

auto AudioScaler::fill_queue(int frames_offset) -> int
{
    int frames_in = m_src->frames() - frames_offset;
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
        m_queue.copy(m_frames_queued, *m_src, frames_offset, frames_copy);
        m_frames_queued += frames_copy;
        frames_offset += frames_copy;
    }
    return frames_offset - offset_unchanged;
}

auto AudioScaler::best_overlap_frames_offset() -> int
{
    const int samples = f2s(m_overlap.frames() - 1);
    {
        auto cit = m_buf_pre_corr.p();
        auto wit = m_table_window.p();
        auto oit = m_overlap.p(1);
        for (int i = 0; i < samples; ++i)
            *cit++ = *wit++ * *oit++;
    }

    int best_off = 0;
    float best_corr = _Min<qint64>(), corr;
    for (int off = 0; off < m_frames_search; ++off) {
        corr = 0;
        auto cit = _C(m_buf_pre_corr).p();
        auto qit = _C(m_queue).p(1 + off);
        for (int i = 0; i < samples; ++i)
            corr += *cit++ * *qit++;
        if (corr > best_corr) {
            best_corr = corr;
            best_off  = off;
        }
    }
    return best_off;
}

auto AudioScaler::output_overlap(int pos, int frames_off) -> void
{
    const int samples = m_overlap.samples();
    auto dit = m_dst.p(pos);
    auto bit = _C(m_table_blend).p();
    auto oit = _C(m_overlap).p();
    auto qit = _C(m_queue).p(frames_off);
    for (int i = 0; i < samples; ++i, ++oit)
        *dit++ = *oit - (*bit++ * (*oit - *qit++));
}
