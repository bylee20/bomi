#include "videorendereritem.hpp"
#include "mposditem.hpp"
#include "mposdbitmap.hpp"
#include "letterboxitem.hpp"
#include "videoframebufferobject.hpp"
#include "videocolor.hpp"
#include "kernel3x3.hpp"
#include "misc/dataevent.hpp"
#include "opengl/opengltexturebinder.hpp"
#include "misc/log.hpp"

DECLARE_LOG_CONTEXT(Video)

enum EventType {NewFrame = QEvent::User + 1, NewFrameImage };

struct VideoCache {
    VideoRendererItem::Cache frame;
    VideoRendererItem::OsdCache osd;
    bool hasOsd = false;
};

struct VideoRendererItem::Data {
    Data(VideoRendererItem *p): p(p) {}
    VideoRendererItem *p = nullptr;
    QTimer sizeChecker; QSize prevSize, osdSize;
    std::deque<VideoCache> queue;
    MpOsdItem mposd;
    VideoCache cache;
    bool take = false, overlayInLetterbox = true, rectangle = false;
    bool forceToUpdateOsd = false;
    QRectF vtx;
    QPoint offset = {0, 0};
    double crop = -1.0, aspect = -1.0, dar = 0.0;
    int alignment = Qt::AlignCenter;
    VideoEffects effects = 0;
    LetterboxItem *letterbox = nullptr;
    GeometryItem *overlay = nullptr;
    Kernel3x3 blur, sharpen, kernel;
    OpenGLTexture2D black;
    QSize displaySize{1, 1};
    static auto isSameRatio(double r1, double r2) -> bool
        {return (r1 < 0.0 && r2 < 0.0) || qFuzzyCompare(r1, r2);}
    auto targetAspectRatio() const -> double
    {
        if (aspect > 0.0)
            return aspect;
        if (aspect == 0.0)
            return itemAspectRatio();
        if (!p->hasFrame())
            return 1.0;
        return displaySize.width()/(double)displaySize.height();
    }
    auto targetCropRatio(double fallback) const -> double
    {
        if (crop > 0.0)
            return crop;
        if (crop == 0.0)
            return itemAspectRatio();
        return fallback;
    }
    auto targetCropRatio() const -> double
        { return targetCropRatio(targetAspectRatio()); }
    auto itemAspectRatio() const -> double { return p->width()/p->height(); }
    auto makeKernel() const -> Kernel3x3
    {
        Kernel3x3 kernel;
        if (effects.contains(VideoEffect::Blur))
            kernel += blur;
        if (effects.contains(VideoEffect::Sharpen))
            kernel += sharpen;
        kernel.normalize();
        return kernel;
    }
    auto fillKernel() -> void
        { if (_Change(kernel, makeKernel())) emit p->kernelChanged(kernel); }
    auto frameRect(const QRectF &area, const QPoint &off = {0, 0},
                   QRectF *letterbox = nullptr) -> QRectF
    {
        if (!p->hasFrame())
            return area;
        QRectF rect = area;
        const double aspect = targetAspectRatio();
        QSizeF frame(aspect, 1.0), letter(targetCropRatio(aspect), 1.0);
        letter.scale(area.width(), area.height(), Qt::KeepAspectRatio);
        frame.scale(letter, Qt::KeepAspectRatioByExpanding);
        QPointF pos(area.x(), area.y());
        pos.rx() += (area.width() - frame.width())*0.5;
        pos.ry() += (area.height() - frame.height())*0.5;
        rect = {pos, frame};

        QPointF xy(area.width(), area.height());
        xy.rx() -= letter.width();
        xy.ry() -= letter.height();
        xy *= 0.5;
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
        return rect;
    }

    auto updateOsdSize() -> void
    {
        if (!_Change(prevSize, vtx.size().toSize())) {
            sizeChecker.stop();
            emit p->osdSizeChanged(osdSize = prevSize);
        }
        forceToUpdateOsd = false;
    }
};

VideoRendererItem::VideoRendererItem(QQuickItem *parent)
    : HighQualityTextureItem(parent)
    , d(new Data(this))
{
    d->letterbox = new LetterboxItem(this);
    const QQmlProperty property(d->letterbox, u"anchors.centerIn"_q);
    property.write(QVariant::fromValue(this));
    setZ(-1);
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::AllButtons);
    setFlag(ItemAcceptsDrops, true);
    connect(&d->sizeChecker, &QTimer::timeout, [=] () { d->updateOsdSize(); });
    d->sizeChecker.setInterval(300);
}

