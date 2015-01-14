#ifndef AUDIOCONVERTER_HPP
#define AUDIOCONVERTER_HPP

#include "audiofilter.hpp"

class AudioConverter : public AudioFilter {
public:
    auto setFormat(const AudioBufferFormat &format) -> void;
    auto run(AudioBufferPtr in) -> AudioBufferPtr;
    auto format() const -> const AudioBufferFormat& { return m_format; }
private:
    AudioBufferFormat m_format;
    using Convert = auto (*)(uchar *dst, float src) -> void;
    Convert m_convert = nullptr;
};

#endif // AUDIOCONVERTER_HPP
