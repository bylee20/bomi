//#include "videorenderer.hpp"
//#include "playengine.hpp"
//#include "overlay.hpp"
//#include "colorproperty.hpp"
//#include "events.hpp"
//#include "videoframe.hpp"
//#include "pref.hpp"
//#include "logodrawer.hpp"
//#include "mpcore.hpp"
//#include "videoshader.hpp"
//#ifdef Q_OS_X11
//#include <X11/Xlib.h>
//#endif
//#ifndef GL_UNPACK_CLIENT_STORAGE_APPLE
//#define GL_UNPACK_CLIENT_STORAGE_APPLE 34226
//#endif

//#ifndef GL_TEXTURE_STORAGE_HINT_APPLE
//#define GL_TEXTURE_STORAGE_HINT_APPLE 34236
//#endif

//#ifndef GL_STORAGE_CACHED_APPLE
//#define GL_STORAGE_CACHED_APPLE 34238
//#endif

//#ifndef GL_TEXTURE_RECTANGLE_ARB
//#define GL_TEXTURE_RECTANGLE_ARB 34037
//#endif

//extern "C" {
//#include <stream/stream_dvdnav.h>
//#include <input/input.h>
//#include <libmpdemux/stheader.h>
//#include <codec-cfg.h>
//#include <libmpcodecs/vd.h>
//void *fast_memcpy(void * to, const void * from, size_t len);
//#undef clamp
//}

//struct MPlayerOsdWrapper : public OsdWrapper {
//	void alloc() {
//		OsdWrapper::alloc();
//		m_shader = new QGLShaderProgram(QGLContext::currentContext());
//		m_shader->addShaderFromSourceCode(QGLShader::Fragment,
//			"uniform sampler2D tex_y, tex_a;"
//			"void main() {"
//			"	float luma = texture2D(tex_y, gl_TexCoord[0].xy).x;"
//			"	float alpha = texture2D(tex_a, gl_TexCoord[0].xy).x;"
//			"	gl_FragColor = vec4(luma, luma, luma, alpha);"
//			"}"
//		);
//		m_shader->link();
//	}
//	void free() {
//		delete m_shader;
//		m_shader = nullptr;
//		OsdWrapper::free();
//	}

//	void draw(int x, int y, int /*w*/, int h, uchar *src, uchar *srca, int stride) {
//		m_pos.rx() = x;
//		m_pos.ry() = y;
//		const int length = h*stride;
//		if ((m_empty = !(length > 0 && count() > 0 && m_shader)))
//			return;
//		m_alpha.resize(length);
//		char *data = m_alpha.data();
//		for (int i=0; i<length; ++i)
//			*data++ = -*srca++;
//		upload(QSize(stride, h), src, 0);
//		upload(QSize(stride, h), m_alpha.data(), 1);
//	}

//	void render() {
//		if (m_empty)
//			return;
//		m_shader->bind();
//		m_shader->setUniformValue("tex_y", 4);
//		m_shader->setUniformValue("tex_a", 5);
//		glActiveTexture(GL_TEXTURE4);
//		glBindTexture(GL_TEXTURE_2D, texture(0));
//		glActiveTexture(GL_TEXTURE5);
//		glBindTexture(GL_TEXTURE_2D, texture(1));
//		glActiveTexture(GL_TEXTURE4);

//		float textureCoords[] = {
//			0.f, 0.f,					sub_x(0), 0.f,
//			sub_x(0), sub_y(0),			0.f, sub_y(0)
//		};

//		const double expand_x = frameVtx.width()/frameSize.width();
//		const double expand_y = frameVtx.height()/frameSize.height();
//		const float x1 = m_pos.x()*expand_x + frameVtx.x();
//		const float y1 = m_pos.y()*expand_y + frameVtx.y();
//		const float x2 = x1 + width(0)*expand_x;
//		const float y2 = y1 + height(0)*expand_y;
//		float vertexCoords[] = {
//			x1, y1,			x2, y1,
//			x2, y2,			x1, y2
//		};

////		glClearColor(0.f, 0.f, 0.f, 0.f);
//		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//		glEnable(GL_BLEND);

