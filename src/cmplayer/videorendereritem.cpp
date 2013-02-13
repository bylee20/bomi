#include "videorendereritem.hpp"
#include "mposditem.hpp"
#include "videoframe.hpp"
#include "shadervar.h"

struct VideoRendererItem::Data {
	VideoFrame frame;
	bool quit = false, framePended = false;
	QRectF vtx;
	QPoint offset = {0, 0};
	double crop = -1.0, aspect = -1.0, dar = 0.0;
	VideoFormat format;
	int alignment = Qt::AlignCenter;
	quint64 drawnFrames = 0;
	ShaderVar shaderVar;
	LetterboxItem *letterbox = nullptr;
	MpOsdItem *mposd = nullptr;
	QQuickItem *overlay = nullptr;
	QByteArray shader;
	int loc_rgb_0, loc_rgb_c, loc_kern_d, loc_kern_c, loc_kern_n, loc_y_tan, loc_y_b;
	int loc_brightness, loc_contrast, loc_sat_hue, loc_dxy, loc_p1, loc_p2, loc_p3;
	int loc_strideCorrection;
	QPointF strideCorrection;
	VideoFormat::Type shaderType = VideoFormat::BGRA;
	QMutex mutex;
};

VideoRendererItem::VideoRendererItem(QQuickItem *parent)
: TextureRendererItem(3, parent), d(new Data) {
	setFlags(ItemHasContents | ItemAcceptsDrops);
	d->mposd = new MpOsdItem(this);
	d->letterbox = new LetterboxItem(this);
	setZ(-1);
	connect(this, &VideoRendererItem::framePended, this, &VideoRendererItem::update, Qt::BlockingQueuedConnection);
}

VideoRendererItem::~VideoRendererItem() {
	delete d;
}

QQuickItem *VideoRendererItem::overlay() const {
	return d->overlay;
}

bool VideoRendererItem::isFramePended() const {
	return d->quit ? false : d->framePended;
}

const VideoFrame &VideoRendererItem::frame() const {
	return d->frame;
}

//VideoFrame &VideoRendererItem::getNextFrame() const {
//	d->mposd->beginNewFrame();
//	d->next.newId();
//	return d->next;
//}

void VideoRendererItem::present(const VideoFrame &frame) {
	if (!d->quit) {
		d->mutex.lock();
		d->frame = frame;
		d->framePended = true;
		d->mutex.unlock();
		emit framePended();
	}
}

QRectF VideoRendererItem::screenRect() const {
	return d->letterbox->screen();
}

int VideoRendererItem::alignment() const {
	return d->alignment;
}

void VideoRendererItem::setAlignment(int alignment) {
	if (d->alignment != alignment) {
		d->alignment = alignment;
		updateGeometry();
		update();
	}
}

double VideoRendererItem::targetAspectRatio() const {
	if (d->aspect > 0.0)
		return d->aspect;
	if (d->aspect == 0.0)
		return itemAspectRatio();
	return d->dar > 0.01 ? d->dar : _Ratio(d->format.size());
}

double VideoRendererItem::targetCropRatio(double fallback) const {
	if (d->crop > 0.0)
		return d->crop;
	if (d->crop == 0.0)
		return itemAspectRatio();
	return fallback;
}

void VideoRendererItem::setOverlay(QQuickItem *overlay) {
	if (d->overlay != overlay) {
		if (d->overlay)
			d->overlay->setParentItem(nullptr);
		if ((d->overlay = overlay))
			d->overlay->setParentItem(this);
	}
}

void VideoRendererItem::geometryChanged(const QRectF &newOne, const QRectF &old) {
	QQuickItem::geometryChanged(newOne, old);
	d->letterbox->setWidth(width());
	d->letterbox->setHeight(height());
	if (d->overlay) {
		d->overlay->setPosition(QPointF(0, 0));
		d->overlay->setSize(QSizeF(width(), height()));
	}
	updateGeometry();
}

void VideoRendererItem::setOffset(const QPoint &offset) {
	if (d->offset != offset) {
		d->offset = offset;
		emit offsetChanged(d->offset);
        setGeometryDirty();
        update();
	}
}

