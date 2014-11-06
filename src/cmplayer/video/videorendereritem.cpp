#include "videorendereritem.hpp"
#include "mposditem.hpp"
#include "mposdbitmap.hpp"
#include "videodata.hpp"
#include "letterboxitem.hpp"
#include "deintoption.hpp"
#include "videotexture.hpp"
#include "videocolor.hpp"
#include "kernel3x3.hpp"
#include "misc/dataevent.hpp"
#include "opengl/opengltexturebinder.hpp"
#include "misc/log.hpp"
#include "videoframeshader.hpp"
#include "enum/interpolatortype.hpp"

DECLARE_LOG_CONTEXT(Video)

enum EventType {NewFrame = QEvent::User + 1, NewFormat, NewFrameImage };

auto initialize_vdpau_interop(QOpenGLContext *ctx) -> void;
auto finalize_vdpau_interop(QOpenGLContext *ctx) -> void;

enum Dirty {
    DirtyColorMatrix    = 0x0001,
    DirtyEffects        = 0x0002,
    DirtyChromaUpscaler = 0x0004,
    DirtyFormat         = 0x0008,
    DirtyDeint          = 0x0016,
    DirtyVertex         = 0xff00
};

struct FrameTime {
    FrameTime(quint64 frames, quint64 time)
        : frames(frames), time(time) { }
    quint64 frames = 0, time = 0;
};

struct VideoRenderer::Data {
    Data(VideoRenderer *p): p(p) {}
    VideoRenderer *p = nullptr;
    QTimer sizeChecker; QSize prevSize, osdSize;
    std::deque<VideoData> queue;
    MpOsdItem mposd;
    VideoData data;
    VideoFormat format;
    bool take = false, overlayOnLetterbox = true, rectangle = false;
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
    QOpenGLContext *ctx = nullptr;
    OpenGLFramebufferObject *fbo = nullptr;
    VideoFrameShader *shader = nullptr;
    QSize displaySize{1, 1};


    uint dirty = 0;
    VideoColor eq;
    InterpolatorType chromaUpscaler = InterpolatorType::Bilinear;
    ColorRange range = ColorRange::Auto;
    ColorSpace space = ColorSpace::Auto;
    DeintOption deint_swdec, deint_hwdec;

    quint64 drawn = 0, dropped = 0;
    std::deque<FrameTime> timings;
    double avgfps = 0.0;
    void clearTimings() {
        timings.clear();
        drawn = 0;
        avgfps = 0;
    }


    template<class T, class Sig>
    auto mark(T &t, const T &prev, Dirty flag, Sig sig,
              UpdateHint hints = UpdateAll) -> bool
    {
        if (_Change(t, prev)) {
            dirty |= flag;
            p->reserve(hints, true);
            emit (p->*sig)(t);
            return true;
        }
        return false;
    }

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
        if (forceToUpdateOsd | !_Change(prevSize, vtx.size().toSize())) {
            sizeChecker.stop();
            emit p->osdSizeChanged(osdSize = prevSize);
        }
        forceToUpdateOsd = false;
    }
};

VideoRenderer::VideoRenderer(QQuickItem *parent)
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

VideoRenderer::~VideoRenderer() {
    delete d;
}

auto VideoRenderer::droppedFrames() const -> int
{
    return d->dropped;
}

auto VideoRenderer::resetTimings() -> void
{
    if (_Change(d->dropped, 0ull))
        emit droppedFramesChanged(d->dropped);
}

auto VideoRenderer::avgfps() const -> double
{
    return d->avgfps;
}

auto VideoRenderer::setOverlayOnLetterbox(bool letterbox) -> void
{
    d->mark(d->overlayOnLetterbox, letterbox, DirtyVertex,
            &VideoRenderer::overlayOnLetterboxChanged, UpdateGeometry);
}

auto VideoRenderer::overlayOnLetterbox() const -> bool
{
    return d->overlayOnLetterbox;
}

