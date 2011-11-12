#include "pixmapoverlay.hpp"
#include <QtCore/QDebug>
#include <QtGui/QPainter>
#include "osdrenderer.hpp"
#include <QtOpenGL/QGLWidget>
#include <QtGui/QPixmap>
#include <QtCore/QMap>

struct PixmapOverlay::Data {
	QRect bg, video;
	QMap<OsdRenderer*, QPixmap> caches;
	bool pending;
};

PixmapOverlay::PixmapOverlay(QGLWidget *video)
: Overlay(video), d(new Data) {
}

PixmapOverlay::~PixmapOverlay() {
	QMap<OsdRenderer*, QPixmap>::iterator it = d->caches.begin();
	for (; it != d->caches.end(); ++it)
		delete it.key();
	delete d;
}

qint64 PixmapOverlay::addOsd(OsdRenderer *osd) {
	if (!osd)
		return -1;
	d->caches[osd] = QPixmap();
	connect(osd, SIGNAL(needToRerender()), this, SLOT(cache()));
	connect(osd, SIGNAL(sizeChanged(QSizeF)), this, SLOT(cache()));
	return (qint64)osd;
}

void PixmapOverlay::setArea(const QRect &bg, const QRect &video) {
	if (d->bg == bg && d->video == video)
		return;
	d->bg = bg;
	d->video = video;
	QMap<OsdRenderer*, QPixmap>::iterator it = d->caches.begin();
	for (; it != d->caches.end(); ++it)
		it.key()->setBackgroundSize(d->bg.size(), video.size());
}

void PixmapOverlay::cache() {
	OsdRenderer *osd = qobject_cast<OsdRenderer*>(sender());
	if (!osd)
		return;
	QPixmap &pix = d->caches[osd];
	if (osd->hasCached()) {
		pix = QPixmap();
	} else {
		const QSize newSize = cachedSize(osd->size().toSize());
		if (pix.size() != newSize)
			pix = QPixmap(newSize);
		if (!pix.isNull()) {
			pix.fill(Qt::transparent);
			QPainter painter(&pix);
			painter.fillRect(pix.rect(), Qt::transparent);
			osd->render(&painter, QPointF(0.0, 0.0));
			painter.end();
		}
	}
}

void PixmapOverlay::render(QPainter *painter) {
	QMap<OsdRenderer*, QPixmap>::const_iterator it = d->caches.begin();
	for (; it != d->caches.end(); ++it) {
		const OsdRenderer *osd = it.key();
		const QPointF pos = osd->posHint() + (osd->letterboxHint() ? d->bg : d->video).topLeft();
		if (it->isNull())
			it.key()->render(painter, pos);
		else
			painter->drawPixmap(pos, *it);
	}
}

QList<OsdRenderer*> PixmapOverlay::takeOsds() {
	QList<OsdRenderer*> ret;
	QMap<OsdRenderer*, QPixmap>::const_iterator it = d->caches.begin();
	for (; it != d->caches.end(); ++it) {
		ret.append(it.key());
	}
	d->caches.clear();
	return ret;
}
