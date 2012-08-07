#include "videorenderer.hpp"
#include "playengine.hpp"
#include "overlay.hpp"
#include "colorproperty.hpp"
#include "events.hpp"
#include "videoframe.hpp"
#include "pref.hpp"
#include "logodrawer.hpp"
#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtCore/QTime>
#include <QtCore/QDebug>
#include <QtCore/QMutex>
#include <QtCore/QSize>
#include <QtCore/QRegExp>
#include <QtCore/QCoreApplication>
#include <math.h>
#include <QtCore/QFile>
#include <QtOpenGL/QGLShader>
#include <QtOpenGL/QGLShaderProgram>
#include "mpcore.hpp"

#ifndef GL_UNPACK_CLIENT_STORAGE_APPLE
#define GL_UNPACK_CLIENT_STORAGE_APPLE 34226
#endif

#ifndef GL_TEXTURE_STORAGE_HINT_APPLE
#define GL_TEXTURE_STORAGE_HINT_APPLE 34236
#endif

#ifndef GL_STORAGE_CACHED_APPLE
#define GL_STORAGE_CACHED_APPLE 34238
#endif

#ifndef GL_TEXTURE_RECTANGLE_ARB
#define GL_TEXTURE_RECTANGLE_ARB 34037
#endif

extern "C" {
#include <stream/stream_dvdnav.h>
#include <input/input.h>
#include <libmpdemux/stheader.h>
#include <codec-cfg.h>
#include <libmpcodecs/vd.h>
void *fast_memcpy(void * to, const void * from, size_t len);
#undef clamp
}

struct MPlayerOsdWrapper : public OsdWrapper {
	void alloc() {
		OsdWrapper::alloc();
		m_shader = new QGLShaderProgram(QGLContext::currentContext());
		m_shader->addShaderFromSourceCode(QGLShader::Fragment,
			"uniform sampler2D tex_y, tex_a;"
			"void main() {"
			"	float luma = texture2D(tex_y, gl_TexCoord[0].xy).x;"
			"	float alpha = texture2D(tex_a, gl_TexCoord[0].xy).x;"
			"	gl_FragColor = vec4(luma, luma, luma, alpha);"
			"}"
		);
		m_shader->link();
	}
	void free() {
		delete m_shader;
		m_shader = nullptr;
		OsdWrapper::free();
	}

	void draw(int x, int y, int /*w*/, int h, uchar *src, uchar *srca, int stride) {
		m_pos.rx() = x;
		m_pos.ry() = y;
		const int length = h*stride;
		if ((m_empty = !(length > 0 && count() > 0 && m_shader)))
			return;
		m_alpha.resize(length);
		char *data = m_alpha.data();
		for (int i=0; i<length; ++i)
			*data++ = -*srca++;
		upload(QSize(stride, h), src, 0);
		upload(QSize(stride, h), m_alpha.data(), 1);
	}

	void render() {
		if (m_empty)
			return;
		m_shader->bind();
		m_shader->setUniformValue("tex_y", 4);
		m_shader->setUniformValue("tex_a", 5);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, texture(0));
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, texture(1));
		glActiveTexture(GL_TEXTURE4);

		float textureCoords[] = {
			0.f, 0.f,					sub_x(0), 0.f,
			sub_x(0), sub_y(0),			0.f, sub_y(0)
		};

		const double expand_x = frameVtx.width()/frameSize.width();
		const double expand_y = frameVtx.height()/frameSize.height();
		const float x1 = m_pos.x()*expand_x + frameVtx.x();
		const float y1 = m_pos.y()*expand_y + frameVtx.y();
		const float x2 = x1 + width(0)*expand_x;
		const float y2 = y1 + height(0)*expand_y;
		float vertexCoords[] = {
			x1, y1,			x2, y1,
			x2, y2,			x1, y2
		};

//		glClearColor(0.f, 0.f, 0.f, 0.f);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, textureCoords);
		glVertexPointer(2, GL_FLOAT, 0, vertexCoords);
		glDrawArrays(GL_QUADS, 0, 4);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
		m_shader->release();
	}
	QRectF frameVtx;
	QSize frameSize;
private:
	int _count() const {return 2;}
	GLint _internalFormat(int) const {return GL_LUMINANCE;}
	GLenum _format(int) const {return GL_LUMINANCE;}
	GLenum _type(int) const {return GL_UNSIGNED_BYTE;}
	QPoint m_pos;
	QByteArray m_alpha;
	QGLShaderProgram *m_shader = nullptr;
};

