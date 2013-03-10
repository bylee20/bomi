#ifndef SNAPSHOTDIALOG_H
#define SNAPSHOTDIALOG_H

#include "stdafx.hpp"

class VideoRendererItem;		class SubtitleRendererItem;

class SnapshotDialog : public QDialog {
	Q_OBJECT
public:
	SnapshotDialog(QWidget *parent = 0);
	~SnapshotDialog();
	void setVideoRenderer(const VideoRendererItem *video);
	void setSubtitleRenderer(const SubtitleRendererItem *subtitle);
public slots:
	void take();
private slots:
	void updateSnapshot(bool sub);
private:
	struct Data;
	Data *d;
};

class ImageViewer : public QScrollArea {
	Q_OBJECT
public:
	ImageViewer(QWidget *parent = 0);
	~ImageViewer();
	QSize sizeHint() const;
	void setText(const QString &text);
	void setImage(const QPixmap &image);
	void scale(double factor);
	QPixmap image() const;
signals:
	void scaleChanged(double scale);
private:
	void zoomOriginal();
	void adjustScrollBar(QScrollBar *scrollBar, double factor);
	struct Data;
	Data *d;
};

#endif
