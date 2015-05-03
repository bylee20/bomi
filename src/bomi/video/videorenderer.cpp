#include "videorenderer.hpp"
#include "letterboxitem.hpp"
#include "mpvosdrenderer.hpp"
#include "opengl/opengltexture2d.hpp"
#include "opengl/openglframebufferobject.hpp"
#include "opengl/opengltexturebinder.hpp"
#include "misc/dataevent.hpp"
#include "misc/log.hpp"
#include "enum/rotation.hpp"
#include <QQmlProperty>
#include <QQuickWindow>

DECLARE_LOG_CONTEXT(Video)

enum EventType {NewFrame = QEvent::User + 1 };

enum DirtyFlag {
    DirtyRot = 1
};

struct FboSet {
    OpenGLFramebufferObject *fbo = nullptr;
    QSize size;
    OpenGLTexture2D fallback;
    QMargins margins;
    QRectF rect;
    OGL::TextureFormat format = OGL::RGBA8_UNorm;
    bool visible = true;
    auto texture() const -> const OpenGLTexture2D*
    {
        if (visible && fbo && !fbo->size().isEmpty())
            return &fbo->texture();
        return &fallback;
    }
    auto renew() -> bool
    {
        if (!visible || (fbo && fbo->size() == size && fbo->format() == format))
            return false;
        _Renew(fbo, size, format);
        return true;
    }
};

struct VideoRenderer::VideoShaderData : public VideoRenderer::ShaderData {
    bool redraw = false, osdVisible = false;
    const FboSet *frame = nullptr, *osd = nullptr;
    QMargins osdMargins;
};

struct VideoRenderer::VideoShaderIface : public VideoRenderer::ShaderIface {
    VideoShaderIface(bool osd): m_osd(osd) {
        if (osd) {
            vertexShader = R"(
                uniform mat4 qt_Matrix;
                attribute vec4 aPosition;
                attribute vec2 aFrameTexCoord;
                attribute vec2 aOsdTexCoord;
                varying vec2 frameTexCoord;
                varying vec2 osdTexCoord;
                void main() {
                    frameTexCoord = aFrameTexCoord;
                    osdTexCoord = aOsdTexCoord;
                    gl_Position = qt_Matrix * aPosition;
                }
            )";
            fragmentShader = R"(
                varying vec2 frameTexCoord;
                varying vec2 osdTexCoord;
                uniform sampler2D frameTex;
                uniform sampler2D osdTex;
                void main() {
                    vec4 frame = texture2D(frameTex, frameTexCoord);
                    vec4 osd = texture2D(osdTex, osdTexCoord);
                    gl_FragColor = (frame * (1.0 - osd.a) + osd);
                }
            )";
        } else {
            vertexShader = R"(
                uniform mat4 qt_Matrix;
                attribute vec4 aPosition;
                attribute vec2 aFrameTexCoord;
                attribute vec2 aOsdTexCoord;
                varying vec2 frameTexCoord;
                void main() {
                    frameTexCoord = aFrameTexCoord;
                    gl_Position = qt_Matrix * aPosition;
                }
            )";
            fragmentShader = R"(
                varying vec2 frameTexCoord;
                uniform sampler2D frameTex;
                void main() {
                    gl_FragColor = texture2D(frameTex, frameTexCoord);
                }
            )";
        }
        attributes << "aPosition" << "aFrameTexCoord" << "aOsdTexCoord";
    }
    void resolve(QOpenGLShaderProgram *prog) override {
        Q_ASSERT(prog->isLinked());
        loc_frameTex = prog->uniformLocation("frameTex");
        if (m_osd)
            loc_osdTex = prog->uniformLocation("osdTex");
    }
    void update(QOpenGLShaderProgram *prog,
                VideoRenderer::ShaderData *data) override {
        auto d = static_cast<VideoShaderData*>(data);
        const auto frame = d->frame->texture();
        if (frame->id() != GL_NONE && !frame->isEmpty()) {
            prog->setUniformValue(loc_frameTex, 0);
            prog->setUniformValue(loc_osdTex, 1);
            func()->glActiveTexture(GL_TEXTURE0);
            frame->bind();
            if (m_osd && d->osdVisible) {
                func()->glActiveTexture(GL_TEXTURE1);
                d->osd->texture()->bind();
                func()->glActiveTexture(GL_TEXTURE0);
            }
        }
    }
private:
    int loc_frameTex = -1, loc_osdTex = -1;
    bool m_osd = false;
};

