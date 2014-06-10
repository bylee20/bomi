#ifndef SNAPSHOTDIALOG_H
#define SNAPSHOTDIALOG_H

class VideoRendererItem;        class SubtitleRendererItem;

class SnapshotDialog : public QDialog {
    Q_OBJECT
public:
    SnapshotDialog(QWidget *parent = 0);
    ~SnapshotDialog();
    auto take() -> void;
    auto setImage(const QImage &video, const QImage &osd,
                  const QImage &subtitle, const QRectF &subRect) -> void;
    auto clear() -> void;
signals:
    void request();
private:
    struct Data;
    Data *d;
};

class ImageViewer : public QScrollArea {
    Q_OBJECT
public:
    ImageViewer(QWidget *parent = 0);
    ~ImageViewer();
    auto sizeHint() const -> QSize;
    auto setText(const QString &text) -> void;
    auto setImage(const QPixmap &image) -> void;
    auto scale(double factor) -> void;
    auto image() const -> QPixmap;
signals:
    void scaleChanged(double scale);
private:
    auto zoomOriginal() -> void;
    auto adjustScrollBar(QScrollBar *scrollBar, double factor) -> void;
    struct Data;
    Data *d;
};

#endif
