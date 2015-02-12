#include "windowobject.hpp"
#include "player/mainwindow.hpp"
#include <QQuickWindow>
#include <QToolTip>

auto MouseObject::updateCursor(Qt::CursorShape shape) -> void
{
    if (_Change(m_cursor, shape == Qt::BlankCursor ? NoCursor : Arrow))
        emit cursorChanged();
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
    connect(m, &MainWindow::fullscreenChanged, this, &WindowObject::fullscreenChanged);
    connect(m, &MainWindow::cursorChanged, this,
            [=] (const QCursor &c) { m_mouse.updateCursor(c.shape()); });
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
