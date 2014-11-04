#ifndef PREFDIALOG_HPP
#define PREFDIALOG_HPP

class Pref;

class PrefDialog : public QDialog {
    Q_OBJECT
public:
    PrefDialog(QWidget *parent = 0);
    ~PrefDialog();
    auto setAudioDeviceList(const QList<QPair<QString, QString>> &devices) -> void;
    auto set(const Pref &pref) -> void;
    auto get(Pref &p) -> void;
signals:
    void applyRequested();
    void resetRequested();
private:
    auto changeEvent(QEvent *event) -> void;
    auto showEvent(QShowEvent *event) -> void;
    struct Data;
    Data *d;
};

#endif // PREFDIALOG_HPP
