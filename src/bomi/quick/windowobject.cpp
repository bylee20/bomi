#include "windowobject.hpp"
#include "player/mainwindow.hpp"
#include "os/os.hpp"
#include <QQuickWindow>
#include <QToolTip>

auto MouseObject::updateCursor(Qt::CursorShape shape) -> void
{
    if (_Change(m_cursor, shape == Qt::BlankCursor ? NoCursor : Arrow))
        emit cursorChanged();
}

bool MouseObject::isIn(QQuickItem *item, const QRectF &rect)
{
    if (!item || !item->window())
        return false;
    auto pos = posFor(item);
    return rect.contains(pos) && item->contains(pos);
}

auto MouseObject::isIn(QQuickItem *item) -> bool
{
    if (!item || !item->window())
        return false;
    return item->contains(posFor(item));
}

auto MouseObject::posFor(QQuickItem *item) -> QPointF
{
    if (!item || !item->window())
        return QPointF();
    return item->mapFromScene(item->window()->mapFromGlobal(QCursor::pos()));
}

/******************************************************************************/

auto WindowObject::set(MainWindow *mw) -> void
{
    m = mw;
    m_z10.setZ(10);
    connect(m, &MainWindow::framelessChanged, this, &WindowObject::framelessChanged);
    connect(m, &MainWindow::fullscreenChanged, this, &WindowObject::fullscreenChanged);
    connect(m, &MainWindow::windowStateChanged, this, [=] (auto state) {
        if (_Change(m_minimized, state == Qt::WindowMinimized))
            emit this->minimizedChanged();
        if (_Change(m_maximized, state == Qt::WindowMaximized))
            emit this->maximizedChanged();
    });
    connect(m, &MainWindow::heightChanged, this, [=] () {
        emit heightChanged();
        emit sizeChanged();
        m_z10.setHeight(m->height());
    });
    connect(m, &MainWindow::widthChanged, this, [=] () {
        emit widthChanged();
        emit sizeChanged();
        m_z10.setWidth(m->width());
    });
    connect(m, &QQuickView::statusChanged, this, [=] (QQuickView::Status s) {
        const auto root = s == QQuickView::Ready ? m->rootObject() : nullptr;
        m_z10.setParentItem(root);
    });
}

auto WindowObject::fullscreen() const -> bool
{
    return m->isFullScreen();
}

auto WindowObject::showToolTip(QQuickItem *item, const QPointF &pos,
                               const QString &text) -> void
{
    const auto p = item->mapToScene(pos).toPoint();
    QToolTip::showText(item->window()->mapToGlobal(p), text);
}

auto WindowObject::hideToolTip() -> void
{
    QToolTip::hideText();
}

auto WindowObject::getMouse() -> MouseObject*
{
    static MouseObject mouse;
    return &mouse;
}

auto WindowObject::size() const -> QSize
{
    return m->size();
}

auto WindowObject::width() const -> int
{
    return m->width();
}

auto WindowObject::height() const -> int
{
    return m->height();
}

auto WindowObject::showNormal() -> void
{
    m->showNormal();
}

auto WindowObject::isMinimized() const -> bool
{
    return m_minimized;
}

auto WindowObject::isMaximized() const -> bool
{
    return m_maximized;
}

auto WindowObject::isFrameless() const -> bool
{
    return m->isFrameless();
}
