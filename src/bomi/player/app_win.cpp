#include "app_win.hpp"
#ifdef Q_OS_WIN

struct AppWin::Data {

};

AppWin::AppWin(QObject *parent)
    : QObject(parent), d(new Data)
{

}

AppWin::~AppWin()
{
    delete d;
}

auto AppWin::setScreensaverDisabled(bool disabled) -> void
{

}

auto AppWin::setHeartbeat(const QString &command, int interval) -> void
{

}

auto AppWin::setAlwaysOnTop(QWidget *widget, bool onTop) -> void
{

}

auto AppWin::setFullScreen(QWidget *widget, bool fs) -> void
{
    auto states = widget->windowState();
    if (fs)
        states |= Qt::WindowFullScreen;
    else
        states &= ~Qt::WindowFullScreen;
    widget->setWindowState(states);
}

auto AppWin::devices() const -> QStringList
{

}

auto AppWin::shutdown() -> bool
{

}

auto AppWin::refreshRate() const -> qreal
{
    return 60;
}

#endif