VideoRendererItem::~VideoRendererItem() {
    delete d;
}

auto VideoRendererItem::setOverlayOnLetterbox(bool letterbox) -> void
{
    if (_Change(d->overlayInLetterbox, letterbox))
        reserve(UpdateGeometry);
}

auto VideoRendererItem::overlayInLetterbox() const -> bool
{
    return d->overlayInLetterbox;
}

auto VideoRendererItem::initializeGL() -> void
{
    HighQualityTextureItem::initializeGL();
    d->black.create(OGL::Repeat);
    OpenGLTextureBinder<OGL::Target2D> binder(&d->black);
    const quint32 p = 0x0;
    d->black.initialize(1, 1, OGL::BGRA, &p);
    d->mposd.initialize();
}

auto VideoRendererItem::finalizeGL() -> void
{
    HighQualityTextureItem::finalizeGL();
    d->cache = VideoCache();
    d->queue.clear();
    d->black.destroy();
    d->mposd.finalize();
}

auto VideoRendererItem::customEvent(QEvent *event) -> void
{
    switch (static_cast<int>(event->type())) {
    case NewFrame: {
        VideoCache cache;
        _GetAllData(event, cache.frame, cache.osd, cache.hasOsd);
        if (!cache.frame.isNull()) {
            _Trace("Queue new video frame %%", cache.frame->size());
            d->queue.push_back(cache);
        } else
            _Trace("Hurry up to empty queue");
        reserve(UpdateMaterial);
        break;
    } case NewFrameImage: {
        QImage image, osd;
        _GetAllData(event, image, osd);
        emit frameImageObtained(image, osd);
        break;
    } default:
        break;
    }
}

auto VideoRendererItem::overlay() const -> QQuickItem*
{
    return d->overlay;
}

auto VideoRendererItem::hasFrame() const -> bool
{
    return !d->cache.frame.isNull() && !d->cache.frame->size().isEmpty();
}

auto VideoRendererItem::requestFrameImage() const -> void
{
    if (!hasFrame())
        emit frameImageObtained(QImage(), QImage());
    else {
        d->take = true;
        const_cast<VideoRendererItem*>(this)->reserve(UpdateMaterial, true);
    }
}

auto VideoRendererItem::present(const Cache &cache, const OsdCache &osd,
                                bool hasOsd) -> void
{
    if (!isInitialized()) {
        _Trace("VideoRendererItem is not initialized yet");
        return;
    }
    _PostEvent(Qt::HighEventPriority, this, NewFrame, cache, osd, hasOsd);
}

auto VideoRendererItem::screenRect() const -> QRectF
{
    return d->letterbox->screen();
}

auto VideoRendererItem::alignment() const -> int
{
    return d->alignment;
}

auto VideoRendererItem::setAlignment(int alignment) -> void
{
    if (_Change(d->alignment, alignment))
        reserve(UpdateGeometry);
}

auto VideoRendererItem::setOverlay(GeometryItem *overlay) -> void
{
    if (d->overlay != overlay) {
        if (d->overlay)
            d->overlay->setParentItem(nullptr);
        if ((d->overlay = overlay))
            d->overlay->setParentItem(this);
    }
}

auto VideoRendererItem::setOffset(const QPoint &offset) -> void
{
    if (d->offset != offset) {
        d->offset = offset;
        emit offsetChanged(d->offset);
        reserve(UpdateGeometry);
    }
}

auto VideoRendererItem::offset() const -> QPoint
{
    return d->offset;
}

auto VideoRendererItem::effects() const -> VideoEffects
{
    return d->effects;
}

auto VideoRendererItem::setEffects(VideoEffects effects) -> void
{
    if ((effects^d->effects) & (VideoEffect::FlipH | VideoEffect::FlipV))
        reserve(UpdateGeometry);
    if (_Change(d->effects, effects)) {
        d->fillKernel();
        emit effectsChanged(effects);
    }
}

auto VideoRendererItem::frameRect(const QRectF &area) const -> QRectF
{
    return d->frameRect(area);
}

auto VideoRendererItem::setAspectRatio(double ratio) -> void
{
    if (!d->isSameRatio(d->aspect, ratio)) {
        d->aspect = ratio;
        reserve(UpdateGeometry);
    }
}

double VideoRendererItem::aspectRatio() const
{
    return d->aspect;
}

auto VideoRendererItem::setCropRatio(double ratio) -> void
{
    if (!d->isSameRatio(d->crop, ratio)) {
        d->crop = ratio;
        reserve(UpdateGeometry);
    }
}

