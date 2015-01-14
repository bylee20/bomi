#ifndef AUDIOCONVERTER_HPP
#define AUDIOCONVERTER_HPP

#include "audiobuffer.hpp"

class AudioConverter
{
public:
    auto setFormat(const AudioBufferFormat &format) -> void;
    auto run(AudioBuffer *in) -> AudioBuffer*;
    auto format() const -> const AudioBufferFormat& { return m_format; }
private:
    AudioBufferFormat m_format;
    AudioBuffer m_dst;
    std::vector<uchar> m_data;
    using Convert = auto (*)(uchar *dst, float src) -> void;
    Convert m_convert = nullptr;
};

#endif // AUDIOCONVERTER_HPP
