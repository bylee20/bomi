#include "busyiconitem.hpp"
#include "opengl/opengltexture2d.hpp"
#include "opengl/opengltexturebinder.hpp"
#include <QVariantAnimation>

struct ColorRing {
    QColor color;
    QImage image;
    float tx0 = 0, ty0 = 0;
};

enum { Dark, Light };

struct BusyIconItem::Data {
    BusyIconItem *p = nullptr;
    qreal thickness = 20, radius = 1, angle = 0, last = -1;
    bool dirtyGeometry = true, running = true;
    bool upload = false, redraw = false;
    int quater = -1, filled = Light;
    ColorRing rings[2];
    QSize textureSize = {1, 1};
    QPointF textureScale = {1, 1};
    QVariantAnimation prog;

    void reset() {
        angle = 0;
        quater = last = -1;
        filled = true;
        p->update();
    }

    void updateAnimation() {
        if (running && p->isVisible())
            prog.start();
        else
            prog.stop();
    }
};

BusyIconItem::BusyIconItem(QQuickItem *parent)
: SimpleTextureItem(parent), d(new Data) {
    d->p = this;
    setFlag(ItemHasContents, true);
    d->rings[Dark].color = Qt::darkGray;
    d->rings[Light].color = Qt::lightGray;
    d->reset();
    vertices().resize(6*3);

    d->prog.setDuration(1000);
    d->prog.setLoopCount(-1);
    d->prog.setStartValue(0.0);
    d->prog.setEndValue(2.0*M_PI);

    connect(&d->prog, &QVariantAnimation::valueChanged,
            [this] (const QVariant &var) {
        d->angle = var.toDouble();
        polish();
        update();
    });
    d->updateAnimation();
}

BusyIconItem::~BusyIconItem() {
    delete d;
}

auto BusyIconItem::itemChange(ItemChange change,
                              const ItemChangeData &data) -> void
{
    QQuickItem::itemChange(change, data);
    if (change == ItemVisibleHasChanged)
        d->updateAnimation();
}

auto BusyIconItem::isRunning() const -> bool
{
    return d->running;
}

auto BusyIconItem::setRunning(bool running) -> void
{
    if (_Change(d->running, running)) {
        d->reset();
        d->updateAnimation();
        polish();
        emit runningChanged();
    }
}

auto BusyIconItem::darkColor() const -> QColor
{
    return d->rings[Dark].color;
}

auto BusyIconItem::lightColor() const -> QColor
{
    return d->rings[Light].color;
}

auto BusyIconItem::thickness() const -> qreal
{
    return d->thickness;
}

auto BusyIconItem::setDarkColor(const QColor &color) -> void
{
    if (_Change(d->rings[Dark].color, color)) {
        d->reset();
        emit darkColorChanged();
        update();
    }
}

auto BusyIconItem::setLightColor(const QColor &color) -> void
{
    if (_Change(d->rings[Light].color, color)) {
        d->reset();
        emit lightColorChanged();
        update();
    }
}

auto BusyIconItem::setThickness(qreal thickness) -> void
{
    if (_Change(d->thickness, thickness)) {
        d->redraw = true;
        emit thicknessChanged();
        polish();
        update();
    }
}

auto BusyIconItem::geometryChanged(const QRectF &n, const QRectF &o) -> void
{
    QQuickItem::geometryChanged(n, o);
    polish();
    update();
}

auto BusyIconItem::initializeGL() -> void
{
    SimpleTextureItem::initializeGL();
    OpenGLTexture2D texture;
    texture.create();
    texture.setAttributes(0, 0, OpenGLTextureTransferInfo::get(OGL::BGRA));
    setTexture(texture);
}

auto BusyIconItem::finalizeGL() -> void
{
    SimpleTextureItem::finalizeGL();
    texture().destroy();
}

auto BusyIconItem::updateTexture(OpenGLTexture2D *texture) -> void
{
    if (d->upload) {
        const int len = d->textureSize.height();
        OpenGLTextureBinder<OGL::Target2D> binder(texture);
        if (texture->height() < len)
            texture->initialize(d->textureSize);
        texture->upload(0, 0, len*2, len, d->rings[0].image.bits());
        texture->upload(len*2+5, 0, len*2, len, d->rings[1].image.bits());
        d->upload = false;
    }
}