class FrameRateMeasure {
public:
	FrameRateMeasure() {m_drawn = 0; m_prev = 0;}
	void reset() {m_time.restart(); m_drawn = 0; m_prev = 0;}
	int elapsed() const {return m_time.elapsed();}
	void frameDrawn(int id) {
		if (m_prev != id) {
			++m_drawn;
			m_prev = id;
		}
	}
	int drawnFrames() const {return m_drawn;}
	double frameRate() const {return (double)m_drawn/(double)m_time.elapsed()*1e3;}
private:
	int m_drawn = 0, m_prev = 0;
	QTime m_time;
};

class VideoRenderer::VideoShader {
public:
	struct Var {
		Var() {
			setColor(m_color);
			setEffects(m_effects);
		}
		inline const ColorProperty &color() const {return m_color;}
		inline void setColor(const ColorProperty &color) {
			m_color = color;
			brightness = qBound(-1.0, m_color.brightness(), 1.0);
			contrast = qBound(0., m_color.contrast() + 1., 2.);
			updateHS();
		}
		inline int setEffects(Effects effects) {
			m_effects = effects;
			int idx = 0;
			rgb_0 = 0.0;
			rgb_c[0] = rgb_c[1] = rgb_c[2] = 1.0;
			kern_c = kern_d = kern_n = 0.0;
			if (!(effects & IgnoreEffect)) {
				if (effects & FilterEffects) {
					idx = 1;
					if (effects & InvertColor) {
						rgb_0 = 1.0;
						rgb_c[0] = rgb_c[1] = rgb_c[2] = -1.0;
					}
				}
				if (effects & KernelEffects) {
					idx = 2;
					const Pref &p = Pref::get();
					if (effects & Blur) {
						kern_c += p.blur_kern_c;
						kern_n += p.blur_kern_n;
						kern_d += p.blur_kern_d;
					}
					if (effects & Sharpen) {
						kern_c += p.sharpen_kern_c;
						kern_n += p.sharpen_kern_n;
						kern_d += p.sharpen_kern_d;
					}
					const double den = 1.0/(kern_c + kern_n*4.0 + kern_d*4.0);
					kern_c *= den;
					kern_d *= den;
					kern_n *= den;
				}
			}
			updateHS();
			return m_idx = idx;
		}
		Effects effects() const {return m_effects;}
		void setYRange(float min, float max) {y_min = min; y_max = max;}
		int id() const {return m_idx;}
	private:
		void updateHS() {
			double sat_sinhue = 0.0, sat_coshue = 0.0;
			if (!(!(m_effects & IgnoreEffect) && (m_effects & Grayscale))) {
				const double sat = qBound(0.0, m_color.saturation() + 1.0, 2.0);
				const double hue = qBound(-M_PI, m_color.hue()*M_PI, M_PI);
				sat_sinhue = sat*sin(hue);
				sat_coshue = sat*cos(hue);
			}
			sat_hue[0][0] = sat_hue[1][1] = sat_coshue;
			sat_hue[1][0] = -(sat_hue[0][1] = sat_sinhue);
		}
		float rgb_0, rgb_c[3];
		float kern_d, kern_c, kern_n;
		float y_min = 0.0f, y_max = 1.0f;
		float brightness, contrast, sat_hue[2][2];
		Effects m_effects = 0;
		int m_idx = 0;
		ColorProperty m_color;

		friend class VideoShader;
	};

