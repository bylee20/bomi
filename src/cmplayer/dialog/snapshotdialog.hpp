#ifndef SNAPSHOTDIALOG_H
#define SNAPSHOTDIALOG_H

#include "stdafx.hpp"

class VideoRendererItem;        class SubtitleRendererItem;

class SnapshotDialog : public QDialog {
    Q_OBJECT
public:
    SnapshotDialog(QWidget *parent = 0);
    ~SnapshotDialog();
    auto setVideoRenderer(const VideoRendererItem *video) -> void;
    auto setSubtitleRenderer(const SubtitleRendererItem *subtitle) -> void;
    auto take() -> void;
private:
    auto updateSnapshot(bool sub) -> void;
    auto updateSubtitleImage() -> void;
    auto redraw() -> void;
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
