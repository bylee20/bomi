#ifndef OS_HPP
#define OS_HPP

namespace OS {

auto initialize() -> void;
auto finalize() -> void;

auto isFullScreen(const QWidget *w) -> bool;
auto isAlwaysOnTop(const QWidget *w) -> bool;
auto setAlwaysOnTop(QWidget *w, bool onTop) -> void;
auto setFullScreen(QWidget *w, bool fs) -> void;
auto setScreensaverDisabled(bool disabled) -> void;
auto shutdown() -> bool;
auto canShutdown() -> bool;
auto processTime() -> quint64;
auto systemTime() -> quint64;

auto opticalDrives() -> QStringList;
auto refreshRate() -> qreal;

}

#endif // OS_HPP