	VideoShader(const QGLContext *ctx): m_shader(ctx) {}
	bool add(const QString &fileName) {return m_shader.addShaderFromSourceFile(QGLShader::Fragment, fileName);}
	bool add(const QByteArray &code) {return m_shader.addShaderFromSourceCode(QGLShader::Fragment, code);}
	bool bind() {return m_shader.bind();}
	void release() {m_shader.release();}
	bool link() {
		const bool ret = m_shader.link();
		if (ret) {
			loc_y = m_shader.uniformLocation("y");
			loc_u = m_shader.uniformLocation("u");
			loc_v = m_shader.uniformLocation("v");
			loc_brightness = m_shader.uniformLocation("brightness");
			loc_contrast = m_shader.uniformLocation("contrast");
			loc_sat_hue = m_shader.uniformLocation("sat_hue");
			loc_rgb_c = m_shader.uniformLocation("rgb_c");
			loc_rgb_0 = m_shader.uniformLocation("rgb_0");
			loc_y_tan = m_shader.uniformLocation("y_tan");
			loc_y_b = m_shader.uniformLocation("y_b");
			loc_dxy = m_shader.uniformLocation("dxy");
			loc_kern_c = m_shader.uniformLocation("kern_c");
			loc_kern_d = m_shader.uniformLocation("kern_d");
			loc_kern_n = m_shader.uniformLocation("kern_n");
		}
		return ret;
	}
	void setUniforms(const Var &var, const VideoFormat &format) {
		m_shader.setUniformValue(loc_y, 0);
		m_shader.setUniformValue(loc_u, 1);
		m_shader.setUniformValue(loc_v, 2);
		m_shader.setUniformValue(loc_brightness, var.brightness);
		m_shader.setUniformValue(loc_contrast, var.contrast);
		m_shader.setUniformValue(loc_sat_hue, var.sat_hue);
		const bool filter = var.effects() & FilterEffects;
		const bool kernel = var.effects() & KernelEffects;
		if (filter || kernel) {
			m_shader.setUniformValue(loc_rgb_c, var.rgb_c[0], var.rgb_c[1], var.rgb_c[2]);
			m_shader.setUniformValue(loc_rgb_0, var.rgb_0);
			const float y_tan = 1.0/(var.y_max - var.y_min);
			m_shader.setUniformValue(loc_y_tan, y_tan);
			m_shader.setUniformValue(loc_y_b, (float)-var.y_min*y_tan);
		}
		if (kernel) {
			const float dx = 1.0/(double)format.stride;
			const float dy = 1.0/(double)format.height;
			m_shader.setUniformValue(loc_dxy, dx, dy, -dx, 0.f);
			m_shader.setUniformValue(loc_kern_c, var.kern_c);
			m_shader.setUniformValue(loc_kern_n, var.kern_n);
			m_shader.setUniformValue(loc_kern_d, var.kern_d);
		}
	}
private:
	QGLShaderProgram m_shader;
	int loc_rgb_0, loc_rgb_c, loc_kern_d, loc_kern_c, loc_kern_n, loc_y_tan, loc_y_b;
	int loc_brightness, loc_contrast, loc_sat_hue, loc_y, loc_u, loc_v, loc_dxy;
};

struct VideoRenderer::Data {
	static const int i420ToRgbSimple = 0;
	static const int i420ToRgbFilter = 1;
	static const int i420ToRgbKernel = 2;
	FrameRateMeasure fps;
	int frameId;
	int align = Qt::AlignCenter;
	QSize renderSize;
	LogoDrawer logo;
	GLuint texture[3];
	VideoFrame *buffer, *frame, buf[2];
	QRectF vtx;
	QPoint offset = {0, 0};
	VideoFormat format;
	VideoScreen *gl = nullptr;
	VideoShader::Var var;
	PlayEngine *engine;
	QList<VideoShader*> shaders;
	double crop = -1.0, aspect = -1.0, dar = 0.0;
	double cpu = -1.0;
	bool prepared = false, logoOn = false, frameIsSet = false, hasPrograms = false, binding = false;
	bool render = false;
	MPlayerOsdWrapper osd;
	StreamList streams;
	QString codec;
	QSize viewport;
	bool clientStorage = false;
};

VideoRenderer::VideoRenderer(PlayEngine *engine)
: d(new Data) {
	d->engine = engine;
	d->frame = &d->buf[0];
	d->buffer = &d->buf[1];

	connect(engine, SIGNAL(aboutToOpen()), this, SLOT(onAboutToOpen()));
	connect(engine, SIGNAL(aboutToPlay()), this, SLOT(onAboutToPlay()));
}

VideoRenderer::~VideoRenderer() {
	setScreen(nullptr);
	delete d;
}

void VideoRenderer::drawAlpha(void *p, int x, int y, int w, int h, uchar *src, uchar *srca, int stride) {
	Data *d = reinterpret_cast<VideoRenderer*>(p)->d;
	if (d->gl) {
		d->gl->makeCurrent();
		d->osd.draw(x, y, w, h, src, srca, stride);
		d->gl->doneCurrent();
	}
}

void VideoRenderer::onAboutToPlay() {
}

void VideoRenderer::onAboutToOpen() {
	d->streams.clear();
	d->dar = 0.0;
	d->codec.clear();
}

