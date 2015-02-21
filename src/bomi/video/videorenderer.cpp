#include "videorenderer.hpp"
#include "letterboxitem.hpp"
#include "opengl/opengltexture2d.hpp"
#include "opengl/openglframebufferobject.hpp"
#include "opengl/opengltexturebinder.hpp"
#include "misc/dataevent.hpp"
#include "misc/log.hpp"
#include <QQmlProperty>
#include <QQuickWindow>

DECLARE_LOG_CONTEXT(Video)

enum EventType {NewFrame = QEvent::User + 1 };


struct VideoRenderer::VideoShaderData : public VideoRenderer::ShaderData {
    const OpenGLTexture2D *texture = nullptr;
};

struct VideoRenderer::VideoShaderIface : public VideoRenderer::ShaderIface {
    VideoShaderIface() {
        vertexShader = R"(
            uniform mat4 qt_Matrix;
            attribute vec4 aPosition;
            attribute vec2 aTexCoord;
            varying vec2 texCoord;
            void main() {
                texCoord = aTexCoord;
                gl_Position = qt_Matrix * aPosition;
            }
        )";
        fragmentShader = R"(
            varying vec2 texCoord;
            uniform float qt_Opacity;
            uniform sampler2D tex;
            void main() {
                gl_FragColor = texture2D(tex, texCoord) * qt_Opacity;
            }
        )";
        attributes << "aPosition" << "aTexCoord";
    }
    void resolve(QOpenGLShaderProgram *prog) override {
        Q_ASSERT(prog->isLinked());
        loc_tex = prog->uniformLocation("tex");
    }
    void update(QOpenGLShaderProgram *prog,
                const VideoRenderer::ShaderData *data) override {
        auto d = static_cast<const VideoShaderData*>(data);
        if (d->texture->id() != GL_NONE && !d->texture->isEmpty()) {
            prog->setUniformValue(loc_tex, 0);
            func()->glActiveTexture(GL_TEXTURE0);
            d->texture->bind();
        }
    }
private:
    int loc_tex = -1;
};

struct VideoRenderer::Data {
    VideoRenderer *p = nullptr;
    double crop = -1.0, aspect = -1.0, dar = 0.0;
    bool onLetterbox = true, redraw = false;
    bool flip_h = false, flip_v = false;
    Qt::Alignment alignment = Qt::AlignCenter;
    QRectF vtx; QPoint offset = {0, 0};
    LetterboxItem *letterbox = nullptr;
    GeometryItem *overlay = nullptr;
    OpenGLTexture2D black;
    OpenGLFramebufferObject *fbo = nullptr;
    QSize displaySize{0, 1}, fboSize, prevSize;
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
    auto fboSizeHint() const -> QSize
    {
        auto size = displaySize;
        size.scale(vtx.width() + 0.5, vtx.height() + 0.5, Qt::KeepAspectRatio);
        return size;
    }
    auto updateFboSize(const QSize &size) -> void
    {
        if (_Change(fboSize, size)) {
            redraw = true;
            p->reserve(UpdateAll);
        }
    }
};

VideoRenderer::VideoRenderer(QQuickItem *parent)
    : ShaderRenderItem<OGL::TextureVertex>(parent), d(new Data)
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
        if (_Change(d->prevSize, d->fboSizeHint())) {
            d->sizeChecker.stop();
            d->updateFboSize(d->prevSize);
        }
    });
    d->sizeChecker.setInterval(300);
    d->sizeChecker.setSingleShot(true);
}

VideoRenderer::~VideoRenderer() {
    delete d;
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
    ShaderRenderItem<OGL::TextureVertex>::initializeGL();
    d->black.create(OGL::Repeat);
    OpenGLTextureBinder<OGL::Target2D> binder(&d->black);
    const quint32 p = 0x0;
    d->black.initialize(1, 1, OGL::BGRA, &p);
}

auto VideoRenderer::finalizeGL() -> void
{
    ShaderRenderItem<OGL::TextureVertex>::finalizeGL();
    d->black.destroy();
    _Delete(d->fbo);
}

auto VideoRenderer::customEvent(QEvent *event) -> void
{
    switch (static_cast<int>(event->type())) {
    case NewFrame: {
        auto ds = _GetData<QSize>(event);
        if (_Change(d->displaySize, ds)) {
            reserve(UpdateGeometry, false);
            d->fboSize = d->fboSizeHint();
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
    return !d->displaySize.isEmpty();
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

auto VideoRenderer::setOffset(const QPoint &offset) -> void
{
    if (_Change(d->offset, offset)) {
        polish();
        emit offsetChanged(d->offset);
    }
}

auto VideoRenderer::offset() const -> QPoint
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
    size.scale(d->displaySize, Qt::KeepAspectRatioByExpanding);
    QSizeF crop(d->targetCropRatio(aspect), 1.0);
    crop.scale(size, Qt::KeepAspectRatio);
    return crop.toSize();
}

auto VideoRenderer::updatePolish() -> void
{
    ShaderRenderItem<OGL::TextureVertex>::updatePolish();
    QRectF letter;
    if (_Change(d->vtx, d->frameRect(geometry(), d->offset, &letter))) {
        d->sizeChecker.start();
        reserve(UpdateGeometry, false);
    }
    if (d->letterbox->set(rect(), letter))
        emit screenRectChanged(d->letterbox->screen());
    if (d->overlay) {
        const auto g = d->onLetterbox ? rect() : d->letterbox->screen();
        d->overlay->setGeometry(g);
    }
}

//auto VideoRenderer::updateTexture(OpenGLTexture2D *texture) -> void
//{

//}

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
    ShaderRenderItem<OGL::TextureVertex>::geometryChanged(new_, old);
    if (new_.size().toSize() != old.size().toSize())
        polish();
}


auto VideoRenderer::createShader() const -> ShaderIface*
{
    return new VideoShaderIface;
}

auto VideoRenderer::createData() const -> ShaderData*
{
    auto data = new VideoShaderData;
    data->texture = &d->black;
    return data;
}

auto VideoRenderer::updateData(ShaderData *_data) -> void
{
    auto data = static_cast<VideoShaderData*>(_data);
    if (!d->redraw) {
        _Trace("VideoRendererItem::updateTexture(): no queued frame");
    } else if (!d->fboSize.isEmpty()) {
        d->redraw = false;
        if (!d->fbo || d->fbo->size() != d->fboSize)
            _Renew(d->fbo, d->fboSize);
        auto w = window();
        if (w && d->render) {
            w->resetOpenGLState();
            d->render(d->fbo);
            w->resetOpenGLState();
        }
    }
    data->texture = !d->fboSize.isEmpty() && d->fbo ? &d->fbo->texture() : &d->black;
}

auto VideoRenderer::updateVertex(Vertex *vertex) -> void
{
    double top = 0.0, left = 0.0, right = 1.0, bottom = 1.0;
    if (d->flip_v)
        std::swap(top, bottom);
    if (d->flip_h)
        std::swap(left, right);
    Vertex::fillAsTriangleStrip(vertex, d->vtx.topLeft(), d->vtx.bottomRight(),
                                {left, top}, {right, bottom});
}

auto VideoRenderer::initializeVertex(Vertex *vertex) const -> void
{
    Vertex::fillAsTriangleStrip(vertex, {0, 0}, {0, 0}, {0, 0}, {1, 1});
}