//		glEnableClientState(GL_VERTEX_ARRAY);
//		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
//		glTexCoordPointer(2, GL_FLOAT, 0, textureCoords);
//		glVertexPointer(2, GL_FLOAT, 0, vertexCoords);
//		glDrawArrays(GL_QUADS, 0, 4);
//		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
//		glDisableClientState(GL_VERTEX_ARRAY);
//		m_shader->release();
//	}
//	QRectF frameVtx;
//	QSize frameSize;
//private:
//	int _count() const {return 2;}
//	GLint _internalFormat(int) const {return GL_LUMINANCE;}
//	GLenum _format(int) const {return GL_LUMINANCE;}
//	GLenum _type(int) const {return GL_UNSIGNED_BYTE;}
//	QPoint m_pos;
//	QByteArray m_alpha;
//	QGLShaderProgram *m_shader = nullptr;
//};

//struct VideoRenderer::Data {
//	static const int i420ToRgbSimple = 0;
//	static const int i420ToRgbFilter = 1;
//	static const int i420ToRgbKernel = 2;
//	int frameId = 0;
//	int alignment = Qt::AlignCenter;
//	QSize renderSize;
//	LogoDrawer logo;
//	GLuint textures[3];
//	VideoFrame *buffer, *frame, buf[2];
//	QRectF vtx;
//	QPoint offset = {0, 0};
//	VideoFormat format;
//	VideoScreen screen;
//	VideoShader::Var var;
//	PlayEngine *engine;
//	VideoShader *shader = nullptr;
//	double crop = -1.0, aspect = -1.0, dar = 0.0;
//	double cpu = -1.0;
//	bool prepared = false, logoOn = false, frameIsSet = false, binding = false;
//	bool render = false;
//	MPlayerOsdWrapper osd;
//	StreamList streams;
//	QString codec;
//	QSize viewport;
//	bool takeSnapshot = false;
//	bool clientStorage = false;
//	int prevFrameId = -1;
//	quint64 drawnFrames = 0;
//	GLuint vaTexture = GL_NONE;
//};

//QGLWidget *glContext = nullptr;

//VideoRenderer::VideoRenderer(PlayEngine *engine)
//: d(new Data) {
//	d->engine = engine;
//	d->frame = &d->buf[0];
//	d->buffer = &d->buf[1];
//	d->screen.r = this;
//	d->screen.setMinimumSize(QSize(200, 100));
//	d->screen.setAutoFillBackground(false);
//	d->screen.setAttribute(Qt::WA_OpaquePaintEvent, true);
//	d->screen.setAttribute(Qt::WA_NoSystemBackground, true);
////	d->screen.setAttribute(Qt::WA_PaintOutsidePaintEvent, true);
//	d->screen.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
//	d->screen.setMouseTracking(true);
//	d->screen.setContextMenuPolicy(Qt::CustomContextMenu);
//	connect(engine, SIGNAL(aboutToOpen()), this, SLOT(onAboutToOpen()));
//	connect(engine, SIGNAL(aboutToPlay()), this, SLOT(onAboutToPlay()));

//	gl = &d->screen;
//	makeCurrent();
//	glGenTextures(3, d->textures);
//	d->shader = new VideoShader(gl->context());
//	auto exts = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
//	d->clientStorage = strstr(exts, "GL_APPLE_client_storage") && strstr(exts, "GL_APPLE_texture_range");
//	d->osd.alloc();
//	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
//	d->logo.bind(gl);
//	doneCurrent();
//}

//VideoRenderer::~VideoRenderer() {
////	makeCurrent();
////	d->osd.free();
////	delete d->shader;
////	glDeleteTextures(3, d->textures);
////	d->logo.bind(nullptr);
////	doneCurrent();
//	delete d;
//}

//void VideoRenderer::makeCurrent() {
////	XLockDisplay(QX11Info::display());
//	gl->makeCurrent();
//}
//void VideoRenderer::doneCurrent() {
//	gl->doneCurrent();
////	XUnlockDisplay(QX11Info::display());
//}


