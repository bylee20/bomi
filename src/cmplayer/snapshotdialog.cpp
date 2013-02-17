#include "snapshotdialog.hpp"
#include "global.hpp"
#include "subtitlerendereritem.hpp"
#include "videorendereritem.hpp"
#include "subtitledrawer.hpp"
#include "ui_snapshotdialog.h"
#include "info.hpp"

struct SnapshotDialog::Data {
	Ui::SnapshotDialog ui;
	const VideoRendererItem *video = nullptr;
	const SubtitleRendererItem *subtitle = nullptr;
	SubtitleDrawer drawer;
	QImage image, sub;
	QPointF subPos;
	bool hasSubtitle = false;
};

SnapshotDialog::SnapshotDialog(QWidget *parent)
: QDialog(parent), d(new Data) {
	d->ui.setupUi(this);
	connect(d->ui.zoomIn, &QAbstractButton::clicked, d->ui.viewer, &ImageViewer::zoomIn);
	connect(d->ui.zoomOut, SIGNAL(clicked()), d->ui.viewer, SLOT(zoomOut()));
	connect(d->ui.original, SIGNAL(clicked()), d->ui.viewer, SLOT(zoomOriginal()));
	connect(d->ui.take, SIGNAL(clicked()), this, SLOT(take()));
	connect(d->ui.save, SIGNAL(clicked()), this, SLOT(save()));
	connect(d->ui.subtitle, SIGNAL(toggled(bool)), this, SLOT(updateSnapshot(bool)));
//	connect(d->ui.clip, SIGNAL(clicked()), this, SLOT(copyToClipboard()));
}

SnapshotDialog::~SnapshotDialog() {
	delete d;
}

void SnapshotDialog::save() {
	static const Info::ExtList ext = Info::ExtList()
			<< "bmp" << "jpg" << "jpeg" << "png" << "ppm" << "tiff" << "xbm" << "xpm";
	const QString filter = tr("Images") + ' ' + ext.toFilter();
	const QString fileName = _L("cmplayer-snapshot-") % QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss") % _L(".png");
	QString file = _GetSaveFileName(this, tr("Save File"), fileName, filter);
	if (!file.isEmpty()) {
		if (!ext.contains(QFileInfo(file).suffix()))
			file += ".png";
		d->ui.viewer->image().save(file);
	}
}

void SnapshotDialog::copyToClipboard() {
	QApplication::clipboard()->setPixmap(d->ui.viewer->image());
}

void SnapshotDialog::setVideoRenderer(const VideoRendererItem *video) {
	d->video = video;
}

void SnapshotDialog::setSubtitleRenderer(const SubtitleRendererItem *subtitle) {
	d->subtitle = subtitle;
}

void SnapshotDialog::updateSnapshot(bool sub) {
	if (!d->video || d->image.isNull()) {
		d->ui.viewer->setText(tr("Failed in getting a snapshot!"));
	} else {
		QPixmap pixmap = QPixmap::fromImage(d->image);
		if (sub && d->hasSubtitle) {
			QPainter painter(&pixmap);
			painter.drawImage(d->subPos, d->sub);
		}
		d->ui.viewer->setImage(pixmap);
	}
}

void SnapshotDialog::take() {
	if (!d->video || !d->video->hasFrame())
		return;
	d->ui.take->setEnabled(false);
	const QImage frame = d->video->frameImage();
	d->image = QImage(d->video->sizeHint(), QImage::Format_ARGB32_Premultiplied);
	QPainter painter(&d->image);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);
	painter.drawImage(d->video->frameRect(d->image.rect()), frame);
	painter.end();
	d->hasSubtitle = false;
	if (d->subtitle) {
		d->drawer.setText(d->subtitle->text());
		d->drawer.setAlignment(d->subtitle->alignment());
		d->drawer.setMargin(d->subtitle->margin());
		d->drawer.setStyle(d->subtitle->style());
		QImage subtitle; QSize size; QPointF offset;
		if ((d->hasSubtitle = d->drawer.draw(subtitle, size, offset, frame.rect(), d->subtitle->dpr()))) {
			if (d->drawer.style().shadow.enabled) {
				QImage shadow(subtitle.size(), QImage::Format_ARGB32_Premultiplied);

				const auto color = d->drawer.style().shadow.color;
				const int r = color.red(), g = color.green(), b = color.blue();
				const double alpha = color.alphaF();
				for (int x=0; x<shadow.width(); ++x) {
					for (int y=0; y<shadow.height(); ++y)
						shadow.setPixel(x, y, qRgba(r, g, b, alpha*qAlpha(subtitle.pixel(x, y))));
				}
				d->sub = QImage(subtitle.size(), QImage::Format_ARGB32_Premultiplied);
				d->sub.fill(0x0);
				painter.begin(&d->sub);
				painter.drawImage(offset, shadow);
				painter.drawImage(QPoint(0, 0), subtitle);
				painter.end();
			} else {
				d->sub = subtitle;
			}
			d->subPos = d->drawer.pos(d->sub.size(), frame.rect());
		}
	}
	updateSnapshot(d->ui.subtitle->isChecked());
	d->ui.save->setEnabled(!d->image.isNull());
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

QSize ImageViewer::sizeHint() const {
	return d->label->sizeHint() + QSize(5, 5);
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