bool VideoRenderer::parse(const Id &id) {
	if (getStream(id, "VIDEO", "VID", d->streams))
		return true;
	if (!id.name.isEmpty()) {
		if (same(id.name, "VIDEO_ASPECT")) {
			d->dar = id.value.toDouble();
			return true;
		}
	}
	return false;
}

bool VideoRenderer::parse(const QString &/*line*/) {
	return false;
}

StreamList VideoRenderer::streams() const {
	return d->streams;
}

int VideoRenderer::currentStreamId() const {
	MPContext *mpctx = d->engine->context();
	if (mpctx && mpctx->sh_video)
		return mpctx->sh_video->vid;
	return -1;
}
void VideoRenderer::setCurrentStream(int id) {
	d->engine->tellmp("switch_video", id);
}

void VideoRenderer::setScreen(VideoScreen *gl) {
	if (d->gl == gl)
		return;
	if (d->gl) {
		d->gl->makeCurrent();
		qDeleteAll(d->shaders);
		d->shaders.clear();
		glDeleteTextures(3, d->texture);
		d->gl->doneCurrent();
	}
	d->gl = gl;
	if (d->gl) {
		d->gl->makeCurrent();
		auto exts = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
		d->clientStorage = strstr(exts, "GL_APPLE_client_storage") && strstr(exts, "GL_APPLE_texture_range");
		glGenTextures(3, d->texture);
		auto readAll = [] (const QString &fileName) -> QByteArray {
			QFile file(fileName);
			if (file.open(QFile::ReadOnly))
				return file.readAll();
			return QByteArray();
		};
		const auto common = readAll(":/shaders/i420_to_rgb_common.glsl");
		auto makeShader = [gl, &common, &readAll] (const QString &fileName) -> VideoShader* {
			auto shader = new VideoShader(gl->context());
			shader->add(common);
			shader->add(readAll(fileName));
			shader->link();
			return shader;
		};
		d->shaders << makeShader(":/shaders/i420_to_rgb_simple.glsl")
			<< makeShader(":/shaders/i420_to_rgb_filter.glsl")
			<< makeShader(":/shaders/i420_to_rgb_kernel.glsl");
		d->hasPrograms =  d->shaders[d->var.id()];
		Q_ASSERT(d->hasPrograms);

		d->gl->doneCurrent();

		d->gl->setMinimumSize(QSize(200, 100));
		d->gl->setAutoFillBackground(false);
		d->gl->setAttribute(Qt::WA_OpaquePaintEvent, true);
		d->gl->setAttribute(Qt::WA_NoSystemBackground, true);
		d->gl->setAttribute(Qt::WA_PaintOutsidePaintEvent, true);
		d->gl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		d->gl->setMouseTracking(true);
		d->gl->setContextMenuPolicy(Qt::CustomContextMenu);
	}
	d->logo.bind(d->gl);
}

QImage VideoRenderer::frameImage() const {
	if (!d->frameIsSet)
		return QImage();
	return d->frame->toImage();
}

double VideoRenderer::frameRate() const {
	return d->engine->hasVideo() ? d->engine->context()->sh_video->fps : 25;
}

double VideoRenderer::outputFrameRate(bool reset) const {
	const double ret = d->fps.frameRate();
	if (reset)
		d->fps.reset();
	return ret;
}

const VideoFormat &VideoRenderer::format() const {
	return d->format;
}

bool VideoRenderer::hasFrame() const {
	return d->frameIsSet;
}

void VideoRenderer::uploadBufferFrame() {
//	qSwap(d->buffer, d->frame);
	auto &frame = *d->frame;
	if (!d->prepared || d->format != frame.format)
		prepare(frame.format);
	if (!d->gl || !(d->frameIsSet = !frame.format.isEmpty()))
		return;
	auto min = 0, max = 255;
	const auto effects = d->var.effects();
	if (!(effects & IgnoreEffect)) {
		if (effects & RemapLuma) {
			min = Pref::get().adjust_contrast_min_luma;
			max = Pref::get().adjust_contrast_max_luma;
		}
	}
	d->var.setYRange((float)min/255.0f, (float)max/255.0f);

	d->gl->makeCurrent();
	const auto w = frame.format.stride, h = frame.format.height;
//	if (d->clientStorage) {
//		glBindTexture(GL_TEXTURE_2D, d->texture[0]);
//		glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
//		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame.y());
//		glBindTexture(GL_TEXTURE_2D, d->texture[1]);
//		glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
//		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w/2, h/2, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame.u());
//		glBindTexture(GL_TEXTURE_2D, d->texture[2]);
//		glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
//		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w/2, h/2, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame.v());
//	} else{
		glBindTexture(GL_TEXTURE_2D, d->texture[0]);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame.y());
		glBindTexture(GL_TEXTURE_2D, d->texture[1]);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w/2, h/2, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame.u());
		glBindTexture(GL_TEXTURE_2D, d->texture[2]);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w/2, h/2, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame.v());
