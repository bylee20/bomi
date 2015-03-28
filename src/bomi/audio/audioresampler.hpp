#ifndef AUDIORESAMPLER_HPP
#define AUDIORESAMPLER_HPP

#include "audiofilter.hpp"

class AudioResampler : public AudioFilter {
public:
    AudioResampler();
    ~AudioResampler();
    auto setFormat(const AudioBufferFormat &in, const AudioBufferFormat &out) -> void;
    auto run(AudioBufferPtr &in) -> AudioBufferPtr override;
    auto setScale(double scale) -> void final;
    auto delay() const -> double override;
    auto reset() -> void override;
    auto passthrough(const AudioBufferPtr &in) const -> bool override;
    static auto canAccept(int format) -> bool;
private:
    auto reconfigure() -> void;
    struct Data;
    Data *d;
};

#endif // AUDIORESAMPLER_HPP