auto VideoRenderer::initializeGL() -> void
{
    HighQualityTextureItem::initializeGL();
    d->ctx = QOpenGLContext::currentContext();
    initialize_vdpau_interop(d->ctx);

    d->black.create(OGL::Repeat);
    OpenGLTextureBinder<OGL::Target2D> binder(&d->black);
    const quint32 p = 0x0;
    d->black.initialize(1, 1, OGL::BGRA, &p);
    d->mposd.initialize();
    d->shader = new VideoFrameShader;
    d->dirty = 0xffffffff;
}

auto VideoRenderer::finalizeGL() -> void
{
    HighQualityTextureItem::finalizeGL();
    d->data = VideoData();
    d->queue.clear();
    d->black.destroy();
    d->mposd.finalize();
    _Delete(d->shader);
    _Delete(d->fbo);
    finalize_vdpau_interop(d->ctx);
    d->ctx = nullptr;
}

auto VideoRenderer::customEvent(QEvent *event) -> void
{
    switch (static_cast<int>(event->type())) {
    case NewFrame: {
        VideoData data;
        _GetAllData(event, data);
        if (data.hasImage())
            d->queue.push_back(data);
        reserve(UpdateMaterial);
        break;
    } case NewFrameImage: {
        QImage image, osd;
        _GetAllData(event, image, osd);
        emit frameImageObtained(image, osd);
        break;
    } case NewFormat: {
        d->mark(d->format, _GetData<VideoFormat>(event), DirtyFormat,
                &VideoRenderer::formatChanged, UpdateAll);
        break;
    } default:
        break;
    }
}

auto VideoRenderer::overlay() const -> QQuickItem*
{
    return d->overlay;
}

auto VideoRenderer::hasFrame() const -> bool
{
    return d->data.hasImage() && !d->format.size().isEmpty();
}

auto VideoRenderer::requestFrameImage() const -> void
{
    if (!hasFrame())
        emit frameImageObtained(QImage(), QImage());
    else {
        d->take = true;
        const_cast<VideoRenderer*>(this)->reserve(UpdateMaterial, true);
    }
}

auto VideoRenderer::prepare(const VideoFormat &format) -> void
{
    if (!isInitialized())
        _Trace("VideoRendererItem::prepare(): not initialized");
    else {
        _Trace("VideoRendererItem::prepare(): prepare new format");
        _PostEvent(Qt::HighEventPriority, this, NewFormat, format);
    }
}

auto VideoRenderer::present(const VideoData &data) -> void
{
    if (!isInitialized())
        _Trace("VideoRendererItem::present(): not initialized");
    else {
        _Trace("VideoRendererItem::present(): post new frame");
        _PostEvent(Qt::HighEventPriority, this, NewFrame, data);
    }
}

auto VideoRenderer::screenRect() const -> QRectF
{
    return d->letterbox->screen();
}

auto VideoRenderer::alignment() const -> int
{
    return d->alignment;
}

auto VideoRenderer::setAlignment(int alignment) -> void
{
    d->mark(d->alignment, alignment, DirtyVertex,
            &VideoRenderer::alignmentChanged, UpdateGeometry);
}

auto VideoRenderer::setOverlay(GeometryItem *overlay) -> void
{
    if (d->overlay != overlay) {
        if (d->overlay)
            d->overlay->setParentItem(nullptr);
        if ((d->overlay = overlay))
            d->overlay->setParentItem(this);
    }
}

auto VideoRenderer::setOffset(const QPoint &offset) -> void
{
    d->mark(d->offset, offset, DirtyVertex,
            &VideoRenderer::offsetChanged, UpdateGeometry);
}

auto VideoRenderer::setDeintOptions(DeintOption swdec, DeintOption hwdec) -> void
{
    if (_Change(d->deint_swdec, swdec) | _Change(d->deint_hwdec, hwdec)) {
        d->dirty |= DirtyDeint;
        reserve(UpdateMaterial);
    }
}