//void VideoRenderer::drawAlpha(void *p, int x, int y, int w, int h, uchar *src, uchar *srca, int stride) {
//	auto r = reinterpret_cast<VideoRenderer*>(p);
////	r->makeCurrent();
////	r->d->osd.draw(x, y, w, h, src, srca, stride);
////	r->doneCurrent();
//}

//void VideoRenderer::onAboutToPlay() {
//}

//void VideoRenderer::onAboutToOpen() {
//	d->streams.clear();
//	d->dar = 0.0;
//	d->codec.clear();
//}

//bool VideoRenderer::parse(const Id &id) {
//	if (getStream(id, "VIDEO", "VID", d->streams))
//		return true;
//	if (!id.name.isEmpty()) {
//		if (same(id.name, "VIDEO_ASPECT")) {
//			d->dar = id.value.toDouble();
//			return true;
//		}
//	}
//	return false;
//}

//bool VideoRenderer::parse(const QString &/*line*/) {
//	return false;
//}

//StreamList VideoRenderer::streams() const {
//	return d->streams;
//}

//int VideoRenderer::currentStreamId() const {
//	MPContext *mpctx = d->engine->context();
//	if (mpctx && mpctx->sh_video)
//		return mpctx->sh_video->vid;
//	return -1;
//}
//void VideoRenderer::setCurrentStream(int id) {
//	d->engine->tellmp("switch_video", id);
//}

//GLuint VideoRenderer::texture(int idx) const {
//	return d->textures[idx];
//}

//QImage VideoRenderer::frameImage() const {
//	if (!d->frameIsSet)
//		return QImage();
//	return d->frame->toImage();
//}

//void VideoRenderer::takeSnapshot() const {
//	d->takeSnapshot = true;
//	const_cast<VideoRenderer*>(this)->update();
//}

//quint64 VideoRenderer::drawnFrames() const {
//	return d->drawnFrames;
//}

//double VideoRenderer::frameRate() const {
//	return d->engine->hasVideo() ? d->engine->context()->sh_video->fps : 25;
//}

//const VideoFormat &VideoRenderer::format() const {
//	return d->format;
//}

//bool VideoRenderer::hasFrame() const {
//	return (!d->logoOn && d->frameIsSet && d->prepared);
//}

//bool VideoRenderer::beginUploadingTextures() {
//	if (!(d->frameIsSet = !d->format.isEmpty()))
//		return false;
//	auto min = 0, max = 255;
//	const auto effects = d->var.effects();
//	if (!(effects & IgnoreEffect)) {
//		if (effects & RemapLuma) {
//			min = Pref::get().adjust_contrast_min_luma;
//			max = Pref::get().adjust_contrast_max_luma;
//		}
//	}
//	d->var.setYRange((float)min/255.0f, (float)max/255.0f);
//	makeCurrent();
//	return true;
//}

//void VideoRenderer::endUploadingTextures() {
//	++d->frameId;
//	doneCurrent();
//}

//VideoFrame &VideoRenderer::bufferFrame() {
//	return *d->frame;
//	return *d->buffer;
//}

