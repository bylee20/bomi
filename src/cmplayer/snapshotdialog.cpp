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
	connect(d->ui.zoomIn, &QAbstractButton::clicked, [this] () { d->ui.viewer->scale(1.25); });
	connect(d->ui.zoomOut, &QAbstractButton::clicked, [this] () { d->ui.viewer->scale(0.8); });
	connect(d->ui.original, &QAbstractButton::clicked, [this] () { d->ui.viewer->scale(1.0); });
	connect(d->ui.take, &QAbstractButton::clicked, this, &SnapshotDialog::take);
	connect(d->ui.subtitle, &QAbstractButton::toggled, this, &SnapshotDialog::updateSnapshot);
	connect(d->ui.clip, &QAbstractButton::clicked, [this] () { qApp->clipboard()->setPixmap(d->ui.viewer->image()); });
	connect(d->ui.save, &QAbstractButton::clicked, [this] () {
		const auto ext = Info::writableImageExt();
		const QString filter = tr("Images") + ' ' + ext.toFilter();
		const QString fileName = _L("cmplayer-snapshot-") % QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss") % _L(".png");
		QString file = _GetSaveFileName(this, tr("Save File"), fileName, filter);
		if (!file.isEmpty()) {
			if (!ext.contains(QFileInfo(file).suffix()))
				file += ".png";
			d->ui.viewer->image().save(file);
		}
	});
}

SnapshotDialog::~SnapshotDialog() {
	delete d;
}

void SnapshotDialog::setVideoRenderer(const VideoRendererItem *video) {
	if (d->video)
		disconnect(d->video, 0, this, 0);
	if ((d->video = video))
		connect(d->video, SIGNAL(frameImageObtained(QImage)), this, SLOT(onFrameImageObtained(QImage)), Qt::QueuedConnection);
}

void SnapshotDialog::updateSubtitleImage() {
	d->hasSubtitle = false;
	if (!d->subtitle)
		return;
	d->drawer.setText(d->subtitle->text());
	d->drawer.setAlignment(d->subtitle->alignment());
	d->drawer.setMargin(d->subtitle->margin());
	d->drawer.setStyle(d->subtitle->style());
	QImage subtitle; QSize size; QPointF offset;
	if (!(d->hasSubtitle = d->drawer.draw(subtitle, size, offset, d->image.rect(), d->subtitle->dpr())))
		return;
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
		QPainter painter(&d->sub);
		painter.drawImage(offset, shadow);
		painter.drawImage(QPoint(0, 0), subtitle);
	} else
		d->sub = subtitle;
	d->subPos = d->drawer.pos(d->sub.size(), d->image.rect());
}

void SnapshotDialog::onFrameImageObtained(const QImage &image) {
	const QImage frame = image;
	if (frame.size() == d->video->sizeHint())
		d->image = frame;
	else {
		d->image = QImage(d->video->sizeHint(), QImage::Format_ARGB32_Premultiplied);
		QPainter painter(&d->image);
		painter.setRenderHint(QPainter::SmoothPixmapTransform);
		painter.drawImage(d->video->frameRect(d->image.rect()), frame);
	}
	updateSubtitleImage();
	updateSnapshot(d->ui.subtitle->isChecked());
	d->ui.take->setEnabled(true);
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
	d->ui.save->setEnabled(!d->image.isNull());
}

void SnapshotDialog::take() {
	if (!d->video || !d->video->hasFrame())
		return;
	d->ui.take->setEnabled(false);
	d->video->requestFrameImage();
	return;
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
