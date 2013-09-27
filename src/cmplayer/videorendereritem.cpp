#include "videorendereritem.hpp"
#include "mposditem.hpp"
#include "videoframe.hpp"
#include "hwacc.hpp"
#include "global.hpp"
#include "letterboxitem.hpp"
#include "videotextureshader.hpp"
#include "dataevent.hpp"
#include "videoframeshader.hpp"
#include "videotextureshader.glsl.hpp"

struct VideoRendererItem::Data {
	QLinkedList<VideoFrame> queue;
	VideoFrame frame;
	VideoFormat format;
	quint64 drawnFrames = 0;
	bool newFrame = false, shaderChanged = false;
	QRectF vtx;
	QPoint offset = {0, 0};
	double crop = -1.0, aspect = -1.0, dar = 0.0, pts = MP_NOPTS_VALUE;
	int alignment = Qt::AlignCenter;
	LetterboxItem *letterbox = nullptr;
	MpOsdItem *mposd = nullptr;
	QQuickItem *overlay = nullptr;
	VideoFrameShader *shader = nullptr;
	Kernel3x3 blur, sharpen, kernel;
	Effects effects = 0;
	ColorProperty color;
	DeintInfo deint;
	InterpolatorType interpolator = InterpolatorType::Bilinear;
	OpenGLFramebufferObject *fbo = nullptr;
	QMutex mutex;
	int loc_vMatrix = -1, loc_tex = -1, loc_lut_bicubic = -1;
	QByteArray fragCode, vertexCode;
	QByteArray header() const {
		QByteArray header;
		header += "const float texWidth = " + QByteArray::number(fbo ? fbo->width() : 1) + ".0;\n";
		header += "const float texHeight = " + QByteArray::number(fbo ? fbo->height() : 1) + ".0;\n";
		if (interpolator > 0)
			header += "#define USE_BICUBIC\n";
		return header;
	}
	OpenGLTexture lutBicubic;
};

VideoRendererItem::VideoRendererItem(QQuickItem *parent)
: TextureRendererItem(1, parent), d(new Data) {
	setFlags(ItemHasContents | ItemAcceptsDrops);
	d->mposd = new MpOsdItem(this);
	d->letterbox = new LetterboxItem(this);
	setZ(-1);
	setBechmark(true);
}

VideoRendererItem::~VideoRendererItem() {
	delete d;
}

void VideoRendererItem::release() {
	Q_ASSERT(QOpenGLContext::currentContext() != nullptr);
	_Delete(d->fbo);
	_Delete(d->shader);
}

