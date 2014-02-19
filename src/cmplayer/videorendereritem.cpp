#include "videorendereritem.hpp"
#include "mposditem.hpp"
#include "videoframe.hpp"
#include "hwacc.hpp"
#include "global.hpp"
#include "letterboxitem.hpp"
#include "dataevent.hpp"
#include "videoframeshader.hpp"

struct VideoRendererItem::Data {
	Data(VideoRendererItem *p): p(p) {}
	QSGMaterialType shaderIds[2];
	VideoRendererItem *p = nullptr;
	QLinkedList<VideoFrame> queue;
	OpenGLFramebufferObject *fbo = nullptr;
	bool take = false, initialized = false, render = false, direct = false;
	quint64 drawnFrames = 0, lastCheckedFrames = 0, lastCheckedTime = 0;
	QRectF vtx;
	QPoint offset = {0, 0};
	double crop = -1.0, aspect = -1.0, dar = 0.0, ptsIn = MP_NOPTS_VALUE, ptsOut = MP_NOPTS_VALUE;
	int alignment = Qt::AlignCenter;
	LetterboxItem *letterbox = nullptr;
	MpOsdItem *mposd = nullptr;
	GeometryItem *overlay = nullptr;
	VideoFrameShader *shader = nullptr;
	Kernel3x3 blur, sharpen, kernel;
	ColorRange range = ColorRange::Auto;
	Effects effects = 0;
	VideoColor color;
	DeintMethod deint = DeintMethod::None;
	OpenGLTexture2D black;
	QSize displaySize{1, 1}, frameSize{0, 0};
	InterpolatorType chromaUpscaler = InterpolatorType::Bilinear;
    int dropped = 0, fboDepth = 2;
	bool overlayInLetterbox = true;
	void repaint() { render = true; p->update(); }
	void fillKernel() {
		kernel = Kernel3x3();
		if (effects & Blur)
			kernel += blur;
		if (effects & Sharpen)
			kernel += sharpen;
		kernel.normalize();
	}
	QRectF frameRect(const QRectF &area, const QPoint &off = {0, 0}, QRectF *letterbox = nullptr) {
		QRectF rect = area;
		if (p->hasFrame()) {
			const double aspect = p->targetAspectRatio();
			QSizeF frame(aspect, 1.0), letter(p->targetCropRatio(aspect), 1.0);
			letter.scale(area.width(), area.height(), Qt::KeepAspectRatio);
			frame.scale(letter, Qt::KeepAspectRatioByExpanding);
			QPointF pos(area.x(), area.y());
			pos.rx() += (area.width() - frame.width())*0.5;
			pos.ry() += (area.height() - frame.height())*0.5;
			rect = {pos, frame};

			QPointF xy(area.width(), area.height());
			xy.rx() -= letter.width(); xy.ry() -= letter.height();	xy *= 0.5;
			QPointF offset = off;
			offset.rx() *= letter.width()/100.0;
			offset.ry() *= letter.height()/100.0;
			if (alignment & Qt::AlignLeft)
				offset.rx() -= xy.x();
			else if (alignment & Qt::AlignRight)
				offset.rx() += xy.x();
			if (alignment & Qt::AlignTop)
				offset.ry() -= xy.y();
			else if (alignment & Qt::AlignBottom)
				offset.ry() += xy.y();
			rect.translate(offset);
			xy += offset;
			if (letterbox)
				*letterbox = {xy, letter};
		}
		return rect;
	}

	void updateGeometry(bool forceUpdateOsd) {
		QRectF letter;
		if (_Change(vtx, frameRect(p->geometry(), offset, &letter))) {
			if (vtx.size() != QSizeF(mposd->width(), mposd->height())) {
				if (forceUpdateOsd)
					mposd->forceUpdateTargetSize();
			}
			mposd->setGeometry(vtx);
			emit p->frameRectChanged(vtx);
		}
		if (letterbox->set(p->rect(), letter))
			emit p->screenRectChanged(letterbox->screen());
		if (overlay)
			overlay->setGeometry(overlayInLetterbox ? p->rect() : letterbox->screen());
		p->setGeometryDirty();
	}
};