QPoint VideoRendererItem::offset() const {
	return d->offset;
}

quint64 VideoRendererItem::drawnFrames() const {
	return d->drawnFrames;
}

VideoRendererItem::Effects VideoRendererItem::effects() const {
	return d->shaderVar.effects();
}

void VideoRendererItem::setEffects(Effects effects) {
	if (d->shaderVar.effects() != effects) {
		d->shaderVar.setEffects(effects);
		update();
	}
}

void VideoRendererItem::updateGeometry() {
	QRectF vtx(x(), y(), width(), height());
	if (!d->format.isEmpty()) {
			const double aspect = targetAspectRatio();
			QSizeF frame(aspect, 1.0), letter(targetCropRatio(aspect), 1.0);
			letter.scale(width(), height(), Qt::KeepAspectRatio);
			frame.scale(letter, Qt::KeepAspectRatioByExpanding);
			QPointF pos(x(), y());
			pos.rx() += (width() - frame.width())*0.5;
			pos.ry() += (height() - frame.height())*0.5;
			vtx = QRectF(pos, frame);
	}
	if (d->vtx != vtx) {
		d->vtx = vtx;
		d->mposd->setPosition(d->vtx.topLeft());
		d->mposd->setSize(d->vtx.size());
        setGeometryDirty();
	}
}

void VideoRendererItem::quit() {
	d->quit = true;
	d->mutex.lock();
	d->framePended = false;
	d->mutex.unlock();
//	d->wait.wakeAll();
	setOverlay(nullptr);
}

void VideoRendererItem::setColor(const ColorProperty &prop) {
	if (d->shaderVar.color() != prop) {
		d->shaderVar.setColor(prop);
		update();
	}
}

const ColorProperty &VideoRendererItem::color() const {
	return d->shaderVar.color();
}

void VideoRendererItem::setAspectRatio(double ratio) {
	if (!isSameRatio(d->aspect, ratio)) {
		d->aspect = ratio;
		updateGeometry();
		update();
	}
}

double VideoRendererItem::aspectRatio() const {
	return d->aspect;
}

void VideoRendererItem::setCropRatio(double ratio) {
	if (!isSameRatio(d->crop, ratio)) {
		d->crop = ratio;
		updateGeometry();
		update();
	}
}

double VideoRendererItem::cropRatio() const {
	return d->crop;
}

void VideoRendererItem::setVideoAspectRaito(double ratio) {
	d->dar = ratio;
}


QSize VideoRendererItem::sizeHint() const {
	if (d->format.isEmpty())
		return QSize(400, 300);
	const double aspect = targetAspectRatio();
	QSizeF size(aspect, 1.0);
	size.scale(d->format.size(), Qt::KeepAspectRatioByExpanding);
	QSizeF crop(targetCropRatio(aspect), 1.0);
	crop.scale(size, Qt::KeepAspectRatio);
	return crop.toSize();
}

int VideoRendererItem::outputWidth() const {
	return d->dar > 0.01 ? (int)(d->dar*(double)d->format.height() + 0.5) : d->format.width();
}

