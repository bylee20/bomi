#ifndef AUDIOANALYZER_HPP
#define AUDIOANALYZER_HPP

#include "audionormalizeroption.hpp"
#include "audiofilter.hpp"

class AudioAnalyzer : public AudioFilter {
    struct LevelInfo {
        LevelInfo(int frames = 0): frames(frames) { }
        int frames = 0; double level = 0.0;
    };
public:
    auto resetNormalizer() -> void
        { m_gain = 1.0; m_history.clear(); m_historyIt = m_history.end(); }
    auto isNormalizerActive() const -> bool { return m_normalizerActive; }
    auto setNormalizerActive(bool on) -> void { m_normalizerActive = on; resetNormalizer(); }
    auto setNormalizerOption(const AudioNormalizerOption &opt)
        { m_normalizerOption = opt; resetNormalizer(); }
    auto setFormat(const AudioBufferFormat &format) -> void;
    auto run(AudioBufferPtr in) -> AudioBufferPtr;
    auto gain() const -> float { return m_gain; }
private:
    auto average(const LevelInfo &add) const -> LevelInfo;
    AudioNormalizerOption m_normalizerOption;
    bool m_normalizerActive = false;
    std::vector<LevelInfo> m_history;
    std::vector<LevelInfo>::iterator m_historyIt;
    float m_gain = 1.0;
    int m_fps = 0;
};

#endif // AUDIOANALYZER_HPP