VideoRendererItem::VideoRendererItem(QQuickItem *parent)
: TextureRendererItem(parent), d(new Data(this)) {
	setFlag(ItemAcceptsDrops, true);
	d->mposd = new MpOsdItem(this);
	d->letterbox = new LetterboxItem(this);
	setZ(-1);
}

VideoRendererItem::~VideoRendererItem() {
	delete d;
}

double VideoRendererItem::avgfps() const {
	const auto time = _SystemTime();
	const auto frames = d->drawnFrames;
	double fps = 0.0;
	if (time > d->lastCheckedTime && frames > d->lastCheckedFrames)
		fps = double(frames - d->lastCheckedFrames)/(double(time - d->lastCheckedTime)*1e-6);
	d->lastCheckedFrames = frames;
	d->lastCheckedTime = time;
	return fps;
}

void VideoRendererItem::setOverlayOnLetterbox(bool letterbox) {
	if (_Change(d->overlayInLetterbox, letterbox))
		d->updateGeometry(false);
}

bool VideoRendererItem::overlayInLetterbox() const {
	return d->overlayInLetterbox;
}

void VideoRendererItem::initializeGL() {
	TextureRendererItem::initializeGL();
	Q_ASSERT(QOpenGLContext::currentContext() != nullptr);
	_Renew(d->shader);
	d->shader->setDeintMethod(d->deint);
	d->shader->setColor(d->color);
	d->shader->setEffects(d->effects);
	d->shader->setChromaInterpolator(d->chromaUpscaler);
	d->shader->setRange(d->range);
	d->black.create(OGL::Repeat);
	OpenGLTextureBinder<OGL::Target2D> binder(&d->black);
	const quint32 p = 0x0;
	d->black.initialize(1, 1, OGL::BGRA, &p);
	d->initialized = true;
	d->direct = false;
	setRenderTarget(d->black);
}

void VideoRendererItem::finalizeGL() {
	TextureRendererItem::finalizeGL();
	Q_ASSERT(QOpenGLContext::currentContext() != nullptr);
	d->black.destroy();
	_Delete(d->fbo);
	_Delete(d->shader);
	d->initialized = false;
}

