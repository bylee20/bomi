#ifndef AUDIOEQUALIZERWIDGET_HPP
#define AUDIOEQUALIZERWIDGET_HPP

struct AudioEqualizer;

class AudioEqualizerDialog : public QDialog {
    Q_OBJECT
public:
    AudioEqualizerDialog(QWidget *parent = nullptr);
    ~AudioEqualizerDialog();
    auto setEqualizer(const AudioEqualizer &eq) -> void;
    auto equalizer() const -> AudioEqualizer;
signals:
    void equalizerChanged(const AudioEqualizer &eq);
private:
    struct Data;
    Data *d;
};

#endif // AUDIOEQUALIZERWIDGET_HPP
