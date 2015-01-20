#include "audioanalyzer.hpp"

auto AudioAnalyzer::setFormat(const AudioBufferFormat &format) -> void
{
    m_fps = format.fps();
    resetNormalizer();
}

auto AudioAnalyzer::passthrough(const AudioBufferPtr &in) const -> bool
{
    return !m_normalizerActive || in->isEmpty();
}

auto AudioAnalyzer::run(AudioBufferPtr &in) -> AudioBufferPtr
{
    if (!m_normalizerActive)
        return in;
    const int frames = in->frames();
    LevelInfo input(frames);
    auto sview = in->constView<float>();
    for (auto it = sview.begin(); it != sview.end(); ++it)
        input.level += qAbs(*it);
    input.level /= in->samples();
    const auto avg = this->average(input);
    const double targetGain = m_normalizerOption.gain(avg.level);
    if (targetGain < 0)
        m_gain = 1.0;
    else {
        const double rate = targetGain/m_gain;
        if (rate > 1.05) {
            m_gain *= 1.05;
        } else if (rate < 0.95)
            m_gain *= 0.95;
        else
            m_gain = targetGain;
    }
    const auto secs = avg.frames/static_cast<double>(m_fps);
    if (secs >= m_normalizerOption.bufferLengthInSeconds) {
        if (++m_historyIt == m_history.end())
            m_historyIt = m_history.begin();
        *m_historyIt = input;
    } else {
        m_history.push_back(input);
        m_historyIt = --m_history.end();
    }
    emit gainCalculated(m_gain);
    return in;
}

auto AudioAnalyzer::average(const LevelInfo &add) const -> LevelInfo
{
    LevelInfo total;
    for (const auto &one : m_history) {
        total.level += one.level*one.frames;
        total.frames += one.frames;
    }
    total.level += add.level*add.frames;
    total.frames += add.frames;
    total.level /= total.frames;
    return total;
}
