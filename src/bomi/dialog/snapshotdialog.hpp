#ifndef SNAPSHOTDIALOG_H
#define SNAPSHOTDIALOG_H

#include <QRunnable>

class VideoRenderer;        class SubtitleRenderer;

class SnapshotSaver : public QRunnable {
public:
    SnapshotSaver(const QImage &image, const QString &fileName, int quality);
    auto isWritable() const -> bool { return m_writable; }
private:
    const QImage m_image;
    const QString m_fileName;
    const int m_quality;
    bool m_writable = false;
    auto run() -> void final;
};

class SnapshotDialog : public QDialog {
    Q_DECLARE_TR_FUNCTIONS(SnapshotDialog)
    using Take = std::function<void(void)>;
public:
    SnapshotDialog(QWidget *parent = 0);
    ~SnapshotDialog();
    auto take() -> void;
    auto setImage(const QImage &video, const QImage &osd) -> void;
    auto clear() -> void;
    auto setTakeFunc(Take &&func) -> void;
private:
    struct Data;
    Data *d;
};

#include <QScrollArea>

class ImageViewer : public QScrollArea {
public:
    ImageViewer(QWidget *parent = 0);
    ~ImageViewer();
    auto sizeHint() const -> QSize;
    auto setText(const QString &text) -> void;
    auto setImage(const QPixmap &image) -> void;
    auto scale(double factor) -> void;
    auto image() const -> QPixmap;
private:
    auto zoomOriginal() -> void;
    auto adjustScrollBar(QScrollBar *scrollBar, double factor) -> void;
    struct Data;
    Data *d;
};

#endif
