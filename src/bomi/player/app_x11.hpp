#ifndef APP_X11_HPP
#define APP_X11_HPP

#include <QObject>

class AppX11 : public QObject {
    Q_OBJECT
public:
    AppX11(QObject *parent = 0);
    ~AppX11();
    auto setScreensaverDisabled(bool disabled) -> void;
    auto setHeartbeat(const QString &command, int interval) -> void;
    auto setAlwaysOnTop(QWidget *widget, bool onTop) -> void;
    auto devices() const -> QStringList;
    auto shutdown() -> bool;
    auto refreshRate() const -> qreal;
private:
    AppX11(const AppX11&) = delete;
    AppX11 &operator = (const AppX11&) = delete;
    struct Data;
    Data *d = nullptr;
};

#endif // APPLICATION_X11_HPP
