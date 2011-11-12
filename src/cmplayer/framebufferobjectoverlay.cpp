#include "framebufferobjectoverlay.hpp"
#include <QtCore/QDebug>
#include <QtOpenGL/QGLFramebufferObject>

struct FramebufferObjectOverlay::Data {
	QRect bg, video;
	QList<OsdRenderer*> osds;
	QGLFramebufferObject *fbo;
	bool pending;
};

FramebufferObjectOverlay::FramebufferObjectOverlay(QGLWidget *video)
: Overlay(video), d(new Data) {
	d->pending = false;
	d->fbo = 0;
}

FramebufferObjectOverlay::~FramebufferObjectOverlay() {
	qDeleteAll(d->osds);
	delete d->fbo;
	delete d;
}


qint64 FramebufferObjectOverlay::addOsd(OsdRenderer *osd) {
	if (!osd)
		return -1;
	d->osds << osd;
	connect(osd, SIGNAL(needToRerender()), this, SLOT(cache()));
	connect(osd, SIGNAL(sizeChanged(QSizeF)), this, SLOT(cache()));
	return d->osds.size() - 1;
}

#include <QTime>

void FramebufferObjectOverlay::setArea(const QRect &bg, const QRect &video) {
	if (bg == d->bg && d->video == video)
		return;
	const QSize newSize = OsdRenderer::cachedSize(bg.size());
	d->pending = true;
	d->bg = bg;
	d->video = video;
	if (!d->fbo || d->fbo->size() != newSize) {
		delete d->fbo;
		this->video()->makeCurrent();
		d->fbo = new QGLFramebufferObject(newSize);
	}
	for (int i=0; i<d->osds.size(); ++i)
		d->osds[i]->setBackgroundSize(d->bg.size(), d->video.size());
	d->pending = false;
	cache();
}

void FramebufferObjectOverlay::cache() {
	if (d->pending || !d->fbo)
		return;
	video()->makeCurrent();

	d->fbo->bind();
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	d->fbo->release();

	QPainter painter(d->fbo);
	for (int i=0; i<d->osds.size(); ++i) {
		OsdRenderer *osd = d->osds[i];
		const QPointF pos = osd->posHint() + (osd->letterboxHint() ? d->bg : d->video).topLeft();
		osd->render(&painter, pos);
	}
	painter.end();
	video()->update();
}

void FramebufferObjectOverlay::render(QPainter */*painter*/) {
	if (d->fbo)
		video()->drawTexture(QRectF(d->bg.topLeft(), d->fbo->size()), d->fbo->texture());
}

QList<OsdRenderer*> FramebufferObjectOverlay::takeOsds() {
	QList<OsdRenderer*> osds = d->osds;
	d->osds.clear();
	return osds;
}