struct VideoRenderer::Node : public QSGGeometryNode {
    Node(VideoRenderer *r): r(r) { setFlag(UsePreprocess, true); }
    auto preprocess() -> void final
    {
        auto d = shaderData(material());
        r->render(static_cast<VideoRenderer::VideoShaderData*>(d));
    }
private:
    VideoRenderer *r;
};

struct VideoRenderer::Data {
    VideoRenderer *p = nullptr;
    double crop = -1.0, aspect = -1.0, dar = 0.0;
    bool onLetterbox = true, redraw = false, portrait = false;
    bool flip_h = false, flip_v = false, scaler = false;
    Qt::Alignment alignment = Qt::AlignCenter;
    QRectF vtx; QPointF offset = {0, 0};
    LetterboxItem *letterbox = nullptr;
    GeometryItem *overlay = nullptr;
    Rotation rotation = Rotation::D0;
    int dirty = 0;

    FboSet frame, osd;

    QSize sourceSize{0, 1};
    QTimer sizeChecker;
    RenderFrameFunc render = nullptr;

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
        return sourceSize.width()/(double)sourceSize.height();
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
    auto frameRect(const QRectF &area, const QPointF &off = {0, 0},
                   QRectF *letterbox = nullptr) -> QRectF
    {
        if (!p->hasFrame())
            return area;

        const double aspect = targetAspectRatio();
        QSizeF frame(aspect, 1.0), letter(targetCropRatio(aspect), 1.0);
        if (portrait) {
            std::swap(frame.rwidth(), frame.rheight());
            std::swap(letter.rwidth(), letter.rheight());
        }
        letter.scale(area.width(), area.height(), Qt::KeepAspectRatio);
        frame.scale(letter, Qt::KeepAspectRatioByExpanding);

        QPointF pos(area.x(), area.y());
        pos.rx() += (area.width() - frame.width())*0.5;
        pos.ry() += (area.height() - frame.height())*0.5;
        QRectF rect = {pos, frame};

        QPointF xy(area.width(), area.height());
        xy.rx() -= letter.width();
        xy.ry() -= letter.height();
        xy *= 0.5;
        QPointF offset = off;
        offset.rx() *= letter.width();
        offset.ry() *= letter.height();
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
    auto osdSizeHint() const -> QSize
    {
        if (onLetterbox)
            return p->size().toSize();
        return fboSizeHint();
    }
    auto fboSizeHint() const -> QSize
    {
        auto size = sourceSize;
        if (portrait)
            std::swap(size.rwidth(), size.rheight());
        if (scaler)
            size.scale(qCeil(vtx.width()), qCeil(vtx.height()), Qt::KeepAspectRatio);
        return size;
    }
};

VideoRenderer::VideoRenderer(QQuickItem *parent)
    : Super(parent), d(new Data)
{
    d->p = this;
    d->letterbox = new LetterboxItem(this);
    const QQmlProperty property(d->letterbox, u"anchors.centerIn"_q);
    property.write(QVariant::fromValue(this));
    setZ(-1);
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::AllButtons);
    setFlag(ItemAcceptsDrops, true);
    connect(&d->sizeChecker, &QTimer::timeout, [this] () {
        if (_Change(d->frame.size, d->fboSizeHint()) |
                _Change(d->osd.size, d->osdSizeHint())) {
            d->redraw = true;
            reserve(UpdateAll);
        }
    });
    d->sizeChecker.setInterval(300);
    d->sizeChecker.setSingleShot(true);
}

VideoRenderer::~VideoRenderer() {
    delete d;
}

auto VideoRenderer::setFramebufferObjectFormat(OGL::TextureFormat format) -> void
{
    if (_Change(d->frame.format, format)) {
        d->redraw = true;
        reserve(UpdateMaterial);
    }
}

auto VideoRenderer::framebufferObjectFormat() const -> OGL::TextureFormat
{
    return d->frame.format;
}

auto VideoRenderer::setRenderFrameFunction(const RenderFrameFunc &func) -> void
{
    d->render = func;
}

auto VideoRenderer::updateForNewFrame(const QSize &displaySize) -> void
{
    _PostEvent(Qt::HighEventPriority, this, NewFrame, displaySize);
}

auto VideoRenderer::setOverlayOnLetterbox(bool letterbox) -> void
{
    if (_Change(d->onLetterbox, letterbox)) {
        if (_Change(d->osd.size, d->osdSizeHint()))
            reserve(UpdateAll);
        polish();
        emit overlayOnLetterboxChanged(d->onLetterbox);
    }
}

auto VideoRenderer::overlayOnLetterbox() const -> bool
{
    return d->onLetterbox;
}

