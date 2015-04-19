#ifndef ENCODERDIALOG_HPP
#define ENCODERDIALOG_HPP

class FileNameGenerator;

class EncoderDialog : public QDialog {
public:
    EncoderDialog(QWidget *parent = nullptr);
    ~EncoderDialog();
    auto isBusy() const -> bool;
    auto setSource(const QByteArray &mrl, const QSize &size, const FileNameGenerator &g) -> void;
    auto setRange(int start, int end) -> void;
    auto setAudio(const QByteArray &audio) -> void;
    auto setGenerator(const FileNameGenerator &g) -> void;
    auto start() -> bool;
    auto cancel() -> void;
private:
    auto closeEvent(QCloseEvent *event) -> void final;
    auto customEvent(QEvent *event) -> void final;
    auto run() -> QString;
    struct Data;
    Data *d;
};

#endif // ENCODERDIALOG_HPP
