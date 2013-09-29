#include "videorendereritem.hpp"
#include "mposditem.hpp"
#include "videoframe.hpp"
#include "hwacc.hpp"
#include "global.hpp"
#include "letterboxitem.hpp"
#include "dataevent.hpp"
#include "videoframeshader.hpp"
#include "videorenderershader.glsl.hpp"
#include <atomic>

template<typename T>
class SyncValue {
public:
	SyncValue() = default;
	SyncValue(const T &t): m_t(t), m_new(t) {}
	const T &get() const { return m_t; }
	const T &pended() const { return m_new; }
	bool pend(const T &new_) { m_new = new_; return m_new != m_t; }
	bool update() { return _Change(m_t, m_new); }
	bool replace(const T &new_) { m_new = new_; return update(); }
private:
	T m_t, m_new;
};

struct VideoRendererItem::Data {
	Data(VideoRendererItem *p): p(p) {}
	VideoRendererItem *p = nullptr;
	QLinkedList<VideoFrame> queue;
	OpenGLFramebufferObject *fbo = nullptr;
	bool take = false, initialized = false, render = false;
	quint64 drawnFrames = 0;
	QRectF vtx;
	QPoint offset = {0, 0};
	double crop = -1.0, aspect = -1.0, dar = 0.0, ptsIn = MP_NOPTS_VALUE, ptsOut = MP_NOPTS_VALUE;
	int alignment = Qt::AlignCenter;
	LetterboxItem *letterbox = nullptr;
	MpOsdItem *mposd = nullptr;
	QQuickItem *overlay = nullptr;
	VideoFrameShader *shader = nullptr;
	Kernel3x3 blur, sharpen, kernel;
	Effects effects = 0;
	ColorProperty color;
	DeintInfo deint;
	SyncValue<InterpolatorType> interpolator = InterpolatorType::Bilinear;
	int loc_vMatrix = -1, loc_tex = -1, loc_lut_interpolator = -1;
	QByteArray fragCode, vertexCode;
	OpenGLTexture lutInterpolator, black;
	QSize displaySize{1, 1}, frameSize{0, 0};
	QByteArray header() const {
		QByteArray header;
		header += "const float texWidth = " + QByteArray::number(fbo ? fbo->width() : 1) + ".0;\n";
		header += "const float texHeight = " + QByteArray::number(fbo ? fbo->height() : 1) + ".0;\n";
		header += "#define USE_INTERPOLATOR " + QByteArray::number((int)interpolator.get()) + "\n";
		return header;
	}
	void repaint() { render = true; p->update(); }
	void fillKernel() {
		kernel = Kernel3x3();
		if (effects & Blur)
			kernel += blur;
		if (effects & Sharpen)
			kernel += sharpen;
		kernel.normalize();
	}
};

VideoRendererItem::VideoRendererItem(QQuickItem *parent)
: TextureRendererItem(1, parent), d(new Data(this)) {
	setFlags(ItemHasContents | ItemAcceptsDrops);
	d->mposd = new MpOsdItem(this);
	d->letterbox = new LetterboxItem(this);
	setZ(-1);
	setBechmark(true);
}

VideoRendererItem::~VideoRendererItem() {
	delete d;
}

void VideoRendererItem::initializeGL() {
	Q_ASSERT(QOpenGLContext::currentContext() != nullptr);
	d->lutInterpolator.generate();
	_Renew(d->shader);
	d->shader->setDeintInfo(d->deint);
	d->shader->setColor(d->color);
	d->shader->setEffects(d->effects);
	d->black = OpenGLCompat::makeTexture(1, 1, GL_BGRA);
	const quint32 p = 0x0;
	d->black.upload2D(&p);
	d->initialized = true;
}

void VideoRendererItem::finalizeGL() {
	Q_ASSERT(QOpenGLContext::currentContext() != nullptr);
	d->lutInterpolator.delete_();
	d->black.delete_();
	_Delete(d->fbo);
	_Delete(d->shader);
	d->initialized = false;
}

void VideoRendererItem::customEvent(QEvent *event) {
	switch ((int)event->type()) {
	case NewFrame:
		if (d->queue.size() < 3)
			d->queue.push_back(getData<VideoFrame>(event));
		update();
		break;
	case NextFrame:
		update();
		break;
	case UpdateDeint: {
		auto deint = getData<DeintInfo>(event);
		if (!deint.isHardware())
			deint = DeintInfo();
		if (_Change(d->deint, deint) && d->shader) {
			d->shader->setDeintInfo(d->deint);
			d->repaint();
		}
		break;
	} default:
		break;
	}
}