//	}
	++d->frameId;
	d->gl->doneCurrent();
}

VideoFrame &VideoRenderer::bufferFrame() {
	return *d->frame;
	return *d->buffer;
}

void VideoRenderer::prepare(const VideoFormat &format) {
	if (d->format != format) {
		d->prepared = false;
		d->var.setYRange(0.0f, 1.0f);
		d->frame->format = format;
		d->format = format;
		emit formatChanged(d->format);
		const int w[3] = {d->format.stride, d->format.stride/2, d->format.stride/2};
		const int h[3] = {d->format.height, d->format.height/2, d->format.height/2};
		d->gl->makeCurrent();
		for (int i=0; i<3; ++i) {
			glBindTexture(GL_TEXTURE_2D, d->texture[i]);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//			if (d->clientStorage) {
//				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_STORAGE_HINT_APPLE, GL_STORAGE_CACHED_APPLE);
//				glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
//			}
			glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w[i], h[i], 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, nullptr);

		}
		d->gl->doneCurrent();
		d->fps.reset();
		d->osd.frameSize = d->format.size();
		d->prepared = true;
		updateSize();
		emit formatChanged(d->format);
	}
}

int VideoRenderer::outputWidth() const {
	return d->dar > 0.01 ? (int)(d->dar*(double)d->format.height + 0.5) : d->format.width;
}

double VideoRenderer::targetAspectRatio() const {
	if (d->aspect > 0.0)
		return d->aspect;
	if (d->aspect == 0.0)
		return d->gl ? d->gl->aspectRatio() : 1.0;
	return d->dar > 0.01 ? d->dar : (double)d->format.width/(double)d->format.height;
}

double VideoRenderer::targetCropRatio(double fallback) const {
	if (d->crop > 0.0)
		return d->crop;
	if (d->crop == 0.0)
		return d->gl ? d->gl->aspectRatio() : 1.0;
	return fallback;
}

QSize VideoRenderer::sizeHint() const {
	if (d->format.isEmpty())
		return QSize(400, 300);
	const double aspect = targetAspectRatio();
	QSizeF size(aspect, 1.0);
	size.scale(d->format.size(), Qt::KeepAspectRatioByExpanding);
	QSizeF crop(targetCropRatio(aspect), 1.0);
	crop.scale(size, Qt::KeepAspectRatio);
	return crop.toSize();
}

void VideoRenderer::update() {
	if (!d->render)
		d->render = true;
	if (d->gl)
		PlayEngine::get().enqueue(new PlayEngine::Cmd(PlayEngine::Cmd::VideoUpdate));
}

void VideoRenderer::setAspectRatio(double ratio) {
	if (!isSameRatio(d->aspect, ratio)) {
		d->aspect = ratio;
		updateSize();
		update();
	}
}

double VideoRenderer::aspectRatio() const {
	return d->aspect;
}

void VideoRenderer::setCropRatio(double ratio) {
	if (!isSameRatio(d->crop, ratio)) {
		d->crop = ratio;
		updateSize();
		update();
	}
}

double VideoRenderer::cropRatio() const {
	return d->crop;
}

void VideoRenderer::setLogoMode(bool on) {
	d->logoOn = on;
}

void VideoRenderer::setColorProperty(const ColorProperty &prop) {
	d->var.setColor(prop);
	update();
}

const ColorProperty &VideoRenderer::colorProperty() const {
	return d->var.color();
}

void VideoRenderer::setFixedRenderSize(const QSize &size) {
	if (d->renderSize != size) {
		d->renderSize = size;
		updateSize();
		update();
	}
}

QSize VideoRenderer::renderableSize() const {
	return d->renderSize.isEmpty() ? (d->gl ? d->gl->size() : QSize(100, 100)) : d->renderSize;
}