QByteArray VideoRendererItem::shader(int type) {
	if (VideoFormat::isYCbCr(type)) {
		// **** common shader *****
		QByteArray shader = (R"(
			uniform float brightness, contrast;
			uniform mat2 sat_hue;
			uniform vec3 rgb_c;
			uniform float rgb_0;
			uniform float y_tan, y_b;

			void convert(inout vec3 yuv) {
				const vec3 yuv_0 = vec3(0.0625, 0.5, 0.5);

				yuv -= yuv_0;

				yuv.yz *= sat_hue;
				yuv *= contrast;
				yuv.x += brightness;

				const mat3 coef = mat3(
					1.16438356,  0.0,          1.59602679,
					1.16438356, -0.391762290, -0.812967647,
					1.16438356,  2.01723214,   0.0
				);
				yuv *= coef;
			}

			void adjust_rgb(inout vec3 rgb) {
				rgb *= rgb_c;
				rgb += rgb_0;
			}

			void renormalize_y(inout float y) {
				y = y_tan*y + y_b;
			}

			void apply_filter_convert(inout vec3 yuv) {
				renormalize_y(yuv.x);
				convert(yuv);
				adjust_rgb(yuv);
			}
		)");
		switch (type) {
		case VideoFormat::YV12:
		case VideoFormat::I420:
			shader.append(R"(
				uniform sampler2D p1, p2, p3;
				uniform vec2 sc;
				vec3 get_yuv(const vec2 coord) {
					vec3 yuv;
					yuv.x = texture2D(p1, coord).x;
					yuv.y = texture2D(p2, coord*vec2(sc.x, 1.0)).x;
					yuv.z = texture2D(p3, coord*vec2(sc.y, 1.0)).x;
					return yuv;
				}
			)");
			break;
		case VideoFormat::NV12:
			shader.append(R"(
				uniform sampler2D p1, p2;
				uniform vec2 sc;
				vec3 get_yuv(const vec2 coord) {
					vec3 yuv;
					yuv.x = texture2D(p1, coord).x;
					yuv.yz = texture2D(p2, coord*vec2(sc.x, 1.0)).xw;
					return yuv;
				}
			)");
			break;
		case VideoFormat::NV21:
			shader.append(R"(
				uniform sampler2D p1, p2;
				uniform vec2 sc;
				vec3 get_yuv(const vec2 coord) {
					vec3 yuv;
					yuv.x = texture2D(p1, coord).x;
					yuv.yz = texture2D(p2, coord*vec2(sc.x, 1.0)).wx;
					return yuv;
				}
			)");
			break;
		case VideoFormat::YUY2:
			shader.append(R"(
				uniform sampler2D p1, p2;
				vec3 get_yuv(const vec2 coord) {
					vec3 yuv;
					yuv.x = texture2D(p1, coord).x;
					yuv.yz = texture2D(p2, coord).yw;
					return yuv;
				}
			)");
			break;
		case VideoFormat::UYVY:
			shader.append(R"(
				uniform sampler2D p1, p2;
				vec3 get_yuv(const vec2 coord) {
					vec3 yuv;
					yuv.x = texture2D(p1, coord).a;
					yuv.yz = texture2D(p2, coord).zx;
					return yuv;
				}
			)");
			break;
		default:
			break;
		}
		shader.append(R"(
			varying highp vec2 qt_TexCoord;
			void main() {
				vec3 c = get_yuv(qt_TexCoord);
				convert(c);
				gl_FragColor.xyz = c;
				gl_FragColor.w = 1.0;
			}
		)");
		return shader;
	} else {
		QByteArray shader = (R"(
			uniform sampler2D p1;
			varying highp vec2 qt_TexCoord;
			void main() {
				gl_FragColor = texture2D(p1, qt_TexCoord);
			}
		)");
		return shader;
	}
}

const char *VideoRendererItem::fragmentShader() const {
    d->shaderType = d->format.type();
    d->shader = shader(d->shaderType);
	return d->shader.constData();
}

void VideoRendererItem::link(QOpenGLShaderProgram *program) {
	TextureRendererItem::link(program);
	d->loc_brightness = program->uniformLocation("brightness");
	d->loc_contrast = program->uniformLocation("contrast");
	d->loc_sat_hue = program->uniformLocation("sat_hue");
	d->loc_rgb_c = program->uniformLocation("rgb_c");
	d->loc_rgb_0 = program->uniformLocation("rgb_0");
	d->loc_y_tan = program->uniformLocation("y_tan");
	d->loc_y_b = program->uniformLocation("y_b");
	d->loc_dxy = program->uniformLocation("dxy");
	d->loc_kern_c = program->uniformLocation("kern_c");
	d->loc_kern_d = program->uniformLocation("kern_d");
	d->loc_kern_n = program->uniformLocation("kern_n");
	d->loc_p1 = program->uniformLocation("p1");
	d->loc_p2 = program->uniformLocation("p2");
	d->loc_p3 = program->uniformLocation("p3");
	d->loc_strideCorrection = program->uniformLocation("sc");
}

void VideoRendererItem::drawMpOsd(void *pctx, sub_bitmaps *imgs) {
	reinterpret_cast<VideoRendererItem*>(pctx)->d->mposd->draw(imgs);
}