void VideoRendererItem::customEvent(QEvent *event) {
	switch ((int)event->type()) {
	case NewFrame:
		if (d->queue.size() < 3)
			d->queue.push_back(_GetData<VideoFrame>(event));
		update();
		break;
	case NextFrame:
		update();
		break;
	case Rerender:
		d->repaint();
		break;
	case UpdateDeint: {
		if (_Change(d->deint, _GetData<DeintMethod>(event)) && d->shader) {
			d->shader->setDeintMethod(d->deint);
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

void VideoRendererItem::present(const VideoFrame &frame) {
	if (!d->initialized)
		return;
	if (d->shader && d->queue.size() < 3)
		_PostEvent(this, NewFrame, frame);
	d->mposd->present(false);
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
		d->updateGeometry(false);
		update();
	}
}

void VideoRendererItem::rerender() {
	_PostEvent(this, Rerender);
}

void VideoRendererItem::setDeintMethod(DeintMethod method) {
	_PostEvent(this, UpdateDeint, method);
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

void VideoRendererItem::setOverlay(GeometryItem *overlay) {
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
	d->updateGeometry(false);
}

void VideoRendererItem::setOffset(const QPoint &offset) {
	if (d->offset != offset) {
		d->offset = offset;
		emit offsetChanged(d->offset);
		d->updateGeometry(false);
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
		d->repaint();
	}
}

QRectF VideoRendererItem::frameRect(const QRectF &area) const {
	return d->frameRect(area);
}

void VideoRendererItem::setColor(const VideoColor &prop) {
	if (_Change(d->color, prop) && d->shader) {
		d->shader->setColor(d->color);
		d->repaint();
	}
}

void VideoRendererItem::setRange(ColorRange range) {
	if (_Change(d->range, range) && d->shader) {
		d->shader->setRange(d->range);
		d->repaint();
	}
}

ColorRange VideoRendererItem::range() const {
	return d->range;
}

const VideoColor &VideoRendererItem::color() const {
	return d->color;
}

void VideoRendererItem::setAspectRatio(double ratio) {
	if (!isSameRatio(d->aspect, ratio)) {
		d->aspect = ratio;
		d->updateGeometry(true);
		update();
	}
}

double VideoRendererItem::aspectRatio() const {
	return d->aspect;
}

void VideoRendererItem::setCropRatio(double ratio) {
	if (!isSameRatio(d->crop, ratio)) {
		d->crop = ratio;
		d->updateGeometry(true);
		update();
	}
}

InterpolatorType VideoRendererItem::chromaUpscaler() const {
	return d->chromaUpscaler;
}

void VideoRendererItem::setChromaUpscaler(InterpolatorType type) {
	if (_Change(d->chromaUpscaler, type)) {
		if (d->shader)
			d->shader->setChromaInterpolator(d->chromaUpscaler);
		rerender();
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

int VideoRendererItem::droppedFrames() const {
	return d->dropped;
}

MpOsdItem *VideoRendererItem::mpOsd() const {
	return d->mposd;
}

int VideoRendererItem::delay() const {
	return (d->ptsIn == MP_NOPTS_VALUE || d->ptsOut == MP_NOPTS_VALUE) ? 0.0 : (d->ptsOut - d->ptsIn)*1000.0;
}

void VideoRendererItem::reset() {
	if (_Change(d->dropped, 0))
		emit droppedFramesChanged(d->dropped);
	d->drawnFrames = 0;
	d->lastCheckedFrames = 0;
	d->lastCheckedTime = 0;
}

void VideoRendererItem::prepare(QSGGeometryNode *node) {
	Q_ASSERT(d->shader);
	if (d->take) {
		auto image = renderTarget().toImage();
		if (!image.isNull())
			d->mposd->drawOn(image);
		emit frameImageObtained(image);
		d->take = false;
	}
	if (!d->queue.isEmpty()) {
		auto &frame = d->queue.front();
		if (!frame.format().isEmpty()) {
			d->ptsOut = frame.pts();
			d->render = true;
			d->frameSize = frame.format().size();
			if (_Change(d->displaySize, frame.format().displaySize()))
				d->updateGeometry(true);
			d->shader->upload(frame);
			d->direct = d->shader->directRendering();
			++d->drawnFrames;
		}
		d->queue.pop_front();
		if (!d->queue.isEmpty()) {
			if (d->queue.size() > 3) {
				emit droppedFramesChanged(d->dropped += d->queue.size());
				d->queue.clear();
			} else {
				if (d->queue.size() > 1) {
					d->queue.pop_front();
					emit droppedFramesChanged(++d->dropped);
				}
				update();
			}
		}
	}

	if (d->render && !d->frameSize.isEmpty()) {
		if (d->direct) {
			setRenderTarget(d->shader->renderTarget());
		} else {
			if (!d->fbo || d->fbo->size() != d->frameSize) {
				_Renew(d->fbo, d->frameSize, OpenGLCompat::framebufferObjectTextureFormat());
				Q_ASSERT(d->fbo->isValid());
				setRenderTarget(d->fbo->texture());
			}
			d->fbo->bind();
			d->shader->render(d->kernel);
			d->fbo->release();
		}
		node->markDirty(QSGNode::DirtyMaterial);
	}
	d->render = false;
	LOG_GL_ERROR_Q
}

void VideoRendererItem::getCoords(QRectF &vertices, QRectF &texCoords) {
	double top = 0.0, left = 0.0, right = 1.0, bottom = 1.0;
	if (d->direct) {
		if (d->shader)
			d->shader->getCoords(left, top, right, bottom);
	} else {
		if (d->fbo)
			d->fbo->getCoords(left, top, right, bottom);
	}
	if (!(d->effects & Disable)) {
		if (d->effects & FlipVertically)
			qSwap(top, bottom);
		if (d->effects & FlipHorizontally)
			qSwap(left, right);
	}
	vertices = d->vtx;
	texCoords = {left, top, right-left, bottom-top};
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

QPointF VideoRendererItem::mapToVideo(const QPointF &pos) {
	auto hratio = d->mposd->targetSize().width()/d->vtx.width();
	auto vratio = d->mposd->targetSize().height()/d->vtx.height();
	auto p = pos - d->vtx.topLeft();
	p.rx() *= hratio;
	p.ry() *= vratio;
	return p;
}