void VideoRenderer::updateSize() {
	const QSizeF widget = renderableSize();
	QRectF vtx(QPointF(0, 0), widget);
	if (!d->logoOn && d->hasPrograms && d->prepared) {
		const double aspect = targetAspectRatio();
		QSizeF frame(aspect, 1.0);
		QSizeF letter(targetCropRatio(aspect), 1.0);
		letter.scale(widget, Qt::KeepAspectRatio);
		frame.scale(letter, Qt::KeepAspectRatioByExpanding);
		vtx.setLeft((widget.width() - frame.width())*0.5);
		vtx.setTop((widget.height() - frame.height())*0.5);
		vtx.setSize(frame);
	}
	if (d->vtx != vtx) {
		d->vtx = vtx;
		d->osd.frameVtx = d->vtx;
		if (d->gl)
			d->gl->overlay()->setArea(QRect(QPoint(0, 0), widget.toSize()), d->vtx.toRect());
	}
}

void VideoRenderer::render() {
	if (!d->gl)
		return;
	const QSizeF widget = renderableSize();
	if (!d->logoOn && d->hasPrograms && d->frameIsSet && d->prepared) {
		QSizeF letter(targetCropRatio(targetAspectRatio()), 1.0);
		letter.scale(widget, Qt::KeepAspectRatio);

		QPointF offset = d->offset;
		QPointF xy(widget.width(), widget.height());
		xy.rx() -= letter.width(); xy.ry() -= letter.height();	xy *= 0.5;
		if (d->align & Qt::AlignLeft)
			offset.rx() -= xy.x();
		else if (d->align & Qt::AlignRight)
			offset.rx() += xy.x();
		if (d->align & Qt::AlignTop)
			offset.ry() -= xy.y();
		else if (d->align & Qt::AlignBottom)
			offset.ry() += xy.y();
		xy += offset;
		double top = 0.0, left = 0.0;
		double bottom = (double)(d->format.height-1)/(double)d->format.height;
		double right = (double)(d->format.width-1)/(double)d->format.stride;
		const Effects effects = d->var.effects();
		if (!(effects & IgnoreEffect)) {
			if (effects & FlipHorizontally)
				qSwap(left, right);
			if (effects & FlipVertically)
				qSwap(top, bottom);
		}
		d->gl->makeCurrent();
		if (d->viewport != d->gl->size()) {
			d->viewport = d->gl->size();
			glViewport(0, 0, d->viewport.width(), d->viewport.height());
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0, d->viewport.width(), d->viewport.height(), 0, -1, 1);
			glMatrixMode(GL_MODELVIEW);
		}
		glLoadIdentity();
		auto shader = d->shaders[d->var.id()];

//		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		shader->bind();
		shader->setUniforms(d->var, d->frame->format);
		const float textureCoords[] = {
			(float)left,	(float)top,
			(float)right,	(float)top,
			(float)right,	(float)bottom,
			(float)left,	(float)bottom,
		};
		const float vertexCoords[] = {
			(float)(d->vtx.left()+offset.x()),	(float)(d->vtx.top()+offset.y()),
			(float)(d->vtx.right()+offset.x()),	(float)(d->vtx.top()+offset.y()),
			(float)(d->vtx.right()+offset.x()),	(float)(d->vtx.bottom()+offset.y()),
			(float)(d->vtx.left()+offset.x()),	(float)(d->vtx.bottom()+offset.y())
		};

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, d->texture[0]);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, d->texture[1]);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, d->texture[2]);
		glActiveTexture(GL_TEXTURE0);

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, textureCoords);
		glVertexPointer(2, GL_FLOAT, 0, vertexCoords);
		glDrawArrays(GL_QUADS, 0, 4);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);

		shader->release();

		glColor3f(0.0f, 0.0f, 0.0f);
		auto fillRect = [] (float x, float y, float w ,float h) {glRectf(x, y, x+w, y+h);};
		if (0.0 <= xy.y() && xy.y() <= widget.height())
			fillRect(.0, .0, widget.width(), xy.y());
		xy.ry() += letter.height();
		if (0.0 <= xy.y() && xy.y() <= widget.height())
			fillRect(.0, xy.y(), widget.width(), widget.height()-xy.y());
		if (0.0 <= xy.x() && xy.x() <= widget.width())
			fillRect(.0, .0, xy.x(), widget.height());
		xy.rx() += letter.width();
		if (0.0 <= xy.x() && xy.x() <= widget.width())
			fillRect(xy.x(), .0, widget.width()-xy.x(), widget.height());

		d->gl->overlay()->renderToScreen();
		d->osd.render();
		d->gl->swapBuffers();
		d->fps.frameDrawn(d->frameId);
	} else {
		d->gl->makeCurrent();
		QPainter painter(d->gl);
		d->logo.draw(&painter, QRectF(QPointF(0, 0), widget));
		painter.beginNativePainting();
		d->gl->overlay()->renderToScreen();
		painter.endNativePainting();
		d->gl->doneCurrent();
	}
	if (d->render)
		d->render = false;
}

