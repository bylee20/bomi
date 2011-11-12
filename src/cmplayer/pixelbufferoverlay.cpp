//#include "pixelbufferoverlay.hpp"
//#include <QtOpenGL/QGLPixelBuffer>
//#include <QtCore/QDebug>

//struct PixelBufferOverlay::Data {
//	QRect bg, video;
//	GLuint texture;
//	QList<OsdRenderer*> osds;
//	QGLPixelBuffer *pbuffer;
//	bool pending;
//};


//PixelBufferOverlay::PixelBufferOverlay(QGLWidget *video)
//: Overlay(video), d(new Data) {
//	d->texture = 0;
//	d->pbuffer = 0;
//	d->pending = false;
//}

//PixelBufferOverlay::~PixelBufferOverlay() {
//	qDeleteAll(d->osds);
//	delete d->pbuffer;
//	delete d;
//}

//qint64 PixelBufferOverlay::addOsd(OsdRenderer *osd) {
//	if (!osd)
//		return -1;
//	d->osds << osd;
//	connect(osd, SIGNAL(needToRerender()), this, SLOT(cache()));
//	connect(osd, SIGNAL(sizeChanged(QSizeF)), this, SLOT(cache()));
//	return d->osds.size() - 1;
//}

//void PixelBufferOverlay::setArea(const QRect &bg, const QRect &video) {
//	if (bg == d->bg && d->video == video)
//		return;
//	const QSize newSize = OsdRenderer::cachedSize(bg.size());
//	d->pending = true;
//	d->bg = bg;
//	d->video = video;

//	if (!d->pbuffer || d->pbuffer->size() != newSize) {
//		delete d->pbuffer;
//		this->video()->makeCurrent();
//		QGLWidget *w = this->video();
//		d->pbuffer = new QGLPixelBuffer(newSize, w->format(), w);
//	}
//	for (int i=0; i<d->osds.size(); ++i)
//		d->osds[i]->setBackgroundSize(d->bg.size(), d->video.size());
//	d->pending = false;
//	cache();
//}


//void PixelBufferOverlay::cache() {
//	if (d->pending || !d->pbuffer)
//		return;

//	d->pbuffer->makeCurrent();
//	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

//	QPainter painter(d->pbuffer);
//	for (int i=0; i<d->osds.size(); ++i) {
//		OsdRenderer *osd = d->osds[i];
//		const QPointF pos = osd->posHint() + (osd->letterboxHint() ? d->bg : d->video).topLeft();
//		osd->render(&painter, pos);

////		QPointF pos = d->osds[i]->posHint();
////		if (pos.y() < d->bg.top())
////			pos.setY(d->bg.top());
////		d->osds[i]->render(&painter, pos);
//	}
//	painter.end();
//	d->pbuffer->updateDynamicTexture(d->texture);
//	d->pbuffer->toImage().save("test.png");
//	video()->update();
//}

//void PixelBufferOverlay::render(QPainter */*painter*/) {
//	if (d->texture != 0 && d->pbuffer)
//		video()->drawTexture(QRectF(d->bg.topLeft(), d->pbuffer->size()), d->texture);
//}