void VideoRendererItem::customEvent(QEvent *event) {
	switch ((int)event->type()) {
	case NewFrame:
		d->newFrame = true;
		d->queue.push_back(getData<VideoFrame>(event));
		update();
		break;
	case RenderNextFrame:
		update();
		break;
	case UpdateDeint: {
		auto deint = getData<DeintInfo>(event);
		if (!deint.isHardware())
			deint = DeintInfo();
		if (_Change(d->deint, deint)) {
			if (d->shader)
				d->shader->setDeintInfo(d->deint);
			update();
		}
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
	if (d->format.isEmpty() || !d->fbo)
		emit frameImageObtained(QImage());
	else if (d->frame.hasImage())
		emit frameImageObtained(d->frame.toImage());
	else {
		auto image = d->fbo->toImage();
		d->mposd->drawOn(image);
		emit frameImageObtained(image);
	}
}

void VideoRendererItem::present(const QImage &image) {
	present(VideoFrame(image));
}

void VideoRendererItem::present(const VideoFrame &frame, bool redraw) {
//	d->mutex.lock();
//	d->back = frame;
//	d->mutex.unlock();
	postData(this, NewFrame, frame);
	d->mposd->present(redraw);
	d->pts = frame.pts();
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

void VideoRendererItem::updateKernel() {
	d->kernel = Kernel3x3();
	if (d->effects & Blur)
		d->kernel += d->blur;
	if (d->effects & Sharpen)
		d->kernel += d->sharpen;
	d->kernel.normalize();
}

void VideoRendererItem::setEffects(Effects effects) {
	if (_Change(d->effects, effects)) {
		if (d->shader)
			d->shader->setEffects(d->effects);
		updateKernel();
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
		if (d->shader)
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
//	if (!d->shader)
//		return TextureRendererItem::fragmentShader();
//	d->fragCode = d->shader->fragment();
//	return d->fragCode.data();
	d->fragCode = d->header();
	d->fragCode += "#define FRAGMENT\n";
	d->fragCode += QByteArray((char*)videotextureshader_glsl, videotextureshader_glsl_len);
	return d->fragCode.constData();
}

const char *VideoRendererItem::vertexShader() const {
//	if (!d->shader)
//		return TextureRendererItem::vertexShader();
//	d->vertexCode = d->shader->vertex();
//	return d->vertexCode;
	d->vertexCode = d->header();
	d->vertexCode += "#define VERTEX\n";
	d->vertexCode += QByteArray((char*)videotextureshader_glsl, videotextureshader_glsl_len);
	return d->vertexCode.constData();
}

const char * const *VideoRendererItem::attributeNames() const {
//	if (!d->shader)
//		return TextureRendererItem::attributeNames();
//	return d->shader->attributes();
	static const char *const names[] = { "vPosition", "vCoord", nullptr };
	return names;
}

void VideoRendererItem::link(QOpenGLShaderProgram *prog) {
	d->loc_tex = prog->uniformLocation("tex");
	d->loc_vMatrix = prog->uniformLocation("vMatrix");
	d->loc_lut_bicubic = prog->uniformLocation("lut_bicubic");
//	if (d->shader)
//		d->shader->link(prog);
}

MpOsdItem *VideoRendererItem::mpOsd() const {
	return d->mposd;
}

void VideoRendererItem::bind(const RenderState &state, QOpenGLShaderProgram *program) {
//	if (d->shader)
//		d->shader->render(program, state.combinedMatrix(), d->kernel);
	program->setUniformValue(d->loc_vMatrix, state.combinedMatrix());
	program->setUniformValue(d->loc_tex, 0);
	program->setUniformValue(d->loc_lut_bicubic, 1);
	auto f = OpenGLCompat::functions();
	f->glActiveTexture(GL_TEXTURE0);
	if (d->fbo)
		d->fbo->texture().bind();
	if (d->interpolator > 0) {
		f->glActiveTexture(GL_TEXTURE1);
		d->lutBicubic.bind();
		f->glActiveTexture(GL_TEXTURE0);
	}
}

int VideoRendererItem::delay() const {
	return 0;
	return d->pts == MP_NOPTS_VALUE ? 0.0 : -(d->frame.pts() - d->pts)*1000.0;
}

void VideoRendererItem::beforeUpdate() {
//	if (!d->newFrame)
//		return;
	if (d->queue.isEmpty())
		return;
//	d->mutex.lock();
//	qSwap(d->back, d->frame);
//	d->mutex.unlock();
	d->frame = d->queue.takeFirst();
	if (_Change(d->format, d->frame.format())) {
		_Renew(d->shader, d->format, d->color, d->effects, d->deint);
		updateGeometry(true);
		d->shaderChanged = true;
	}
	if (d->shader && d->shader->needsToBuild()) {
		d->shader->build();
		d->shaderChanged = true;
	}
	if (d->shaderChanged)
		resetNode();
	if (!d->format.isEmpty() && d->shader) {
		if (!d->fbo || d->fbo->size() != d->format.size())
			_Renew(d->fbo, d->format.size(), GL_TEXTURE_2D, GL_NEAREST);
		d->shader->upload(d->frame);
		if (d->fbo) {
			d->fbo->bind();
			d->shader->render(d->kernel);
			d->fbo->release();
		}
		++d->drawnFrames;
	}
	if (!d->queue.isEmpty()) {
		if (d->queue.size() > 2)
			d->queue.clear();
		else
			postData(this, RenderNextFrame);
	}
	d->newFrame = false;
	d->shaderChanged = false;
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

void VideoRendererItem::initializeTextures() {
	d->lutBicubic = OpenGLCompat::allocateBicubicLutTexture(textures()[0], d->interpolator);
}

QQuickItem *VideoRendererItem::osd() const {
	return d->mposd;
}

void VideoRendererItem::setKernel(int blur_c, int blur_n, int blur_d, int sharpen_c, int sharpen_n, int sharpen_d) {
	d->blur.set(blur_c, blur_n, blur_d);
	d->sharpen.set(sharpen_c, sharpen_n, sharpen_d);
	updateKernel();
	update();
}

void VideoRendererItem::setInterpolator(InterpolatorType interpolator) {
	if (_Change(d->interpolator, interpolator)) {
		d->shaderChanged = true;
		update();
	}
}
