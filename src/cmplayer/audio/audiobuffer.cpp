#include "audiobuffer.hpp"

auto AudioBuffer::fill(float s) -> void
{
    for (auto it = m_begin; it != m_end; ++it)
        *it++ = s;
}

auto AudioBuffer::fill(int frame, float s) -> void
{
    auto p = this->p(frame);
    for (int ch = 0; ch < m_nch; ++ch)
        *p++ = s;
}

auto AudioBuffer::expand(int frames) -> void
{
    m_frames = frames;
    if (!m_allocated || !canPut(frames)) {
        m_allocated = true;
        m_data.resize(int(frames*1.2 + 0.5)*m_nch);
        m_begin = m_data.data();
        m_end = m_begin + samples();
        m_raw.resize(1);
        m_raw[0] = (uchar*)m_data.data();
    }
}

auto AudioBuffer::setForRawData(af_format format, int nch) -> void
{
    m_nch = nch;
    m_bps = af_fmt2bps(format);
    m_raw.resize(af_fmt_is_planar(format) ? nch : 1);
}