void VideoRendererItem::bind(const RenderState &state, QOpenGLShaderProgram *program) {
	TextureRendererItem::bind(state, program);
	program->setUniformValue(d->loc_p1, 0);
	program->setUniformValue(d->loc_p2, 1);
	program->setUniformValue(d->loc_p3, 2);
	program->setUniformValue(d->loc_brightness, d->shaderVar.brightness);
	program->setUniformValue(d->loc_contrast, d->shaderVar.contrast);
	program->setUniformValue(d->loc_sat_hue, d->shaderVar.sat_hue);
	const float dx = 1.0/(double)d->format.drawWidth();
	const float dy = 1.0/(double)d->format.drawHeight();
	program->setUniformValue(d->loc_dxy, dx, dy, -dx, 0.f);
	program->setUniformValue(d->loc_strideCorrection, d->strideCorrection);
	const bool filter = d->shaderVar.effects() & FilterEffects;
	const bool kernel = d->shaderVar.effects() & KernelEffects;
	if (filter || kernel) {
		program->setUniformValue(d->loc_rgb_c, d->shaderVar.rgb_c[0], d->shaderVar.rgb_c[1], d->shaderVar.rgb_c[2]);
		program->setUniformValue(d->loc_rgb_0, d->shaderVar.rgb_0);
		const float y_tan = 1.0/(d->shaderVar.y_max - d->shaderVar.y_min);
		program->setUniformValue(d->loc_y_tan, y_tan);
		program->setUniformValue(d->loc_y_b, (float)-d->shaderVar.y_min*y_tan);
	}
	if (kernel) {
		program->setUniformValue(d->loc_kern_c, d->shaderVar.kern_c);
		program->setUniformValue(d->loc_kern_n, d->shaderVar.kern_n);
		program->setUniformValue(d->loc_kern_d, d->shaderVar.kern_d);
	}
	if (!d->format.isEmpty()) {
		auto f = QOpenGLContext::currentContext()->functions();
		f->glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture(0));
		f->glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, texture(1));
		f->glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, texture(2));
		f->glActiveTexture(GL_TEXTURE0);
	}
}

void VideoRendererItem::beforeUpdate() {
	QMutexLocker locker(&d->mutex);
	if (!d->framePended)
		return;
	if (d->format != d->frame.format()) {
		d->format = d->frame.format();
		d->mposd->setFrameSize(d->format.size());
		resetNode();
		updateGeometry();
		emit formatChanged(d->format);
	}
    if (d->shaderType != d->format.type())
		resetNode();
	if (!d->format.isEmpty()) {
		const int w = d->format.byteWidth(0), h = d->format.byteHeight(0);
		auto setTex = [this] (int idx, GLenum fmt, int width, int height, const uchar *data) {
			glBindTexture(GL_TEXTURE_2D, texture(idx));
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, fmt, GL_UNSIGNED_BYTE, data);
		};
		switch (d->format.type()) {
		case VideoFormat::I420:
		case VideoFormat::YV12:
			setTex(0, GL_LUMINANCE, d->format.byteWidth(0), d->format.byteHeight(0), d->frame.data(0));
			setTex(1, GL_LUMINANCE, d->format.byteWidth(1), d->format.byteHeight(1), d->frame.data(1));
			setTex(2, GL_LUMINANCE, d->format.byteWidth(2), d->format.byteHeight(2), d->frame.data(2));
			break;
		case VideoFormat::NV12:
		case VideoFormat::NV21:
			setTex(0, GL_LUMINANCE, w, h, d->frame.data(0));
			setTex(1, GL_LUMINANCE_ALPHA, w >> 1, h >> 1, d->frame.data(1));
			break;
		case VideoFormat::YUY2:
		case VideoFormat::UYVY:
			setTex(0, GL_LUMINANCE_ALPHA, w >> 1, h, d->frame.data(0));
			setTex(1, GL_BGRA, w >> 2, h, d->frame.data(0));
			break;
		case VideoFormat::RGBA:
			setTex(0, GL_RGBA, w >> 2, h, d->frame.data(0));
			break;
		case VideoFormat::BGRA:
			setTex(0, GL_BGRA, w >> 2, h, d->frame.data(0));
			break;
		default:
			break;
		}
		++(d->drawnFrames);
	}
	d->framePended = false;
}