auto VideoRenderer::deintOptionForSwdec() const -> DeintOption
{
    return d->deint_swdec;
}

auto VideoRenderer::deintOptionForHwdec() const -> DeintOption
{
    return d->deint_hwdec;
}

auto VideoRenderer::offset() const -> QPoint
{
    return d->offset;
}

auto VideoRenderer::effects() const -> VideoEffects
{
    return d->effects;
}

auto VideoRenderer::setEffects(VideoEffects effects) -> void
{
    if (d->mark(d->effects, effects, DirtyEffects,
                &VideoRenderer::effectsChanged, UpdateAll))
        d->fillKernel();
}

auto VideoRenderer::frameRect(const QRectF &area) const -> QRectF
{
    return d->frameRect(area);
}

auto VideoRenderer::setAspectRatio(double ratio) -> void
{
    if (!d->isSameRatio(d->aspect, ratio)) {
        d->aspect = ratio;
        reserve(UpdateGeometry);
    }
}

double VideoRenderer::aspectRatio() const
{
    return d->aspect;
}

auto VideoRenderer::setCropRatio(double ratio) -> void
{
    if (!d->isSameRatio(d->crop, ratio)) {
        d->crop = ratio;
        reserve(UpdateGeometry);
    }
}

auto VideoRenderer::cropRatio() const -> double
{
    return d->crop;
}

auto VideoRenderer::sizeHint() const -> QSize
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

auto VideoRenderer::osdSize() const -> QSize
{
    return d->osdSize;
}

auto VideoRenderer::chromaUpscaler() const -> InterpolatorType
{
    return d->chromaUpscaler;
}

auto VideoRenderer::setChromaUpscaler(InterpolatorType type) -> void
{
    d->mark(d->chromaUpscaler, type, DirtyChromaUpscaler,
            &VideoRenderer::chromaUpscalerChanged, UpdateMaterial);
}

auto VideoRenderer::setColorRange(ColorRange range) -> void
{
    d->mark(d->range, range, DirtyColorMatrix,
            &VideoRenderer::colorRangeChanged, UpdateMaterial);
}

auto VideoRenderer::colorSpace() const -> ColorSpace
{
    return d->space;
}

auto VideoRenderer::setColorSpace(ColorSpace space) -> void
{
    d->mark(d->space, space, DirtyColorMatrix,
            &VideoRenderer::colorSpaceChanged, UpdateMaterial);
}

auto VideoRenderer::colorRange() const -> ColorRange
{
    return d->range;
}

auto VideoRenderer::setEqualizer(const VideoColor &prop) -> void
{
    d->mark(d->eq, prop, DirtyColorMatrix,
            &VideoRenderer::equalizerChanged, UpdateMaterial);
}

auto VideoRenderer::equalizer() const -> const VideoColor&
{
    return d->eq;
}

auto VideoRenderer::updatePolish() -> void
{
    HighQualityTextureItem::updatePolish();
    //    d->hasOsd = false;
}

