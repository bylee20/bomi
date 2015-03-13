#ifndef AUDIONORMALIZEROPTION_HPP
#define AUDIONORMALIZEROPTION_HPP

struct AudioDevice {
    QString name;
    QString description;
};

struct AudioNormalizerOption {
    DECL_EQ(AudioNormalizerOption, &T::use_rms, &T::smoothing, &T::max, &T::target, &T::chunk_sec)
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
    static auto default_() -> AudioNormalizerOption;
    bool use_rms = false; int smoothing = 15;
    double max = 10.0, target = 0.95, chunk_sec = 0.5;
};

class AudioNormalizerOptionWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(AudioNormalizerOption value READ option WRITE setOption NOTIFY optionChanged)
public:
    AudioNormalizerOptionWidget(QWidget *parent = nullptr);
    ~AudioNormalizerOptionWidget();
    auto option() const -> AudioNormalizerOption;
    auto setOption(const AudioNormalizerOption &option) -> void;
signals:
    void optionChanged();
private:
    struct Data;
    Data *d;
};

Q_DECLARE_METATYPE(AudioNormalizerOption);

#endif // AUDIONORMALIZEROPTION_HPP
