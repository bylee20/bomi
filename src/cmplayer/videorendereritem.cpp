#include "videorendereritem.hpp"
#include "hwacc_vaapi.hpp"
#include "mposditem.hpp"
#include "videoframe.hpp"
#include "shadervar.h"
#include "global.hpp"

struct VideoRendererItem::Data {
	VideoFrame frame;
	bool framePended = false, checkFormat = true, shaderChanged = false;
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
	QImage image;
	bool take = false;
#ifdef Q_OS_LINUX
	VaApiSurfaceGLX *vaapi = nullptr;
#endif
};

static const QEvent::Type UpdateEvent = (QEvent::Type)(QEvent::User+1);

VideoRendererItem::VideoRendererItem(QQuickItem *parent)
: TextureRendererItem(3, parent), d(new Data) {
	setFlags(ItemHasContents | ItemAcceptsDrops);
	d->mposd = new MpOsdItem(this);
	d->letterbox = new LetterboxItem(this);
	setZ(-1);
}

VideoRendererItem::~VideoRendererItem() {
#ifdef Q_OS_LINUX
	delete d->vaapi;
#endif
	delete d;
}

void VideoRendererItem::customEvent(QEvent *event) {
	if (event->type() == UpdateEvent)
		update();
}

QQuickItem *VideoRendererItem::overlay() const {
	return d->overlay;
}

bool VideoRendererItem::isFramePended() const {
	return d->framePended;
}

const VideoFrame &VideoRendererItem::frame() const {
	return d->frame;
}

bool VideoRendererItem::hasFrame() const {
	return !d->format.isEmpty();
}

void VideoRendererItem::requestFrameImage() const {
	if (d->format.isEmpty())
		emit frameImageObtained(QImage());
	else if (!d->image.isNull())
		emit frameImageObtained(d->image);
	else if (d->format.type() != VideoFormat::VAGL) {
		d->mutex.lock();
		QImage image = d->frame.toImage();
		d->mutex.unlock();
		d->mposd->drawOn(image);
		emit frameImageObtained(image);
	} else {
		Q_ASSERT(d->format.type() == VideoFormat::VAGL);
		d->take = true;
		const_cast<VideoRendererItem*>(this)->update();
	}
}

void VideoRendererItem::present(const QImage &image) {
	if (d->image.isNull() && image.isNull())
		return;
	if (image.format() != QImage::Format_ARGB32_Premultiplied)
		d->image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
	else
		d->image = image;
	d->framePended = true;
	update();
}

void VideoRendererItem::present(const VideoFrame &frame, bool checkFormat) {
	d->mutex.lock();
	d->image = QImage();
	d->frame = frame;
	d->framePended = true;
	d->checkFormat = checkFormat;
	d->mutex.unlock();
	qApp->postEvent(this, new QEvent(UpdateEvent));
	d->mposd->present();
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
	return _Ratio(d->format.outputSize());
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
		updateGeometry();
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
		d->shaderChanged = d->shaderVar.setEffects(effects);
		setGeometryDirty();
		update();
	}
}

QRectF VideoRendererItem::frameRect(const QRectF &area) const {
	if (!d->format.isEmpty()) {
		const double aspect = targetAspectRatio();
		QSizeF frame(aspect, 1.0), letter(targetCropRatio(aspect), 1.0);
		letter.scale(area.width(), area.height(), Qt::KeepAspectRatio);
		frame.scale(letter, Qt::KeepAspectRatioByExpanding);
		QPointF pos(area.x(), area.y());
		pos.rx() += (area.width() - frame.width())*0.5;
		pos.ry() += (area.height() - frame.height())*0.5;
		qDebug() << pos;
		return QRectF(pos, frame);
	} else
		return area;
}