auto VideoRenderer::initializeGL() -> void
{
    Super::initializeGL();
    d->frame.fallback.create(OGL::Repeat);
    OpenGLTextureBinder<OGL::Target2D> binder(&d->frame.fallback);
    const quint32 p = 0x0;
    d->frame.fallback.initialize(1, 1, OGL::BGRA, &p);
    d->osd.fallback = d->frame.fallback;
}

auto VideoRenderer::finalizeGL() -> void
{
    Super::finalizeGL();
    d->frame.fallback.destroy();
    _Delete(d->frame.fbo);
}

auto VideoRenderer::customEvent(QEvent *event) -> void
{
    switch (static_cast<int>(event->type())) {
    case NewFrame: {
        auto ds = _GetData<QSize>(event);
        if (_Change(d->sourceSize, ds)) {
            reserve(UpdateGeometry, false);
            d->frame.size = d->fboSizeHint();
            d->osd.size = d->osdSizeHint();
            polish();
        }
        d->redraw = true;
        reserve(UpdateMaterial);
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
    return !d->sourceSize.isEmpty();
}

auto VideoRenderer::screenRect() const -> QRectF
{
    return d->letterbox->screen();
}

auto VideoRenderer::alignment() const -> Qt::Alignment
{
    return d->alignment;
}

auto VideoRenderer::setAlignment(Qt::Alignment alignment) -> void
{
    if (_Change(d->alignment, alignment)) {
        polish();
        emit alignmentChanged(d->alignment);
    }
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

auto VideoRenderer::setOffset(const QPointF &offset) -> void
{
    if (_Change(d->offset, offset)) {
        polish();
        emit offsetChanged(d->offset);
    }
}

auto VideoRenderer::offset() const -> QPointF
{
    return d->offset;
}

auto VideoRenderer::frameRect(const QRectF &area) const -> QRectF
{
    return d->frameRect(area);
}

auto VideoRenderer::setAspectRatio(double ratio) -> void
{
    if (!d->isSameRatio(d->aspect, ratio)) {
        d->aspect = ratio;
        polish();
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
        polish();
        reserve(UpdateGeometry);
    }
}

auto VideoRenderer::cropRatio() const -> double
{
    return d->crop;
}

auto VideoRenderer::outputAspectRatio() const -> double
{
    return d->targetAspectRatio();
}

auto VideoRenderer::sizeHint() const -> QSize
{
    if (!hasFrame())
        return QSize(400, 300);
    const double aspect = d->targetAspectRatio();
    QSizeF size(aspect, 1.0);
    size.scale(d->sourceSize, Qt::KeepAspectRatioByExpanding);
    QSizeF crop(d->targetCropRatio(aspect), 1.0);
    crop.scale(size, Qt::KeepAspectRatio);
    if (d->portrait)
        std::swap(crop.rwidth(), crop.rheight());
    return crop.toSize();
}

auto VideoRenderer::updatePolish() -> void
{
    Super::updatePolish();
    d->sizeChecker.stop();
    QRectF letter;
    if (_Change(d->vtx, d->frameRect({0, 0, width(), height()}, d->offset, &letter))) {
        reserve(UpdateGeometry, false);
        const auto p0 = d->vtx.topLeft();
        const auto p1 = d->vtx.bottomRight();
#define NORM(v, vv) (vv - p0.v()) / double(p1.v() - p0.v())
        d->frame.rect.setTop(NORM(y, 0));
        d->frame.rect.setBottom(NORM(y, height()));
        d->frame.rect.setLeft(NORM(x, 0));
        d->frame.rect.setRight(NORM(x, width()));
#undef NORM
    }
    if (d->onLetterbox) {
        d->osd.rect = QRectF(0, 0, 1, 1);
        d->osd.margins.setTop(d->vtx.top());
        d->osd.margins.setLeft(d->vtx.left());
        d->osd.margins.setRight(width() - d->vtx.right() + 0.5);
        d->osd.margins.setBottom(height() - d->vtx.bottom() + 0.5);
    } else {
        d->osd.rect = d->frame.rect;
        d->osd.margins = QMargins();
    }
    if (d->letterbox->set(rect(), letter))
        emit screenRectChanged(d->letterbox->screen());
    if (d->overlay) {
        const auto g = d->onLetterbox ? rect() : d->letterbox->screen();
        d->overlay->setGeometry(g);
    }
    d->sizeChecker.start();
}

auto VideoRenderer::setFlipped(bool horizontal, bool vertical) -> void
{
    if (_Change(d->flip_h, horizontal) | _Change(d->flip_v, vertical))
        reserve(UpdateGeometry);
}

auto VideoRenderer::mapToVideo(const QPointF &pos) -> QPointF
{
    return pos - d->vtx.topLeft();
}

auto VideoRenderer::geometryChanged(const QRectF &new_, const QRectF& old) -> void
{
    Super::geometryChanged(new_, old);
    if (new_.size().toSize() != old.size().toSize())
        polish();
}

auto VideoRenderer::type() const -> Type*
{
    static Type type[2];
    return &type[d->osd.visible];
}

auto VideoRenderer::createShader() const -> ShaderIface*
{
    return new VideoShaderIface(d->osd.visible);
}

auto VideoRenderer::createData() const -> ShaderData*
{
    auto data = new VideoShaderData;
    data->frame = &d->frame;
    data->osd = &d->osd;
    return data;
}

auto VideoRenderer::createNode() const -> QSGGeometryNode*
{
    return new Node(const_cast<VideoRenderer*>(this));
}

auto VideoRenderer::render(VideoShaderData *data) -> void
{
    if (!data->redraw)
        return;
    data->redraw = false;
    auto w = window();
    if (w && d->render) {
        w->resetOpenGLState();
        d->render(d->frame.fbo, data->osdVisible ? d->osd.fbo : nullptr, data->osdMargins);
        w->resetOpenGLState();
    }
}

auto VideoRenderer::updateData(ShaderData *_data) -> void
{
    auto data = static_cast<VideoShaderData*>(_data);
    if (!d->redraw) {
        _Trace("VideoRendererItem::updateTexture(): no queued frame");
    } else if (!d->frame.size.isEmpty()) {
        d->redraw = false;
        d->frame.renew();
        d->osd.renew();
        data->redraw = true;
        data->osdMargins = d->osd.margins;
        data->osdVisible = d->osd.visible;
    }
}

#define FILL_TS(...) OGL::CoordAttr::fillTriangleStrip(vertex, &Vertex:: __VA_ARGS__)

auto VideoRenderer::updateVertex(Vertex *vertex) -> void
{
    auto tl = d->frame.rect.topLeft();
    auto br = d->frame.rect.bottomRight();
    if (d->flip_v)
        std::swap(tl.ry(), br.ry());
    if (d->flip_h)
        std::swap(tl.rx(), br.rx());
    FILL_TS(position, {0, 0}, {width(), height()});
    FILL_TS(frameTexCoord, tl, br);
    FILL_TS(osdTexCoord, d->osd.rect.topLeft(), d->osd.rect.bottomRight());
}

auto VideoRenderer::initializeVertex(Vertex *vertex) const -> void
{
    FILL_TS(position, {0, 0}, {1, 1});
    FILL_TS(frameTexCoord, {0, 0}, {1, 1});
    FILL_TS(osdTexCoord, {0, 0}, {0, 0});
}

#undef FILL_TS

auto VideoRenderer::setRotation(Rotation r) -> void
{
    if (_Change(d->rotation, r)) {
        if (_Change(d->portrait, _IsOneOf(r, Rotation::D90, Rotation::D270))) {
            d->frame.size = d->fboSizeHint();
            d->redraw = true;
            reserve(UpdateAll);
        }
        d->dirty = DirtyRot;
        polish();
        reserve(UpdateGeometry);
    }
}

auto VideoRenderer::rotation() const -> Rotation
{
    return d->rotation;
}

auto VideoRenderer::isPortrait() const -> bool
{
    return d->portrait;
}

auto VideoRenderer::mapFromVideo(const QRect &rect) -> QRectF
{
    const auto src = sizeHint();
    const auto rx = d->vtx.width() / src.width();
    const auto ry = d->vtx.height() / src.height();

    QPointF tl = rect.topLeft();
    tl.rx() *= rx;
    tl.ry() *= ry;
    tl += d->vtx.topLeft();

    QSizeF size = rect.size();
    size.rwidth() *= rx;
    size.rheight() *= ry;

    return { tl, size };
}

auto VideoRenderer::setScalerEnabled(bool on) -> void
{
    return;
    if (_Change(d->scaler, on)) {
        d->frame.size = d->fboSizeHint();
        d->redraw = true;
        polish();
        reserve(UpdateAll);
    }
}

auto VideoRenderer::setOsdVisible(bool visible) -> void
{
    if (_Change(d->osd.visible, visible)) {
        d->redraw = true;
        reserve(UpdateAll);
    }
}

auto VideoRenderer::updateAll() -> void
{
    polish();
    reserve(UpdateAll);
}
