#ifndef AUDIOEQUALIZERWIDGET_HPP
#define AUDIOEQUALIZERWIDGET_HPP

class AudioEqualizer;

class AudioEqualizerDialog : public QDialog {
    Q_DECLARE_TR_FUNCTIONS(AudioEqualizerDialog)
    using Update = std::function<void(const AudioEqualizer&)>;
public:
    AudioEqualizerDialog(QWidget *parent = nullptr);
    ~AudioEqualizerDialog();
    auto setEqualizer(const AudioEqualizer &eq) -> void;
    auto equalizer() const -> AudioEqualizer;
    auto setUpdateFunc(Update &&func) -> void;
private:
    struct Data;
    Data *d;
};

#endif // AUDIOEQUALIZERWIDGET_HPP