void VideoRendererItem::updateGeometry() {
	const QRectF vtx = frameRect(QRectF(x(), y(), width(), height()));
	if (_Change(d->vtx, vtx))
		d->mposd->setSize(d->vtx.size());
	setGeometryDirty();
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

QSize VideoRendererItem::sizeHint() const {
	if (d->format.isEmpty())
		return QSize(400, 300);
	const double aspect = targetAspectRatio();
	QSizeF size(aspect, 1.0);
	size.scale(d->format.outputSize(), Qt::KeepAspectRatioByExpanding);
	QSizeF crop(targetCropRatio(aspect), 1.0);
	crop.scale(size, Qt::KeepAspectRatio);
	return crop.toSize();
}

const char *VideoRendererItem::fragmentShader() const {
    d->shaderType = d->format.type();
	d->shader = d->shaderVar.fragment(d->shaderType);
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
	reinterpret_cast<VideoRendererItem*>(pctx)->d->mposd->drawOn(imgs);
}

void VideoRendererItem::bind(const RenderState &state, QOpenGLShaderProgram *program) {
	TextureRendererItem::bind(state, program);
	program->setUniformValue(d->loc_p1, 0);
	program->setUniformValue(d->loc_p2, 1);
	program->setUniformValue(d->loc_p3, 2);
	program->setUniformValue(d->loc_brightness, d->shaderVar.brightness);
	program->setUniformValue(d->loc_contrast, d->shaderVar.contrast);
	program->setUniformValue(d->loc_sat_hue, d->shaderVar.sat_hue);
	const float dx = 1.0/(double)d->format.alignedWidth();
	const float dy = 1.0/(double)d->format.alignedHeight();
	program->setUniformValue(d->loc_dxy, dx, dy, -dx, 0.f);
	program->setUniformValue(d->loc_strideCorrection, d->strideCorrection);
	const auto effects = d->shaderVar.effects();
	const bool filter = effects & FilterEffects;
	const bool kernel = effects & KernelEffects;
	if (filter || kernel) {
		program->setUniformValue(d->loc_rgb_c, d->shaderVar.rgb_c[0], d->shaderVar.rgb_c[1], d->shaderVar.rgb_c[2]);
		program->setUniformValue(d->loc_rgb_0, d->shaderVar.rgb_0);
	}
	if (kernel) {
		program->setUniformValue(d->loc_kern_c, d->shaderVar.kern_c);
		program->setUniformValue(d->loc_kern_n, d->shaderVar.kern_n);
		program->setUniformValue(d->loc_kern_d, d->shaderVar.kern_d);
	}
	if (effects & RemapLuma) {
		const float y_tan = 1.0/d->shaderVar.m_luma.difference();
		program->setUniformValue(d->loc_y_tan, y_tan);
		program->setUniformValue(d->loc_y_b, (float)-d->shaderVar.m_luma.min);
	} else {
		program->setUniformValue(d->loc_y_tan, 1.0f);
		program->setUniformValue(d->loc_y_b, 0.0f);
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
	bool reset = false;
	if (!d->image.isNull()) {
		const VideoFormat format(d->image);
		if (format != d->format) {
			reset = true;
			d->format = format;
			resetNode();
			updateGeometry();
		}
	} else if (d->checkFormat && d->format != d->frame.format()) {
		reset = true;
		d->format = d->frame.format();
		d->mposd->setFrameSize(d->format.size());
		resetNode();
		updateGeometry();
	}
	if (!reset && (d->shaderChanged || d->shaderType != d->format.type()))
		resetNode();
	if (!d->format.isEmpty()) {
		auto setTex = [this] (int idx, GLenum fmt, int width, int height, const uchar *data) {
			glBindTexture(GL_TEXTURE_2D, texture(idx));
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, fmt, d->format.glFormat(), data);
		};
		if (d->image.isNull()) {
			const int w = d->format.bytesPerLine(0), h = d->format.lines(0);
			switch (d->format.type()) {
			case VideoFormat::I420:
				setTex(0, GL_LUMINANCE, w, h, d->frame.data(0));
				setTex(1, GL_LUMINANCE, d->format.bytesPerLine(1), d->format.lines(1), d->frame.data(1));
				setTex(2, GL_LUMINANCE, d->format.bytesPerLine(2), d->format.lines(2), d->frame.data(2));
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
#ifdef Q_OS_LINUX
			case VideoFormat::VAGL:
				glBindTexture(GL_TEXTURE_2D, texture(0));
				if (!d->vaapi)
					d->vaapi = new VaApiSurfaceGLX(texture(0));
				d->vaapi->copy(d->frame.data(3));
				if (d->take) {
					QImage image(d->format.alignedSize(), QImage::Format_ARGB32);
					glBindTexture(GL_TEXTURE_2D, texture(0));
					glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, image.bits());
					d->mposd->drawOn(image);
					emit frameImageObtained(image);
					d->take = false;
				}
				break;
#endif
			default:
				break;
			}
		} else
			setTex(0, GL_BGRA, d->image.width(), d->image.height(), d->image.bits());
		++d->drawnFrames;
	}
	d->shaderChanged = false;
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
	double top = 0.0, left = 0.0, right = _Ratio(d->format.width(), d->format.alignedWidth()), bottom = 1.0;
	if (!(d->shaderVar.effects() & IgnoreEffect)) {
		if (d->shaderVar.effects() & FlipVertically)
			qSwap(top, bottom);
		if (d->shaderVar.effects() & FlipHorizontally)
			qSwap(left, right);
	}
	auto vtx = d->vtx.translated(offset);
	d->mposd->setPosition(vtx.topLeft());
	set(tp, vtx, QRectF(left, top, right-left, bottom-top));
}

