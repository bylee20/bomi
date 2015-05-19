#include "maskareaitem.hpp"
#include "opengl/opengltexture2d.hpp"
#include "opengl/opengltexturebinder.hpp"
#include <QQmlFile>
#include <QStyleHints>

struct MaskAreaItem::Data {
    MaskAreaItem *p = nullptr;
    QUrl url;
    QImage mask;
    qreal alpha = -1.0;
    qreal x = -1, y = -1;
    bool pressed = false, hovered = false;
    QPointF pressedPos;
    auto setPressed(bool r)
    {
        if (_Change(pressed, r))
            emit p->pressedChanged(pressed);
    }
    auto checkHovered(const QPointF &pos) -> void
    {
        if (_Change(x, pos.x()))
            emit p->mouseXChanged();
        if (_Change(y, pos.y()))
            emit p->mouseYChanged();
        setHovered(p->contains(pos));
    }

    auto setHovered(bool h) -> void
    {
        if (_Change(hovered, h)) {
            emit p->hoveredChanged(hovered);
            if (hovered)
                emit p->entered();
            else
                emit p->exited();
        }
    }
    auto useMask() const -> bool { return 0 <= alpha && alpha <= 1; }
};

MaskAreaItem::MaskAreaItem(QQuickItem *parent)
    : QQuickItem(parent), d(new Data)
{
    d->p = this;
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton);
}

MaskAreaItem::~MaskAreaItem()
{
    delete d;
}

auto MaskAreaItem::source() const -> QUrl
{
    return d->url;
}

auto MaskAreaItem::contains(qreal x, qreal y) const -> bool
{
    if (!_InRange(0.0, x, width()) || !_InRange(0.0, y, height()))
        return false;
    if (!d->useMask())
        return true;
    if (d->mask.isNull())
        return false;
    auto ix = x * (d->mask.width() - 1.0) / width();
    auto iy = y * (d->mask.height() - 1.0) / height();
    return qAlpha(d->mask.pixel(ix, iy)) / 255.0 > d->alpha;
}

auto MaskAreaItem::setSource(const QUrl &url) -> void
{
    if (_Change(d->url, url)) {
        d->mask.load(QQmlFile::urlToLocalFileOrQrc(d->url));
        emit sourceChanged();
    }
}

auto MaskAreaItem::setAlpha(qreal alpha) -> void
{
    if (_Change(d->alpha, alpha))
        emit maskAlphaChanged();
}

auto MaskAreaItem::alpha() const -> qreal
{
    return d->alpha;
}

auto MaskAreaItem::mouseX() const -> qreal
{
    return d->x;
}

auto MaskAreaItem::mouseY() const -> qreal
{
    return d->y;
}

auto MaskAreaItem::mousePressEvent(QMouseEvent *event) -> void
{
    QQuickItem::mousePressEvent(event);
    if (!contains(event->localPos()))
        return;
    event->setAccepted(true);
    d->setHovered(true);
    MouseEventObject mouse(event);
    emit pressed(&mouse);
    d->setPressed(event->isAccepted());
    if (d->pressed)
        d->pressedPos = event->pos();
}

auto MaskAreaItem::mouseReleaseEvent(QMouseEvent *event) -> void
{
    QQuickItem::mouseReleaseEvent(event);
    d->checkHovered(event->localPos());
    d->setPressed(false);
    MouseEventObject mouse(event);
    emit released(&mouse);
    const int th = qApp->styleHints()->startDragDistance();
    if (th >= qAbs(event->x() - d->pressedPos.x())
            && th >= qAbs(event->y() - d->pressedPos.y())) {
        MouseEventObject mouse(event);
        emit clicked(&mouse);
    }
}

auto MaskAreaItem::mouseMoveEvent(QMouseEvent *event) -> void
{
    QQuickItem::mouseMoveEvent(event);
    d->checkHovered(event->localPos());
}

auto MaskAreaItem::hoverEnterEvent(QHoverEvent *event) -> void
{
    QQuickItem::hoverEnterEvent(event);
    d->checkHovered(event->pos());
}

auto MaskAreaItem::hoverLeaveEvent(QHoverEvent *event) -> void
{
    QQuickItem::hoverLeaveEvent(event);
    d->setHovered(false);
}

auto MaskAreaItem::hoverMoveEvent(QHoverEvent *event) -> void
{
    QQuickItem::hoverMoveEvent(event);
    d->checkHovered(event->pos());
//    event->setAccepted(d->hovered);
}

auto MaskAreaItem::mouseUngrabEvent() -> void
{
    QQuickItem::mouseUngrabEvent();
    emit canceled();
}

auto MaskAreaItem::mouseDoubleClickEvent(QMouseEvent *event) -> void
{
    QQuickItem::mouseDoubleClickEvent(event);
    emit doubleClicked();
}

auto MaskAreaItem::isPressed() const -> bool
{
    return d->pressed;
}

auto MaskAreaItem::isHovered() const -> bool
{
    return d->hovered;
}