QQuickItem *VideoRendererItem::overlay() const {
	return d->overlay;
}

bool VideoRendererItem::hasFrame() const {
	return d->shader && !d->frameSize.isEmpty();
}

void VideoRendererItem::requestFrameImage() const {
	if (!hasFrame())
		emit frameImageObtained(QImage());
	else {
		d->take = true;
		const_cast<VideoRendererItem*>(this)->update();
	}
}

void VideoRendererItem::present(const QImage &image) {
	present(VideoFrame(image));
}

void VideoRendererItem::present(const VideoFrame &frame, bool redraw) {
	if (!d->initialized)
		return;
	if (d->shader && d->queue.size() < 3)
		postData(this, NewFrame, frame);
	d->mposd->present(redraw);
	d->ptsIn = frame.pts();
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
		updateGeometry(false);
		update();
	}
}

void VideoRendererItem::setDeint(const DeintInfo &deint) {
	postData(this, UpdateDeint, deint);
}

double VideoRendererItem::targetAspectRatio() const {
	if (d->aspect > 0.0)
		return d->aspect;
	if (d->aspect == 0.0)
		return itemAspectRatio();
	return hasFrame() ? _Ratio(d->displaySize) : 1.0;
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
	updateGeometry(false);
}

void VideoRendererItem::setOffset(const QPoint &offset) {
	if (d->offset != offset) {
		d->offset = offset;
		emit offsetChanged(d->offset);
		updateGeometry(false);
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
	return d->effects;
}

void VideoRendererItem::setEffects(Effects effects) {
	if (_Change(d->effects, effects)) {
		if (d->shader)
			d->shader->setEffects(d->effects);
		d->fillKernel();
		update();
	}
}

QRectF VideoRendererItem::frameRect(const QRectF &area) const {
	if (hasFrame()) {
		const double aspect = targetAspectRatio();
		QSizeF frame(aspect, 1.0), letter(targetCropRatio(aspect), 1.0);
		letter.scale(area.width(), area.height(), Qt::KeepAspectRatio);
		frame.scale(letter, Qt::KeepAspectRatioByExpanding);
		QPointF pos(area.x(), area.y());
		pos.rx() += (area.width() - frame.width())*0.5;
		pos.ry() += (area.height() - frame.height())*0.5;
		return QRectF(pos, frame);
	} else
		return area;
}

void VideoRendererItem::updateGeometry(bool updateOsd) {
	const QRectF vtx = frameRect(QRectF(x(), y(), width(), height()));
	if (_Change(d->vtx, vtx)) {
		if (d->vtx.size() != QSizeF(d->mposd->width(), d->mposd->height())) {
			if (updateOsd)
				d->mposd->forceUpdateTargetSize();
			d->mposd->setSize(d->vtx.size());
		}
	}
	setGeometryDirty();
}

void VideoRendererItem::setColor(const ColorProperty &prop) {
	if (_Change(d->color, prop) && d->shader) {
		d->shader->setColor(d->color);
		d->repaint();
	}
}

const ColorProperty &VideoRendererItem::color() const {
	return d->color;
}

void VideoRendererItem::setAspectRatio(double ratio) {
	if (!isSameRatio(d->aspect, ratio)) {
		d->aspect = ratio;
		updateGeometry(true);
		update();
	}
}

double VideoRendererItem::aspectRatio() const {
	return d->aspect;
}

void VideoRendererItem::setCropRatio(double ratio) {
	if (!isSameRatio(d->crop, ratio)) {
		d->crop = ratio;
		updateGeometry(true);
		update();
	}
}

double VideoRendererItem::cropRatio() const {
	return d->crop;
}

QSize VideoRendererItem::sizeHint() const {
	if (!hasFrame())
		return QSize(400, 300);
	const double aspect = targetAspectRatio();
	QSizeF size(aspect, 1.0);
	size.scale(d->displaySize, Qt::KeepAspectRatioByExpanding);
	QSizeF crop(targetCropRatio(aspect), 1.0);
	crop.scale(size, Qt::KeepAspectRatio);
	return crop.toSize();
}

static const QByteArray commonCode((char*)videorenderershader_glsl, videorenderershader_glsl_len);

const char *VideoRendererItem::fragmentShader() const {
	d->fragCode = d->header();
	d->fragCode += "#define FRAGMENT\n";
	d->fragCode += commonCode;
	return d->fragCode.constData();
}

const char *VideoRendererItem::vertexShader() const {
	d->vertexCode = d->header();
	d->vertexCode += "#define VERTEX\n";
	d->vertexCode += commonCode;
	return d->vertexCode.constData();
}

const char * const *VideoRendererItem::attributeNames() const {
	static const char *const names[] = { "vPosition", "vCoord", nullptr };
	return names;
}

void VideoRendererItem::link(QOpenGLShaderProgram *prog) {
	d->loc_tex = prog->uniformLocation("tex");
	d->loc_vMatrix = prog->uniformLocation("vMatrix");
	d->loc_lut_interpolator = prog->uniformLocation("lut_interpolator");
}

MpOsdItem *VideoRendererItem::mpOsd() const {
	return d->mposd;
}

void VideoRendererItem::bind(const RenderState &state, QOpenGLShaderProgram *prog) {
	prog->setUniformValue(d->loc_vMatrix, state.combinedMatrix());
	prog->setUniformValue(d->loc_tex, 0);
	prog->setUniformValue(d->loc_lut_interpolator, 1);
	auto f = OpenGLCompat::functions();
	f->glActiveTexture(GL_TEXTURE0);
	if (d->fbo)
		d->fbo->texture().bind();
	else
		d->black.bind();
	if (d->interpolator.get() > 0) {
		f->glActiveTexture(GL_TEXTURE1);
		d->lutInterpolator.bind();
		f->glActiveTexture(GL_TEXTURE0);
	}
}

int VideoRendererItem::delay() const {
	return (d->ptsIn == MP_NOPTS_VALUE || d->ptsOut == MP_NOPTS_VALUE) ? 0.0 : (d->ptsOut - d->ptsIn)*1000.0;
}

void VideoRendererItem::beforeUpdate() {
	Q_ASSERT(d->shader);

	if (d->take) {
		if (d->fbo) {
			auto image = d->fbo->toImage();
			d->mposd->drawOn(image);
			emit frameImageObtained(image);
		} else
			emit frameImageObtained(QImage());
		d->take = false;
	}

	bool reset = false;
	if (!d->queue.isEmpty()) {
		auto &frame = d->queue.front();
		if (!frame.format().isEmpty()) {
			d->ptsOut = frame.pts();
			d->render = true;
			if (_Change(d->frameSize, frame.format().size()))
				reset = true;
			if (_Change(d->displaySize, frame.format().displaySize()))
				updateGeometry(true);
			d->shader->upload(frame);
			++d->drawnFrames;
		}
		d->queue.pop_front();
		if (!d->queue.isEmpty()) {
			if (d->queue.size() > 3)
				d->queue.clear();
			else
				postData(this, NextFrame);
		}
	}
	if (d->interpolator.update()) {
		d->lutInterpolator = OpenGLCompat::allocateInterpolatorLutTexture(d->lutInterpolator.id, d->interpolator.get());
		reset = true;
	}
	if (reset)
		resetNode();

	if (d->render && !d->frameSize.isEmpty()) {
		if (!d->fbo || d->fbo->size() != d->frameSize)
			_Renew(d->fbo, d->frameSize, GL_TEXTURE_2D);
		d->fbo->bind();
		d->shader->render(d->kernel);
		d->fbo->release();
	}
	d->render = false;
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
	double top = 0.0, left = 0.0, right = 1.0, bottom = 1.0;
	if (d->fbo)
		d->fbo->getCoords(left, top, right, bottom);
	if (!(d->effects & Disable)) {
		if (d->effects & FlipVertically)
			qSwap(top, bottom);
		if (d->effects & FlipHorizontally)
			qSwap(left, right);
	}
	auto vtx = d->vtx.translated(offset);
	d->mposd->setPosition(vtx.topLeft());
	set(tp, vtx, QRectF(left, top, right-left, bottom-top));
}

QQuickItem *VideoRendererItem::osd() const {
	return d->mposd;
}

void VideoRendererItem::setKernel(int blur_c, int blur_n, int blur_d, int sharpen_c, int sharpen_n, int sharpen_d) {
	d->blur.set(blur_c, blur_n, blur_d);
	d->sharpen.set(sharpen_c, sharpen_n, sharpen_d);
	d->fillKernel();
	d->repaint();
}

void VideoRendererItem::setInterpolator(InterpolatorType interpolator) {
	if (d->interpolator.pend(interpolator))
		d->repaint();
}
