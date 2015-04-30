#ifndef ENCODERDIALOG_HPP
#define ENCODERDIALOG_HPP

class FileNameGenerator;                class OsdStyle;
class StreamTrack;

class EncoderDialog : public QDialog {
    Q_OBJECT
public:
    EncoderDialog(QWidget *parent = nullptr);
    ~EncoderDialog();
    auto isBusy() const -> bool;
    auto setSource(const QByteArray &mrl, const QSize &size, const FileNameGenerator &g) -> void;
    auto setRange(int start, int end) -> void;
    auto setAudio(const StreamTrack &audio) -> void;
    auto setSubtitle(const StreamTrack &sub, const OsdStyle &style) -> void;
    auto start() -> bool;
    auto cancel() -> void;
    auto cropArea() const -> QRect;
signals:
    void visibleChanged(bool visible);
    void cropAreaChanged(const QRect &rect);
private:
    auto updateCropArea() -> void;
    auto showEvent(QShowEvent *event) -> void final;
    auto hideEvent(QHideEvent *event) -> void final;
    auto closeEvent(QCloseEvent *event) -> void final;
    auto customEvent(QEvent *event) -> void final;
    auto run() -> QString;
    struct Data;
    Data *d;
};

#endif // ENCODERDIALOG_HPP