void VideoRendererItem::initializeTextures() {
#ifdef Q_OS_LINUX
	if (d->vaapi) {
		delete d->vaapi;
		d->vaapi = nullptr;
	}
#endif
	if (d->format.isEmpty())
		return;
	auto bindTex = [this] (int idx) {
			glBindTexture(GL_TEXTURE_2D, this->texture(idx));
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	};
	const int w = d->format.bytesPerLine(0), h = d->format.lines(0);
	auto initTex = [this, &bindTex] (int idx, GLenum fmt, int width, int height) {
		bindTex(idx);
		glTexImage2D(GL_TEXTURE_2D, 0, fmt, width, height, 0, fmt, d->format.glFormat(), nullptr);
	};
	auto initRgbTex = [this, &bindTex, w, h] (int idx, GLenum fmt) {
		bindTex(idx);
		glTexImage2D(GL_TEXTURE_2D, 0, 4, w >> 2, h, 0, fmt, d->format.glFormat(), nullptr);
	};
	d->strideCorrection = QPointF(1.0, 1.0); // texel correction for second and third planar
	// occasionally, the stride does not match exactly, e.g, for I420, stride[1]*2 != stride[0]
	switch (d->format.type()) {
	case VideoFormat::I420:
		initTex(0, GL_LUMINANCE, w, h);
		initTex(1, GL_LUMINANCE, d->format.bytesPerLine(1), d->format.lines(1));
		initTex(2, GL_LUMINANCE, d->format.bytesPerLine(2), d->format.lines(2));
		d->strideCorrection.rx() = ((double)w/(double)(d->format.bytesPerLine(1)*2));
		d->strideCorrection.ry() = ((double)w/(double)(d->format.bytesPerLine(2)*2));
		break;
	case VideoFormat::NV12:
	case VideoFormat::NV21:
		initTex(0, GL_LUMINANCE, w, h);
		initTex(1, GL_LUMINANCE_ALPHA, d->format.bytesPerLine(1) >> 1, d->format.lines(1));
		d->strideCorrection.rx() = ((double)w/(double)(d->format.bytesPerLine(1)));
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
#ifdef Q_OS_LINUX
	case VideoFormat::VAGL:
		initRgbTex(0, GL_BGRA);
		d->vaapi = new VaApiSurfaceGLX(texture(0));
		break;
#endif
	default:
		break;
	}
}

QQuickItem *VideoRendererItem::osd() const {
	return d->mposd;
}

void VideoRendererItem::setLumaRange(int min, int max) {
	d->shaderVar.m_luma = RangeF((double)min/255.0, (double)max/255.0);
	update();
}

void VideoRendererItem::setKernel(int blur_c, int blur_n, int blur_d, int sharpen_c, int sharpen_n, int sharpen_d) {
	d->shaderVar.setKernel(blur_c, blur_n, blur_d, sharpen_c, sharpen_n, sharpen_d);
	update();
}
