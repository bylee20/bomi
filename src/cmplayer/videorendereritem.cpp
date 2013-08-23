#include "videorendereritem.hpp"
#include "mposditem.hpp"
#include "videoframe.hpp"
#include "global.hpp"
#include "textureshader.hpp"

struct VideoRendererItem::Data {
	VideoFrame frame;
	VideoFormat format;
	quint64 drawnFrames = 0;
	bool framePended = false, checkFormat = true, take = false, flipped = false;
	QRectF vtx;
	QPoint offset = {0, 0};
	double crop = -1.0, aspect = -1.0, dar = 0.0;
	int alignment = Qt::AlignCenter;
	LetterboxItem *letterbox = nullptr;
	MpOsdItem *mposd = nullptr;
	QQuickItem *overlay = nullptr;
	QMutex mutex;
	TextureShader *shader = nullptr;
	QByteArray shaderCode;
	VideoFormat::Type shaderType = IMGFMT_BGRA;
	Kernel3x3 blur, sharpen, kernel;
	Effects effects = 0;
	ColorProperty color;
	void updateKernel() {
		kernel = Kernel3x3();
		if (effects & Blur)
			kernel += blur;
		if (effects & Sharpen)
			kernel += sharpen;
		kernel.normalize();
	}
};

static const QEvent::Type UpdateEvent = (QEvent::Type)(QEvent::User+1);

VideoRendererItem::VideoRendererItem(QQuickItem *parent)
: TextureRendererItem(3, parent), d(new Data) {
	setFlags(ItemHasContents | ItemAcceptsDrops);
	d->mposd = new MpOsdItem(this);
	d->letterbox = new LetterboxItem(this);
	d->shader = TextureShader::create(d->format);
	setZ(-1);
}

VideoRendererItem::~VideoRendererItem() {
	delete d->shader;
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
	else if (d->frame.hasImage() || !d->frame.format().isNative())
		emit frameImageObtained(d->frame.toImage());
	else {
		d->take = true;
		const_cast<VideoRendererItem*>(this)->update();
	}
}

void VideoRendererItem::present(const QImage &image) {
	present(VideoFrame(image), false, true);
}

void VideoRendererItem::present(const VideoFrame &frame, bool flipped, bool checkFormat) {
	d->mutex.lock();
	d->frame = frame;
	d->flipped = flipped;
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
	return d->effects;
}

void VideoRendererItem::setEffects(Effects effects) {
	if (_Change(d->effects, effects)) {
		if (!d->shader->setEffects(d->effects))
			d->shaderType = IMGFMT_NONE;
		d->updateKernel();
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
	if (_Change(d->color, prop)) {
		d->shader->setColor(d->color);
		update();
	}
}

const ColorProperty &VideoRendererItem::color() const {
	return d->color;
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
	d->shaderType = d->format.imgfmt();
	d->shaderCode = d->shader->fragment();
	return d->shaderCode.constData();
}

void VideoRendererItem::link(QOpenGLShaderProgram *program) {
	TextureRendererItem::link(program);
	Q_ASSERT(d->shader);
	d->shader->link(program);
}

void VideoRendererItem::drawMpOsd(void *pctx, sub_bitmaps *imgs) {
	reinterpret_cast<VideoRendererItem*>(pctx)->d->mposd->drawOn(imgs);
}

void VideoRendererItem::bind(const RenderState &state, QOpenGLShaderProgram *program) {
	TextureRendererItem::bind(state, program);
	d->shader->render(program, d->kernel);
}

void VideoRendererItem::beforeUpdate() {
	QMutexLocker locker(&d->mutex);
	bool reset = false;
	if ((reset = (d->checkFormat && _Change(d->format, d->frame.format()))))
		d->mposd->setFrameSize(d->format.size());
	if (!reset && d->shaderType != d->format.imgfmt())
		reset = true;
	if (reset) {
		if (d->shader)
			delete d->shader;
		d->shader = TextureShader::create(d->format, d->color, d->effects);
		resetNode();
		updateGeometry();
	}
	if ((d->framePended) && !d->format.isEmpty()) {
		d->shader->upload(d->frame);
		++d->drawnFrames;
		if (d->take) {
			auto image = d->shader->toImage(d->frame);
			d->mposd->drawOn(image);
			emit frameImageObtained(image);
			d->take = false;
		}
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
	double top, left, right, bottom;
	d->shader->getCoords(left, top, right, bottom);
	if (!(d->effects & Disable)) {
		if (d->effects & FlipVertically)
			qSwap(top, bottom);
		if (d->effects & FlipHorizontally)
			qSwap(left, right);
	}
	if (d->flipped)
		qSwap(top, bottom);
	auto vtx = d->vtx.translated(offset);
	d->mposd->setPosition(vtx.topLeft());
	set(tp, vtx, QRectF(left, top, right-left, bottom-top));
}

void VideoRendererItem::initializeTextures() {
	if (d->format.isEmpty())
		return;
	Q_ASSERT(d->shader != nullptr);
	d->shader->initialize(textures());
}

QQuickItem *VideoRendererItem::osd() const {
	return d->mposd;
}

void VideoRendererItem::setKernel(int blur_c, int blur_n, int blur_d, int sharpen_c, int sharpen_n, int sharpen_d) {
	d->blur.set(blur_c, blur_n, blur_d);
	d->sharpen.set(sharpen_c, sharpen_n, sharpen_d);
	d->updateKernel();
	update();
}
