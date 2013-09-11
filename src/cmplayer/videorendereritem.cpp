#include "videorendereritem.hpp"
#include "mposditem.hpp"
#include "videoframe.hpp"
#include "hwacc.hpp"
#include "global.hpp"
#include "letterboxitem.hpp"
#include "videotextureshader.hpp"
#include "dataevent.hpp"

struct VideoRendererItem::Data {
	VideoFrame frame;
	VideoFormat format;
	quint64 drawnFrames = 0;
	bool take = false;
	QRectF vtx;
	QPoint offset = {0, 0};
	double crop = -1.0, aspect = -1.0, dar = 0.0;
	int alignment = Qt::AlignCenter;
	LetterboxItem *letterbox = nullptr;
	MpOsdItem *mposd = nullptr;
	QQuickItem *overlay = nullptr;
	VideoTextureShader *shader = nullptr;
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
	QLinkedList<VideoFrame> queue;
	DeintInfo deint;
	bool deintChanged = false;
};

VideoRendererItem::VideoRendererItem(QQuickItem *parent)
: TextureRendererItem(3, parent), d(new Data) {
	setFlags(ItemHasContents | ItemAcceptsDrops);
	d->mposd = new MpOsdItem(this);
	d->letterbox = new LetterboxItem(this);
	d->shader = VideoTextureShader::create(d->format);
	setZ(-1);
}

VideoRendererItem::~VideoRendererItem() {
	delete d->shader;
	delete d;
}

void VideoRendererItem::customEvent(QEvent *event) {
	switch ((int)event->type()) {
	case EnqueueFrame:
		d->queue.push_back(getData<VideoFrame>(event));
		update();
		break;
	case RenderNextFrame:
		update();
		break;
	case EmptyQueue:
		d->queue.clear();
		break;
	case UpdateDeint: {
		auto deint = getData<DeintInfo>(event);
		if (!deint.isHardware())
			deint = DeintInfo();
		if ((d->deintChanged = _Change(d->deint, deint)))
			update();
		break;
	} default:
		break;
	}
}

QQuickItem *VideoRendererItem::overlay() const {
	return d->overlay;
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
	else if (d->frame.hasImage())
		emit frameImageObtained(d->frame.toImage());
	else {
		d->take = true;
		const_cast<VideoRendererItem*>(this)->update();
	}
}

void VideoRendererItem::present(const QImage &image) {
	present(VideoFrame(image));
}

void VideoRendererItem::present(const VideoFrame &frame) {
	postData(this, EnqueueFrame, frame);
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
	return _Ratio(d->format.displaySize());
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
	if (d->format.isEmpty())
		return QSize(400, 300);
	const double aspect = targetAspectRatio();
	QSizeF size(aspect, 1.0);
	size.scale(d->format.displaySize(), Qt::KeepAspectRatioByExpanding);
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

MpOsdItem *VideoRendererItem::mpOsd() const {
	return d->mposd;
}

//void VideoRendererItem::drawMpOsd(void *pctx, sub_bitmaps *imgs) {
//	reinterpret_cast<VideoRendererItem*>(pctx)->d->mposd->drawOn(imgs);
//}

void VideoRendererItem::bind(const RenderState &state, QOpenGLShaderProgram *program) {
	TextureRendererItem::bind(state, program);
	d->shader->render(program, d->kernel);
}

void VideoRendererItem::emptyQueue() {
	postData(this, EmptyQueue);
}

int VideoRendererItem::delay() const {
	return d->queue.isEmpty() ? 0 : (d->queue.back().pts() - d->queue.front().pts())*1000.0;
}

void VideoRendererItem::beforeUpdate() {
	if (d->queue.isEmpty())
		return;
	d->frame = d->queue.takeFirst();
	const bool formatChanged = _Change(d->format, d->frame.format());
	if (formatChanged || d->shaderType != d->format.imgfmt() || d->deintChanged) {
		if (d->shader)
			delete d->shader;
		d->shader = VideoTextureShader::create(d->format, d->color, d->deint, d->effects);
		resetNode();
		updateGeometry(formatChanged);
	}
	if (!d->format.isEmpty()) {
		d->shader->upload(d->frame);
		++d->drawnFrames;
		if (d->take) {
			auto image = d->shader->toImage(d->frame);
//			d->mposd->drawOn(image);
			emit frameImageObtained(image);
			d->take = false;
		}
	}
	if (!d->queue.isEmpty()) {
		const auto diff = (d->queue.back().pts() - d->queue.front().pts());
		if (diff < 0.1) {
			postData(this, RenderNextFrame);
		} else {
			qDebug() << "Too many frames are queued! Drop them...";
			d->queue.clear();
		}
	}
	d->deintChanged = false;
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
	if (d->frame.field() & VideoFrame::Flipped)
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
