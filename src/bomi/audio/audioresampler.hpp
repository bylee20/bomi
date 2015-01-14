#ifndef AUDIORESAMPLER_HPP
#define AUDIORESAMPLER_HPP

#include "audiofilter.hpp"

class AudioResampler : public AudioFilter {
public:
    AudioResampler();
    ~AudioResampler();
    auto setFormat(const AudioBufferFormat &in, const AudioBufferFormat &out) -> void;
    auto run(AudioBufferPtr in) -> AudioBufferPtr;
    auto delay() const -> double;
    auto reset() -> void override;
private:
    struct Data;
    Data *d;
};

#endif // AUDIORESAMPLER_HPP
