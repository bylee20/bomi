#ifndef AUDIOFILTER_HPP
#define AUDIOFILTER_HPP

#include "audiobuffer.hpp"

class AudioFilter {
public:
    AudioFilter() { }
    virtual ~AudioFilter() { }
    auto setPool(mp_audio_pool *pool) -> void { m_pool = pool; }
    auto newBuffer(const AudioBufferFormat &format, int frames) -> AudioBufferPtr
    { return AudioBuffer::fromMpAudio(mp_audio_pool_get(m_pool, &format.mpAudio(), frames)); }
private:
    mp_audio_pool *m_pool = nullptr;
};

#endif // AUDIOFILTER_HPP