//void VideoRenderer::prepare(const VideoFormat &format) {
//	if (d->format != format) {
//		d->drawnFrames = 0;
//		d->prepared = false;
//		d->var.setYRange(0.0f, 1.0f);
//		d->frame->format = format;
//		d->format = format;
////		emit formatChanged(d->format);
//		makeCurrent();
//		if (d->shader)
//			d->shader->link(format.type);
//		auto bindTex = [this] (int idx) {
//				glBindTexture(GL_TEXTURE_2D, d->textures[idx]);
//				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//		};
//		auto initTex = [this, &bindTex] (int idx, GLenum fmt, int width, int height) {
//			bindTex(idx);
//			glTexImage2D(GL_TEXTURE_2D, 0, fmt, width, height, 0, fmt, GL_UNSIGNED_BYTE, nullptr);
//		};
//		auto initRgbTex = [this, &bindTex] (int idx, GLenum fmt) {
//			bindTex(idx);
//			glTexImage2D(GL_TEXTURE_2D, 0, 4, d->format.width_stride, d->format.height, 0, fmt, GL_UNSIGNED_BYTE, nullptr);
//		};
//		switch (d->format.type) {
//		case VideoFormat::I420:
//		case VideoFormat::YV12:
//			initTex(0, GL_LUMINANCE, d->format.width_stride, d->format.height);
//			initTex(1, GL_LUMINANCE, d->format.width_stride >> 1, d->format.height >> 1);
//			initTex(2, GL_LUMINANCE, d->format.width_stride >> 1, d->format.height >> 1);
//			break;
//		case VideoFormat::NV12:
//		case VideoFormat::NV21:
//			initTex(0, GL_LUMINANCE, d->format.width_stride, d->format.height);
//			initTex(1, GL_LUMINANCE_ALPHA, d->format.width_stride >> 1, d->format.height >> 1);
//			break;
//		case VideoFormat::YUY2:
//		case VideoFormat::UYVY:
//			initTex(0, GL_LUMINANCE_ALPHA, d->format.width_stride, d->format.height);
//			initTex(1, GL_RGBA, d->format.width_stride >> 1, d->format.height);
//			break;
//		case VideoFormat::RGBA:
//			initRgbTex(0, GL_RGBA);
//			break;
//		case VideoFormat::BGRA:
//			initRgbTex(0, GL_BGRA);
//			break;
//		default:
//			break;
//		}
//		doneCurrent();
//		d->osd.frameSize = d->format.size();
//		d->prepared = true;
//		updateSize();
//		emit formatChanged(d->format);
//	}
//	emit prepared();
//}

//int VideoRenderer::outputWidth() const {
//	return d->dar > 0.01 ? (int)(d->dar*(double)d->format.height + 0.5) : d->format.width;
//}

//double VideoRenderer::targetAspectRatio() const {
//	if (d->aspect > 0.0)
//		return d->aspect;
//	if (d->aspect == 0.0)
//		return d->screen.aspectRatio();
//	return d->dar > 0.01 ? d->dar : (double)d->format.width/(double)d->format.height;
//}

//double VideoRenderer::targetCropRatio(double fallback) const {
//	if (d->crop > 0.0)
//		return d->crop;
//	if (d->crop == 0.0)
//		return d->screen.aspectRatio();
//	return fallback;
//}

//QSize VideoRenderer::sizeHint() const {
//	if (d->format.isEmpty())
//		return QSize(400, 300);
//	const double aspect = targetAspectRatio();
//	QSizeF size(aspect, 1.0);
//	size.scale(d->format.size(), Qt::KeepAspectRatioByExpanding);
//	QSizeF crop(targetCropRatio(aspect), 1.0);
//	crop.scale(size, Qt::KeepAspectRatio);
//	return crop.toSize();
//}

//void VideoRenderer::update() {
//	d->screen.update();
////	if (!d->render)
////		d->render = true;
////	PlayEngine::get().enqueue(new PlayEngine::Cmd(PlayEngine::Cmd::VideoUpdate));
//}

//void VideoRenderer::setAspectRatio(double ratio) {
//	if (!isSameRatio(d->aspect, ratio)) {
//		d->aspect = ratio;
//		updateSize();
//		update();
//	}
//}

//double VideoRenderer::aspectRatio() const {
//	return d->aspect;
//}

//void VideoRenderer::setCropRatio(double ratio) {
//	if (!isSameRatio(d->crop, ratio)) {
//		d->crop = ratio;
//		updateSize();
//		update();
//	}
//}

//double VideoRenderer::cropRatio() const {
//	return d->crop;
//}

//void VideoRenderer::setLogoMode(bool on) {
//	d->logoOn = on;
//}

//void VideoRenderer::setColorProperty(const ColorProperty &prop) {
//	d->var.setColor(prop);
//	update();
//}

//const ColorProperty &VideoRenderer::colorProperty() const {
//	return d->var.color();
//}

//void VideoRenderer::setFixedRenderSize(const QSize &size) {
//	if (d->renderSize != size) {
//		d->renderSize = size;
//		updateSize();
//		update();
//	}
//}

//QSize VideoRenderer::renderableSize() const {
//	return d->renderSize.isEmpty() ? d->screen.size() : d->renderSize;
//}

