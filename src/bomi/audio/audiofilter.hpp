#ifndef AUDIOFILTER_HPP
#define AUDIOFILTER_HPP

#include "audiobuffer.hpp"

class AudioFilter {
public:
    AudioFilter() { }
    virtual ~AudioFilter() { }
    auto setPool(mp_audio_pool *pool) -> void { m_pool = pool; }
    auto newBuffer(const AudioBufferFormat &format, int frames) const -> AudioBufferPtr
    { return AudioBuffer::fromMpAudio(mp_audio_pool_get(m_pool, &format.mpAudio(), frames)); }
    virtual auto setScale(double scale) -> void;
    virtual auto reset() -> void;
    virtual auto delay() const -> double;
    virtual auto passthrough(const AudioBufferPtr &in) const -> bool = 0;
    virtual auto run(AudioBufferPtr &in) -> AudioBufferPtr = 0;
private:
    mp_audio_pool *m_pool = nullptr;
};

#endif // AUDIOFILTER_HPP