auto VideoRendererItem::cropRatio() const -> double
{
    return d->crop;
}

auto VideoRendererItem::sizeHint() const -> QSize
{
    if (!hasFrame())
        return QSize(400, 300);
    const double aspect = d->targetAspectRatio();
    QSizeF size(aspect, 1.0);
    size.scale(d->displaySize, Qt::KeepAspectRatioByExpanding);
    QSizeF crop(d->targetCropRatio(aspect), 1.0);
    crop.scale(size, Qt::KeepAspectRatio);
    return crop.toSize();
}

auto VideoRendererItem::osdSize() const -> QSize
{
    return d->osdSize;
}

auto VideoRendererItem::updateTexture(OpenGLTexture2D *texture) -> void
{
    if (d->take) {
        auto image = texture->toImage();
        QImage osd;
        if (!image.isNull() && d->cache.osd)
            osd = d->cache.osd->toImage();
        auto scale = [] (QImage &image, const QSize &size) {
            if (!image.isNull() && image.size() != size)
                image = image.scaled(size, Qt::IgnoreAspectRatio,
                                     Qt::SmoothTransformation);
        };
        scale(image, sizeHint());
        scale(osd, image.size());
        _PostEvent(this, NewFrameImage, image, osd);
        d->take = false;
    }
    if (d->queue.empty()) {
        _Trace("Videe frame queue is empty");
        return;
    }
    auto &front = d->queue.front();
    d->cache.frame.swap(front.frame);
    Q_ASSERT(!d->cache.frame.isNull());
    *texture = d->cache.frame->texture();
    _Trace("Render next frame(%%)", texture->size());

    if (front.osd || d->cache.hasOsd != front.hasOsd) {
        d->cache.hasOsd = front.hasOsd;
        if (front.osd) {
            Q_ASSERT(d->cache.hasOsd);
            d->cache.osd.swap(front.osd);
            d->mposd.draw(d->cache.osd);
        } else if (!d->cache.hasOsd)
            d->mposd.draw(d->cache.osd = { });
        setOverlayTexture(d->mposd.isVisible() ? d->mposd.texture()
                                               : transparentTexture());
    }
    d->queue.pop_front();

    if (_Change(d->rectangle, texture->target() == OGL::TargetRectangle)
            | _Change(d->displaySize, d->cache.frame->displaySize())) {
        d->forceToUpdateOsd = true;
        reserve(UpdateGeometry, false);
    }
}

auto VideoRendererItem::updateVertex(Vertex *vertex) -> void
{
    QRectF letter;
    if (_Change(d->vtx, d->frameRect(geometry(), d->offset, &letter))) {
        emit frameRectChanged(d->vtx);
        if (d->forceToUpdateOsd)
            d->updateOsdSize();
        else
            d->sizeChecker.start();
    }
    if (d->letterbox->set(rect(), letter))
        emit screenRectChanged(d->letterbox->screen());
    if (d->overlay) {
        const auto g = d->overlayInLetterbox ? rect() : d->letterbox->screen();
        d->overlay->setGeometry(g);
    }

    double top = 0.0, left = 0.0, right = 1.0, bottom = 1.0;
    if (d->rectangle) {
        const auto &texture = this->texture();
        right = texture.width();
        bottom = texture.height();
    }
    if (!d->effects.contains(VideoEffect::Disable)) {
        if (d->effects.contains(VideoEffect::FlipV))
            std::swap(top, bottom);
        if (d->effects.contains(VideoEffect::FlipH))
            std::swap(left, right);
    }
    Vertex::fillAsTriangleStrip(vertex, d->vtx.topLeft(), d->vtx.bottomRight(),
                                {left, top}, {right, bottom});
}

auto VideoRendererItem::afterUpdate() -> void
{
    if (!d->queue.empty())
        reserve(UpdateMaterial);
}

auto VideoRendererItem::setKernel(const Kernel3x3 &blur,
                                  const Kernel3x3 &sharpen) -> void
{
    d->blur = blur;
    d->sharpen = sharpen;
    d->fillKernel();
}

auto VideoRendererItem::kernel() const -> const Kernel3x3&
{
    return d->kernel;
}

auto VideoRendererItem::mapToVideo(const QPointF &pos) -> QPointF
{
    auto hratio = d->osdSize.width()/d->vtx.width();
    auto vratio = d->osdSize.height()/d->vtx.height();
    auto p = pos - d->vtx.topLeft();
    p.rx() *= hratio;
    p.ry() *= vratio;
    return p;
}
