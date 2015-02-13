#ifndef APPWIN_H
#define APPWIN_H

#include <QObject>

class AppWin : public QObject {
    Q_OBJECT
public:
    AppWin(QObject *parent = 0);
    ~AppWin();
    auto setScreensaverDisabled(bool disabled) -> void;
    auto setHeartbeat(const QString &command, int interval) -> void;
    auto setAlwaysOnTop(QWidget *widget, bool onTop) -> void;
    auto setFullScreen(QWidget *widget, bool fs) -> void;
    auto devices() const -> QStringList;
    auto shutdown() -> bool;
    auto refreshRate() const -> qreal;
private:
    struct Data;
    Data *d = nullptr;
};


#endif // APPWIN_H