auto VideoRenderer::updateTexture(OpenGLTexture2D *texture) -> void
{
    if (d->queue.empty()) {
        _Trace("VideoRendererItem::updateTexture(): no queued frame");
    } else {
        auto &prev = d->queue.front();
        std::swap(d->data, prev);
        if (d->data.hasImage() && !d->format.isEmpty()) {
            if (d->dirty) {
                if (d->dirty & DirtyFormat) {
                    d->shader->setFormat(d->format.params(), d->format.isInverted());
                    if (!d->fbo || d->fbo->size() != d->format.size())
                        _Renew(d->fbo, d->format.size());
                    reserve(UpdateGeometry, false);
                }

                if (d->dirty & DirtyEffects)
                    d->shader->setEffects(d->effects);
                if (d->dirty & DirtyColorMatrix)
                    d->shader->setColor(d->eq, d->space, d->range);
                if (d->dirty & DirtyChromaUpscaler)
                    d->shader->setChromaUpscaler(d->chromaUpscaler);
                if (d->dirty & DirtyDeint) {
                    auto method = d->deint_swdec.method;
                    if (IMGFMT_IS_HWACCEL(d->format.params().imgfmt))
                        method = d->deint_hwdec.method;
                    d->shader->setDeintMethod(method);
                }
                d->dirty = 0;
            }

            d->shader->prepare(d->data.image());
            d->shader->upload(d->data.image());
            d->fbo->bind();
            d->shader->render(d->kernel);
            d->fbo->release();
            *texture = d->fbo->texture();
        }
//        if (shader->isDirectlyRenderable()) {
//            cache = pool->get(shader->directlyRenderableTexture(),
//                              format.displaySize(), shader->useDMA());
//            if (cache)
//                shader->upload(mpi, const_cast<VideoTexture&>(*cache));
//        } else {
//            cache = pool->get(format.size(), format.displaySize());
//            if (cache) {
//                Q_ASSERT(fbo->size() == cache->size());
//                shader->upload(mpi);
//                glBindTexture(cache->target(), GL_NONE);
//                fbo->bind();
//                fbo->attach(*cache);
//                shader->render(renderer->kernel());
//                fbo->release();
//            }
//        }

        d->mposd.draw(d->data.osd());
        setOverlayTexture(d->mposd.isVisible() ? d->mposd.texture()
                                               : transparentTexture());
        d->queue.pop_front();
        if (!d->queue.empty()) {
            if (d->queue.size() > 3) {
                d->dropped += d->queue.size();
                d->queue.clear();
                emit droppedFramesChanged(d->dropped);
            } else
                reserve(UpdateMaterial);
        }
        if (_Change(d->rectangle, texture->target() == OGL::TargetRectangle)
                | _Change(d->displaySize, d->format.displaySize())) {
            d->forceToUpdateOsd = true;
            reserve(UpdateGeometry, false);
        }

        ++d->drawn;
        constexpr int interval = 4;
        constexpr int max = 20, min = 5;
        const int size = d->timings.size();
        if (size == max && d->drawn & (interval - 1))
            return;
        d->timings.emplace_back(d->drawn, _SystemTime());
        if (size > min) {
            if (size > max)
                d->timings.pop_front();
            const auto df = d->timings.back().frames - d->timings.front().frames;
            const auto dt = d->timings.back().time - d->timings.front().time;
            d->avgfps = df/(dt*1e-6);
        } else
            d->avgfps = 0.0;
        _Trace("VideoRendererItem::updateTexture(): "
               "render queued frame(%%), avgfps: %%",
               texture->size(), d->avgfps);
    }

    if (d->take) {
        auto image = texture->toImage();
        QImage osd;
        if (!image.isNull() && d->data.osd())
            osd = d->data.osd()->toImage();
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
}

auto VideoRenderer::updateVertex(Vertex *vertex) -> void
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
        const auto g = d->overlayOnLetterbox ? rect() : d->letterbox->screen();
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

auto VideoRenderer::afterUpdate() -> void
{
    if (!d->queue.empty())
        reserve(UpdateMaterial);
}

auto VideoRenderer::setKernel(const Kernel3x3 &blur,
                                  const Kernel3x3 &sharpen) -> void
{
    d->blur = blur;
    d->sharpen = sharpen;
    d->fillKernel();
}

auto VideoRenderer::kernel() const -> const Kernel3x3&
{
    return d->kernel;
}

auto VideoRenderer::mapToVideo(const QPointF &pos) -> QPointF
{
    auto hratio = d->osdSize.width()/d->vtx.width();
    auto vratio = d->osdSize.height()/d->vtx.height();
    auto p = pos - d->vtx.topLeft();
    p.rx() *= hratio;
    p.ry() *= vratio;
    return p;
}
