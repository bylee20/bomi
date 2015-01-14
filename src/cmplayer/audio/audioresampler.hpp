#ifndef AUDIORESAMPLER_HPP
#define AUDIORESAMPLER_HPP

#include "audiobuffer.hpp"

class AudioResampler
{
public:
    AudioResampler();
    ~AudioResampler();
    auto setFormat(const AudioBufferFormat &in, const AudioBufferFormat &out) -> void;
    auto run(AudioBuffer *in) -> AudioBuffer*;
    auto delay() const -> double;
private:
    struct Data;
    Data *d;
};

#endif // AUDIORESAMPLER_HPP