//void VideoRenderer::updateSize() {
//	const QSizeF widget = renderableSize();
//	QRectF vtx(QPointF(0, 0), widget);
//	if (!d->logoOn && d->prepared) {
//		const double aspect = targetAspectRatio();
//		QSizeF frame(aspect, 1.0);
//		QSizeF letter(targetCropRatio(aspect), 1.0);
//		letter.scale(widget, Qt::KeepAspectRatio);
//		frame.scale(letter, Qt::KeepAspectRatioByExpanding);
//		vtx.setLeft((widget.width() - frame.width())*0.5);
//		vtx.setTop((widget.height() - frame.height())*0.5);
//		vtx.setSize(frame);
//	}
//	if (d->vtx != vtx) {
//		d->vtx = vtx;
//		d->osd.frameVtx = d->vtx;
//		d->screen.overlay()->setArea(QRect(QPoint(0, 0), widget.toSize()), d->vtx.toRect());
//	}
//}

//GLuint *VideoRenderer::textures() const {
//	return d->textures;
//}

//void VideoRenderer::render() {
//	return;
//	const QSizeF widget = renderableSize();
//	if (hasFrame() && d->shader) {
//		QSizeF letter(targetCropRatio(targetAspectRatio()), 1.0);
//		letter.scale(widget, Qt::KeepAspectRatio);

//		QPointF offset = d->offset;
//		QPointF xy(widget.width(), widget.height());
//		xy.rx() -= letter.width(); xy.ry() -= letter.height();	xy *= 0.5;
//		if (d->alignment & Qt::AlignLeft)
//			offset.rx() -= xy.x();
//		else if (d->alignment & Qt::AlignRight)
//			offset.rx() += xy.x();
//		if (d->alignment & Qt::AlignTop)
//			offset.ry() -= xy.y();
//		else if (d->alignment & Qt::AlignBottom)
//			offset.ry() += xy.y();
//		xy += offset;
//		double top = 0.0, left = 0.0;
//		double bottom = (double)(d->format.height)/(double)d->format.height;
//		double right = (double)(d->format.width)/(double)(d->format.width_stride);
//		const Effects effects = d->var.effects();
//		if (!(effects & IgnoreEffect)) {
//			if (effects & FlipHorizontally)
//				qSwap(left, right);
//			if (effects & FlipVertically)
//				qSwap(top, bottom);
//		}
//		makeCurrent();
//		if (d->viewport != d->screen.size()) {
//			d->viewport = d->screen.size();
//			glViewport(0, 0, d->viewport.width(), d->viewport.height());
//			glMatrixMode(GL_PROJECTION);
//			glLoadIdentity();
//			glOrtho(0, d->viewport.width(), d->viewport.height(), 0, -1, 1);
//			glMatrixMode(GL_MODELVIEW);
//		}
//		glLoadIdentity();
//		d->shader->bind(d->var, d->format);
////		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
//		const float textureCoords[] = {
//			(float)left,	(float)top,
//			(float)right,	(float)top,
//			(float)right,	(float)bottom,
//			(float)left,	(float)bottom,
//		};
//		const float vertexCoords[] = {
//			(float)(d->vtx.left()+offset.x()),	(float)(d->vtx.top()+offset.y()),
//			(float)(d->vtx.right()+offset.x())+1.f,	(float)(d->vtx.top()+offset.y()),
//			(float)(d->vtx.right()+offset.x())+1.f,	(float)(d->vtx.bottom()+offset.y()),
//			(float)(d->vtx.left()+offset.x()),	(float)(d->vtx.bottom()+offset.y())
//		};
//		glActiveTexture(GL_TEXTURE0);
//		glBindTexture(GL_TEXTURE_2D, d->textures[0]);
//		glActiveTexture(GL_TEXTURE1);
//		glBindTexture(GL_TEXTURE_2D, d->textures[1]);
//		glActiveTexture(GL_TEXTURE2);
//		glBindTexture(GL_TEXTURE_2D, d->textures[2]);
//		glActiveTexture(GL_TEXTURE0);

