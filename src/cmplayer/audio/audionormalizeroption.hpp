#ifndef AUDIONORMALIZEROPTION_HPP
#define AUDIONORMALIZEROPTION_HPP

struct AudioNormalizerOption {
    auto operator == (const AudioNormalizerOption &rhs) const -> bool
    {
        return silenceLevel == rhs.silenceLevel
               && targetLevel == rhs.targetLevel
               && minimumGain == rhs.minimumGain
               && maximumGain == rhs.maximumGain
               && bufferLengthInSeconds == rhs.bufferLengthInSeconds;
    }
    auto operator != (const AudioNormalizerOption &rhs) const -> bool
        { return !operator==(rhs); }
    auto gain(double level) const -> double
    {
        const auto lv = qBound(minimumGain, targetLevel / level, maximumGain);
        return (level > silenceLevel) ? lv : -1.0;
    }
    double silenceLevel = 0.0001, minimumGain = 0.1, maximumGain = 10.0;
    double targetLevel = 0.07, bufferLengthInSeconds = 5.0;
};

#endif // AUDIONORMALIZEROPTION_HPP
