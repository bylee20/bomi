#ifndef PREFDIALOG_HPP
#define PREFDIALOG_HPP

class Pref;                             struct AudioDevice;

class PrefDialog : public QDialog {
    Q_OBJECT
public:
    PrefDialog(QWidget *parent = 0);
    ~PrefDialog();
    auto setAudioDeviceList(const QList<AudioDevice> &devices) -> void;
    auto set(const Pref *pref) -> void;
    auto get(Pref *p) -> void;
signals:
    void applyRequested();
private slots:
    void checkModified();
private:
    auto changeEvent(QEvent *event) -> void;
    auto showEvent(QShowEvent *event) -> void;
    struct Data;
    Data *d;
};

#endif // PREFDIALOG_HPP