bool VideoRenderer::needsToBeRendered() const {
	return d->render;
}

void VideoRenderer::onMouseMoved(const QPointF &p) {
	auto mpctx = d->engine->context();
	if (mpctx && mpctx->stream && mpctx->stream->type == STREAMTYPE_DVDNAV && d->vtx.isValid() && !d->frame->format.isEmpty()) {
		auto pos = p;
		pos -= d->vtx.topLeft();
		pos.rx() *= (double)d->format.width/d->vtx.width();
		pos.ry() *= (double)d->format.height/d->vtx.height();
		auto button = 0;
		mp_dvdnav_update_mouse_pos(mpctx->stream, pos.x() + 0.5, pos.y() + 0.5, &button);
	}
}

void VideoRenderer::onMouseClicked(const QPointF &p) {
	auto mpctx = d->engine->context();
	if (mpctx && mpctx->stream && mpctx->stream->type == STREAMTYPE_DVDNAV && d->vtx.isValid() && !d->frame->format.isEmpty()) {
		auto pos = p;
		pos -= d->vtx.topLeft();
		pos.rx() *= (double)d->format.width/d->vtx.width();
		pos.ry() *= (double)d->format.height/d->vtx.height();
		auto button = 0;
		mp_dvdnav_update_mouse_pos(mpctx->stream, pos.x() + 0.5, pos.y() + 0.5, &button);
		mp_dvdnav_handle_input(mpctx->stream, MP_CMD_DVDNAV_MOUSECLICK, &button);
	}
}

void VideoRenderer::clearEffects() {
	d->var.setEffects(0);
	update();
}

void VideoRenderer::setEffect(Effect effect, bool on) {
	setEffects(on ? (d->var.effects() | effect) : (d->var.effects() & ~effect));
}

void VideoRenderer::setEffects(Effects effects) {
	if (d->var.effects() != effects) {
		d->var.setEffects(effects);
		update();
	}
}

void VideoRenderer::setOffset(const QPoint &offset) {
	if (d->offset != offset) {
		d->offset = offset;
		update();
	}
}

QPoint VideoRenderer::offset() const {
	return d->offset;
}

void VideoRenderer::setAlignment(int align) {
	if (d->align != align) {
		d->align = align;
		update();
	}
}

int VideoRenderer::alignment() const {
	return d->align;
}

VideoRenderer::Effects VideoRenderer::effects() const {
	return d->var.effects();
}

void plug(VideoRenderer *renderer, VideoScreen *screen) {
	renderer->setScreen(screen);
	screen->r = renderer;
	screen->makeCurrent();
	renderer->d->osd.alloc();
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	screen->doneCurrent();
}

void unplug(VideoRenderer *renderer, VideoScreen *screen) {
	screen->makeCurrent();
	renderer->d->osd.free();
	renderer->setScreen(nullptr);
	screen->r = nullptr;
	screen->doneCurrent();
}

VideoScreen::VideoScreen()
: QGLFunctions(context()) {
	doneCurrent();
	m_overlay = new Overlay(this);
	setAcceptDrops(false);
}

VideoScreen::~VideoScreen() {
	doneCurrent();
	delete m_overlay;
}

void VideoScreen::changeEvent(QEvent *event) {
	QGLWidget::changeEvent(event);
	if (event->type() == QEvent::ParentChange) {
		if (m_parent)
			m_parent->removeEventFilter(this);
		m_parent = parentWidget();
		m_parent->installEventFilter(this);
	}
}

bool VideoScreen::eventFilter(QObject *o, QEvent *e) {
	if (o == m_parent && e->type() == QEvent::Resize) {
		resize(m_parent->size());
	}
	return false;
}