auto BusyIconItem::updatePolish() -> void
{
    if (_Change(d->radius, qMin(width(), height())*0.5) || d->redraw) {
        const int len = d->radius + 1.5;
        d->textureSize.rwidth() = len * 4 + 5;
        d->textureSize.rheight() = len;
        d->textureScale.rx() = 1.0 / (len * 4 + 5);
        d->textureScale.ry() = 1.0 / len;
        d->rings[0].tx0 = d->radius * d->textureScale.x();
        d->rings[1].tx0 = (len*2 + 5 + d->radius) * d->textureScale.x();
        for (int i=0; i<2; ++i) {
            auto &image = d->rings[i].image;
            image = QImage(len*2, len, QImage::Format_ARGB32_Premultiplied);
            image.fill(0x0);
            QPainter painter(&image);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setBrush(QBrush(d->rings[i].color));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(QPointF(d->radius, 0), d->radius, d->radius);
            painter.setCompositionMode(QPainter::CompositionMode_SourceOut);
            painter.setBrush(QBrush(QColor(Qt::red)));
            const auto in = qMax(d->radius - d->thickness, 0.0);
            painter.drawEllipse(QPointF(d->radius, 0), in, in);
        }
        d->upload = true;
        d->redraw = false;
        reserve(UpdateMaterial);
    }
    QPointF o(d->radius, d->radius);
    if (width() < height())
        o.ry() += 0.5*(height() - width());
    else if (height() < width())
        o.rx() += 0.5*(width() - height());
    const float txrad = d->radius*d->textureScale.x();
    const float tymax = d->radius*d->textureScale.y();

    auto it = vertices().begin();
    auto fill = [&] (int index, double tan1, double tan2, int quater) {
        const float tx0 = d->rings[index].tx0;
        const float ty0 = d->rings[index].ty0;
        QMatrix4x4 mat;
        mat.translate(o.x(), o.y());
        mat.rotate(90.0*quater, 0, 0, 1);
        it->position.set(mat*QPointF{d->radius*tan1, -d->radius});
        it->texCoord.set(tx0 + txrad*tan1, ty0 + tymax);
        ++it;
        it->position.set(mat*QPointF{d->radius*tan2, -d->radius});
        it->texCoord.set(tx0 + txrad*tan2, ty0 + tymax);
        ++it;
        it->position.set(mat*QPointF{0, 0});
        it->texCoord.set(tx0, ty0);
        ++it;
    };

    if (d->angle < d->last)
        d->filled = !d->filled;
    d->last = d->angle;

    if (d->angle <= M_PI/4.0) {
        const auto t = qTan(d->angle);
        fill( d->filled,  0, t, 0);
        fill(!d->filled,  t, 1, 0);
        if (_Change(d->quater, 0)) {
            fill(!d->filled, -1, 0, 0);
            fill(!d->filled, -1, 1, 1);
            fill(!d->filled, -1, 1, 2);
            fill(!d->filled, -1, 1, 3);
        }
    } else if (d->angle <= M_PI*3/4) {
        const auto t = qTan(d->angle - M_PI/2);
        fill( d->filled, -1, t, 1);
        fill(!d->filled,  t, 1, 1);
        if (_Change(d->quater, 1)) {
            fill(!d->filled, -1, 0, 0);
            fill( d->filled,  0, 1, 0);
            fill(!d->filled, -1, 1, 2);
            fill(!d->filled, -1, 1, 3);
        }
    } else if (d->angle <= M_PI*5/4) {
        const auto t = qTan(d->angle - M_PI);
        fill( d->filled, -1, t, 2);
        fill(!d->filled,  t, 1, 2);
        if (_Change(d->quater, 2)) {
            fill(!d->filled, -1, 0, 0);
            fill( d->filled,  0, 1, 0);
            fill( d->filled, -1, 1, 1);
            fill(!d->filled, -1, 1, 3);
        }
    } else if (d->angle <= M_PI*7/4) {
        const auto t = qTan(d->angle - M_PI*3/2);
        fill( d->filled, -1, t, 3);
        fill(!d->filled,  t, 1, 3);
        if (_Change(d->quater, 3)) {
            fill(!d->filled, -1, 0, 0);
            fill( d->filled,  0, 1, 0);
            fill( d->filled, -1, 1, 1);
            fill( d->filled, -1, 1, 2);
        }
    } else {
        const auto t = qTan(d->angle - 2*M_PI);
        fill( d->filled, -1, t, 0);
        fill(!d->filled,  t, 0, 0);
        if (_Change(d->quater, 4)) {
            fill( d->filled,  0, 1, 0);
            fill( d->filled, -1, 1, 1);
            fill( d->filled, -1, 1, 2);
            fill( d->filled, -1, 1, 3);
        }
    }
    reserve(UpdateGeometry);
}
