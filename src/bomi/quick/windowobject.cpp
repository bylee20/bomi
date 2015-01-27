#include "windowobject.hpp"

auto WindowObject::set(MainWindow *mw) -> void
{
    m = mw;
    connect(m, &MainWindow::fullscreenChanged, this, &WindowObject::fullscreenChanged);
}
