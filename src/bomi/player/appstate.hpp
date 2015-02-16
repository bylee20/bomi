#ifndef APPSTATE_HPP
#define APPSTATE_HPP

#include "mrlstate.hpp"
#include "enum/staysontop.hpp"

class MainWindow;

class AppState : public QObject {
    Q_OBJECT
    Q_PROPERTY(StaysOnTop win_stays_on_top MEMBER win_stays_on_top NOTIFY winStaysOnTopChanged)
public:
    AppState();

    MrlState state;

    QPointF win_pos;
    QSize win_size;
    // tool state
    bool auto_exit = false;
    bool playlist_visible = false;
    bool playlist_shuffled = false;
    bool playlist_repeat = false;
    bool history_visible = false;
    bool playinfo_visible = false;
    // window state
    StaysOnTop win_stays_on_top = StaysOnTop::Playing;

    // misc
    bool ask_system_tray = true;

    QString dvd_device, bluray_device;

    auto updateWindowGeometry(const MainWindow *w) -> void;
    auto restoreWindowGeometry(MainWindow *w) -> void;

    auto load() -> void;
    auto save() const -> void;
signals:
    void winStaysOnTopChanged(StaysOnTop top);
};

#endif // APPSTATE_HPP
