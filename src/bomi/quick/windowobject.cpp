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
    connect(m, &MainWindow::heightChanged, this, &WindowObject::heightChanged);
    connect(m, &MainWindow::heightChanged, this, &WindowObject::sizeChanged);
    connect(m, &MainWindow::widthChanged, this, &WindowObject::widthChanged);
    connect(m, &MainWindow::widthChanged, this, &WindowObject::sizeChanged);
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
