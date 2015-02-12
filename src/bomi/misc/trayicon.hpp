#ifndef TRAYICON_HPP
#define TRAYICON_HPP

#include <QSystemTrayIcon>

class TrayIcon : public QObject {
    Q_OBJECT
public:
    enum ActivationReason {
        Quit = -1, Show = -2,
        Trigger = QSystemTrayIcon::Trigger,
        Context = QSystemTrayIcon::Context
    };
    TrayIcon(const QIcon &icon, QObject *parent = nullptr);
    ~TrayIcon();
    auto setVisible(bool visible) -> void;
    static auto isAvailable() -> bool;
    static auto isUnity() -> bool;
signals:
    void activated(ActivationReason reason);
private:
    static auto onQuit(void *menu, void *arg) -> void;
    static auto onShow(void *menu, void *arg) -> void;
    struct Data;
    Data *d;
};

inline auto TrayIcon::isUnity() -> bool
{ return (qgetenv("XDG_CURRENT_DESKTOP").toLower() == "unity"); }

#endif // TRAYICON_HPP
