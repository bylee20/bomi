#include "audiobuffer.hpp"

auto AudioBuffer::expand(int frames) -> void
{
    if (this->frames() == frames)
        return;
    detach();
    if (frames > m_audio->samples)
        mp_audio_realloc_min(m_audio, frames);
    m_audio->samples = frames;
    makeEnds();
}

auto AudioBuffer::makeEnds() -> void
{
    const int bytes = pstride();
    m_ends.resize(planes());
    for (int i = 0; i < planes(); ++i)
        m_ends[i] = (uchar*)m_audio->planes[i] + bytes;
}

auto AudioBuffer::fromMpAudio(mp_audio *mp) -> AudioBufferPtr
{
    auto buffer = new AudioBuffer;
    buffer->m_audio = mp;
    buffer->m_writable = mp_audio_is_writeable(mp);
    buffer->makeEnds();
    return AudioBufferPtr(buffer);
}
