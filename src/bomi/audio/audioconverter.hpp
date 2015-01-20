#ifndef AUDIOCONVERTER_HPP
#define AUDIOCONVERTER_HPP

#include "audiofilter.hpp"

class AudioConverter : public AudioFilter {
public:
    auto setFormat(const AudioBufferFormat &format) -> void;
    auto run(AudioBufferPtr &in) -> AudioBufferPtr override;
    auto format() const -> const AudioBufferFormat& { return m_format; }
    auto passthrough(const AudioBufferPtr &in) const -> bool override;
private:
    AudioBufferFormat m_format;
    using Convert = auto (*)(uchar *dst, float src) -> void;
    Convert m_convert = nullptr;
};

#endif // AUDIOCONVERTER_HPP