void VideoRendererItem::updateTexturedPoint2D(TexturedPoint2D *tp) {
	QSizeF letter(targetCropRatio(targetAspectRatio()), 1.0);
	letter.scale(width(), height(), Qt::KeepAspectRatio);
	QPointF offset = d->offset;
    offset.rx() *= letter.width()/100.0;
    offset.ry() *= letter.height()/100.0;
	QPointF xy(width(), height());
	xy.rx() -= letter.width(); xy.ry() -= letter.height();	xy *= 0.5;
	if (d->alignment & Qt::AlignLeft)
		offset.rx() -= xy.x();
	else if (d->alignment & Qt::AlignRight)
		offset.rx() += xy.x();
	if (d->alignment & Qt::AlignTop)
		offset.ry() -= xy.y();
	else if (d->alignment & Qt::AlignBottom)
		offset.ry() += xy.y();
	xy += offset;
	if (d->letterbox->set(QRectF(0.0, 0.0, width(), height()), QRectF(xy, letter)))
		emit screenRectChanged(d->letterbox->screen());
	set(tp, d->vtx.translated(offset), QRectF(0.0, 0.0, _Ratio(d->format.width(), d->format.drawWidth()), 1.0));
}

void VideoRendererItem::initializeTextures() {
	if (d->format.isEmpty())
		return;
	auto bindTex = [this] (int idx) {
			glBindTexture(GL_TEXTURE_2D, this->texture(idx));
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	};
	const int w = d->format.byteWidth(0), h = d->format.byteHeight(0);
	auto initTex = [this, &bindTex] (int idx, GLenum fmt, int width, int height) {
		bindTex(idx);
		glTexImage2D(GL_TEXTURE_2D, 0, fmt, width, height, 0, fmt, GL_UNSIGNED_BYTE, nullptr);
	};
	auto initRgbTex = [this, &bindTex, w, h] (int idx, GLenum fmt) {
		bindTex(idx);
		glTexImage2D(GL_TEXTURE_2D, 0, 4, w >> 2, h, 0, fmt, GL_UNSIGNED_BYTE, nullptr);
	};
	d->strideCorrection = QPointF(1.0, 1.0);
	switch (d->format.type()) {
	case VideoFormat::I420:
	case VideoFormat::YV12:
		initTex(0, GL_LUMINANCE, d->format.byteWidth(0), d->format.byteHeight(0));
		initTex(1, GL_LUMINANCE, d->format.byteWidth(1), d->format.byteHeight(1));
		initTex(2, GL_LUMINANCE, d->format.byteWidth(2), d->format.byteHeight(2));
		d->strideCorrection.rx() = ((double)d->format.byteWidth(0)/(double)(d->format.byteWidth(1)*2));
		d->strideCorrection.ry() = ((double)d->format.byteWidth(0)/(double)(d->format.byteWidth(2)*2));
		break;
	case VideoFormat::NV12:
	case VideoFormat::NV21:
		initTex(0, GL_LUMINANCE, d->format.byteWidth(0), d->format.byteHeight(0));
		initTex(1, GL_LUMINANCE_ALPHA, d->format.byteWidth(1) >> 1, d->format.byteHeight(1));
		d->strideCorrection.rx() = ((double)d->format.byteWidth(0)/(double)(d->format.byteWidth(1)));
		break;
	case VideoFormat::YUY2:
	case VideoFormat::UYVY:
		initTex(0, GL_LUMINANCE_ALPHA, w >> 1, h);
		initTex(1, GL_RGBA, w >> 2, h);
		break;
	case VideoFormat::RGBA:
		initRgbTex(0, GL_RGBA);
		break;
	case VideoFormat::BGRA:
		initRgbTex(0, GL_BGRA);
		break;
	default:
		break;
	}
}

QQuickItem *VideoRendererItem::osd() const {
	return d->mposd;
}
