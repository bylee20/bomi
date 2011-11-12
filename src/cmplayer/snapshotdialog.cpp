#include "snapshotdialog.hpp"
#include "global.hpp"
#include "subtitlerenderer.hpp"
#include "videorenderer.hpp"
#include "ui_snapshotdialog.h"
#include "info.hpp"
#include <QtCore/QDebug>
#include <QtCore/QDateTime>
#include <QtCore/QFileInfo>
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtGui/QLabel>
#include <QtGui/QScrollBar>
#include <QtGui/QApplication>
#include <QtGui/QFileDialog>
#include <QtGui/QClipboard>

struct SnapshotDialog::Data {
	Ui::SnapshotDialog ui;
	const VideoRenderer *video;
	const SubtitleRenderer *subtitle;
	TextOsdRenderer *osd;
	RichString text;
	QImage frame;
	QSize size;
	QRectF rect;
};

SnapshotDialog::SnapshotDialog(QWidget *parent)
: QDialog(parent), d(new Data) {
	d->ui.setupUi(this);
	d->video = 0;
	d->subtitle = 0;
	d->osd = new TextOsdRenderer;
	connect(d->ui.zoomIn, SIGNAL(clicked()), d->ui.viewer, SLOT(zoomIn()));
	connect(d->ui.zoomOut, SIGNAL(clicked()), d->ui.viewer, SLOT(zoomOut()));
	connect(d->ui.original, SIGNAL(clicked()), d->ui.viewer, SLOT(zoomOriginal()));
	connect(d->ui.take, SIGNAL(clicked()), this, SLOT(take()));
	connect(d->ui.save, SIGNAL(clicked()), this, SLOT(save()));
	connect(d->ui.subtitle, SIGNAL(toggled(bool)), this, SLOT(updateSnapshot(bool)));
// 	connect(d->ui.clip, SIGNAL(clicked()), this, SLOT(copyToClipboard()));
}

SnapshotDialog::~SnapshotDialog() {
	delete d->osd;
	delete d;
}

void SnapshotDialog::save() {
	static const Info::ExtList ext = Info::ExtList()
			<< "bmp" << "jpg" << "jpeg" << "png" << "ppm" << "tiff" << "xbm" << "xpm";
	const QString filter = tr("Images") + ' ' + ext.toFilter();

	QString fileName = QLatin1String("cmplayer-snapshot-");
	fileName += QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss");
	fileName += QLatin1String(".png");
	QString file = getSaveFileName(this, tr("Save File"), fileName, filter);
	if (file.isEmpty())
		return;
	if (!ext.contains(QFileInfo(file).suffix()))
		file += ".png";
	d->ui.viewer->image().save(file);
}

void SnapshotDialog::copyToClipboard() {
	QApplication::clipboard()->setPixmap(d->ui.viewer->image());
}

void SnapshotDialog::setVideoRenderer(const VideoRenderer *video) {
	d->video = video;
}

void SnapshotDialog::setSubtitleRenderer(const SubtitleRenderer *subtitle) {
	d->subtitle = subtitle;
}

void SnapshotDialog::updateSnapshot(bool sub) {
	if (d->frame.isNull()) {
		d->ui.viewer->setText(tr("Failed in getting a snapshot!"));
	} else {
		QPixmap pixmap(d->size);
		QPainter painter(&pixmap);
		painter.setRenderHint(QPainter::SmoothPixmapTransform);
		painter.drawImage(d->rect, d->frame);
		if (sub) {
			d->osd->setBackgroundSize(pixmap.size(), pixmap.size());
			d->osd->showText(d->text);
			d->osd->render(&painter, d->osd->posHint());
		}
		d->ui.viewer->setImage(pixmap);
	}
}

void SnapshotDialog::take() {
	if (!d->video || !d->video->hasFrame())
		return;
	d->ui.take->setEnabled(false);
	d->text = d->subtitle->osd()->text();
	d->frame = d->video->frameImage();
	d->osd->setStyle(d->subtitle->osd()->style());
	d->osd->setAlignment(d->subtitle->osd()->alignment());
	const double aspect = d->video->targetAspectRatio();
	QSizeF size(aspect, 1.0);
	size.scale(d->frame.size(), Qt::KeepAspectRatioByExpanding);
	d->rect = QRectF(0, 0, size.width(), size.height());
	QSizeF crop(d->video->targetCropRatio(aspect), 1.0);
	crop.scale(size, Qt::KeepAspectRatio);
	const double x = (crop.width() - size.width())*0.5;
	const double y = (crop.height() - size.height())*0.5;
	d->rect.moveTo(x, y);
	d->size = crop.toSize();
	updateSnapshot(d->ui.subtitle->isChecked());
	d->ui.save->setEnabled(!d->frame.isNull());
	d->ui.take->setEnabled(true);
}

struct ImageViewer::Data {
	QLabel *label;
	double scale;
};

ImageViewer::ImageViewer(QWidget *parent)
: QScrollArea(parent), d(new Data) {
	d->scale = 1.0;
	d->label = new QLabel(this);
	d->label->setBackgroundRole(QPalette::Base);
	d->label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	d->label->setScaledContents(true);
	d->label->setMinimumSize(200, 150);
	d->label->setAlignment(Qt::AlignCenter);
	
	setBackgroundRole(QPalette::Dark);
	setWidget(d->label);
}

ImageViewer::~ImageViewer() {
	delete d;
}

void ImageViewer::setText(const QString &text) {
	d->label->setText(text);
}

void ImageViewer::setImage(const QPixmap &image) {
	d->label->setPixmap(image);
	zoomOriginal();
}

void ImageViewer::zoomOriginal() {
	d->label->adjustSize();
	d->scale = 1.0;
	emit scaleChanged(d->scale);
}

void ImageViewer::scale(double factor) {
	if (qAbs(1.0 - factor) < 1.0e-5)
		zoomOriginal();
	else if (d->label->pixmap()) {
		d->scale *= factor;
		d->label->resize(d->scale * d->label->pixmap()->size());
		adjustScrollBar(horizontalScrollBar(), factor);
		adjustScrollBar(verticalScrollBar(), factor);
		emit scaleChanged(d->scale);
	}
}

void ImageViewer::adjustScrollBar(QScrollBar *scrollBar, double factor) {
	scrollBar->setValue(int(factor * scrollBar->value() + ((factor - 1) * scrollBar->pageStep()/2)));
}

QPixmap ImageViewer::image() const {
	return d->label->pixmap() ? *d->label->pixmap() : QPixmap();
}
