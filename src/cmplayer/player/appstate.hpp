#ifndef APPSTATE_HPP
#define APPSTATE_HPP

#include "mrlstate.hpp"
#include "enum/staysontop.hpp"

class AppState : public QObject {
    Q_OBJECT
    Q_PROPERTY(StaysOnTop win_stays_on_top MEMBER win_stays_on_top NOTIFY winStaysOnTopChanged)
public:
    AppState();

    QPointF win_pos;
    QSize win_size;
    MrlState state;
    // tool state
    bool auto_exit = false;
    bool playlist_visible = false;
    bool history_visible = false;
    bool playinfo_visible = false;
    // window state
    StaysOnTop win_stays_on_top = StaysOnTop::Playing;

    // misc
    bool ask_system_tray = true;

    QString dvd_device, bluray_device;
    auto save() const -> void;
signals:
    void winStaysOnTopChanged();
private:
    auto loadFromRecord() -> void;
};

#endif // APPSTATE_HPP