//		glEnableClientState(GL_VERTEX_ARRAY);
//		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
//		glTexCoordPointer(2, GL_FLOAT, 0, textureCoords);
//		glVertexPointer(2, GL_FLOAT, 0, vertexCoords);
//		glDrawArrays(GL_QUADS, 0, 4);
//		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
//		glDisableClientState(GL_VERTEX_ARRAY);

//		d->shader->release();

//		glColor3f(0.0f, 0.0f, 0.0f);
//		auto fillRect = [] (float x, float y, float w ,float h) {glRectf(x, y, x+w, y+h);};
//		if (0.0 <= xy.y() && xy.y() <= widget.height())
//			fillRect(.0, .0, widget.width(), xy.y());
//		xy.ry() += letter.height();
//		if (0.0 <= xy.y() && xy.y() <= widget.height())
//			fillRect(.0, xy.y(), widget.width(), widget.height()-xy.y());
//		if (0.0 <= xy.x() && xy.x() <= widget.width())
//			fillRect(.0, .0, xy.x(), widget.height());
//		xy.rx() += letter.width();
//		if (0.0 <= xy.x() && xy.x() <= widget.width())
//			fillRect(xy.x(), .0, widget.width()-xy.x(), widget.height());

//		d->screen.overlay()->renderToScreen();
//		d->osd.render();
//		gl->swapBuffers();
//		doneCurrent();
//		if (d->prevFrameId != d->frameId) {
//			++d->drawnFrames;
//			d->prevFrameId = d->frameId;
//		}
//		if (d->takeSnapshot) {
//			emit tookSnapshot(frameImage());
//			d->takeSnapshot = false;
//		}
//	} else {
//		makeCurrent();
//		QPainter painter(&d->screen);
//		d->logo.draw(&painter, QRectF(QPointF(0, 0), widget));
//		painter.beginNativePainting();
//		d->screen.overlay()->renderToScreen();
//		painter.endNativePainting();
//		doneCurrent();
//	}
//	if (d->render)
//		d->render = false;
//}

//bool VideoRenderer::needsToBeRendered() const {
//	return d->render;
//}

//void VideoRenderer::clearEffects() {
//	d->var.setEffects(0);
//	update();
//}

//void VideoRenderer::setEffect(Effect effect, bool on) {
//	setEffects(on ? (d->var.effects() | effect) : (d->var.effects() & ~effect));
//}

//void VideoRenderer::setEffects(Effects effects) {
//	if (d->var.effects() != effects) {
//		d->var.setEffects(effects);
//		update();
//	}
//}

//void VideoRenderer::setOffset(const QPoint &offset) {
//	if (d->offset != offset) {
//		d->offset = offset;
//		update();
//	}
//}

//QPoint VideoRenderer::offset() const {
//	return d->offset;
//}

//void VideoRenderer::setAlignment(int alignment) {
//	if (d->alignment != alignment) {
//		d->alignment = alignment;
//		update();
//	}
//}

//int VideoRenderer::alignment() const {
//	return d->alignment;
//}

//VideoRenderer::Effects VideoRenderer::effects() const {
//	return d->var.effects();
//}

//VideoScreen &VideoRenderer::screen() const {
//	return d->screen;
//}

//VideoScreen::VideoScreen()
//: QGLFunctions(context()) {
//	glContext = this;
//	makeCurrent();
//	doneCurrent();
//	m_overlay = new Overlay(this);
//	setAcceptDrops(false);
//}

//VideoScreen::~VideoScreen() {
//	makeCurrent();
//	doneCurrent();
//	delete m_overlay;
//}

//void VideoScreen::changeEvent(QEvent *event) {
//	QGLWidget::changeEvent(event);
//	if (event->type() == QEvent::ParentChange) {
//		if (m_parent)
//			m_parent->removeEventFilter(this);
//		m_parent = parentWidget();
//		if (m_parent)
//			m_parent->installEventFilter(this);
//	}
//}

//bool VideoScreen::eventFilter(QObject *o, QEvent *e) {
//	if (o == m_parent && e->type() == QEvent::Resize) {
//		resize(m_parent->size());
//	}
//	return false;
//}
