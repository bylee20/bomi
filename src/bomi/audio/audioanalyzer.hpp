#ifndef AUDIOANALYZER_HPP
#define AUDIOANALYZER_HPP

#include "audionormalizeroption.hpp"
#include "audiofilter.hpp"

class AudioAnalyzer : public AudioFilter {
public:
    class FFT {
    public:
        FFT();
        ~FFT();
        auto push(const AudioBufferPtr &input) -> bool;
        auto pull() -> QList<qreal>;
        auto inputSize() const -> int;
        auto setInputSize(int size) -> void;
    private:
        friend class AudioAnalyzer;
        auto clear() -> void;
        struct Data;
        Data *d;
    };

    AudioAnalyzer();
    ~AudioAnalyzer();
    auto reset() -> void;
    auto setScale(double scale) -> void final;
    auto isNormalizerActive() const -> bool;
    auto setVisualizationActive(bool on) -> void;
    auto setNormalizerActive(bool on) -> void;
    auto setNormalizerOption(const AudioNormalizerOption &opt) -> void;
    auto setFormat(const AudioBufferFormat &format) -> void;
    auto run(AudioBufferPtr &in) -> AudioBufferPtr override;
    auto delay() const -> double;
    auto push(AudioBufferPtr &src) -> void;
    auto pull(bool eof = false) -> AudioBufferPtr;
    auto gain() const -> float;
    auto passthrough(const AudioBufferPtr &in) const -> bool override;
    auto fft() const -> FFT*;
private:
    auto flush() -> AudioBufferPtr;
    struct Data;
    Data *d;
};

#endif // AUDIOANALYZER_HPP
