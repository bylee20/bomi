#ifndef AUDIONORMALIZEROPTION_HPP
#define AUDIONORMALIZEROPTION_HPP

struct AudioDevice {
    QString name;
    QString description;
};

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
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
    static auto default_() -> AudioNormalizerOption;
    double silenceLevel = 0.0001, minimumGain = 0.1, maximumGain = 10.0;
    double targetLevel = 0.07, bufferLengthInSeconds = 5.0;
};

class AudioNormalizerOptionWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(AudioNormalizerOption value READ option WRITE setOption)
public:
    AudioNormalizerOptionWidget(QWidget *parent = nullptr);
    ~AudioNormalizerOptionWidget();
    auto option() const -> AudioNormalizerOption;
    auto setOption(const AudioNormalizerOption &option) -> void;
private:
    struct Data;
    Data *d;
};

Q_DECLARE_METATYPE(AudioNormalizerOption);

#endif // AUDIONORMALIZEROPTION_HPP
