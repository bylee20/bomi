#ifndef APPSTATE_HPP
#define APPSTATE_HPP

#include "mrlstate.hpp"
#include "enum/staysontop.hpp"
#include "enum/framebufferobjectformat.hpp"
#include "enum/visualization.hpp"

class MainWindow;

class AppState : public QObject {
    Q_OBJECT
    Q_PROPERTY(StaysOnTop win_stays_on_top MEMBER win_stays_on_top NOTIFY winStaysOnTopChanged)
    Q_PROPERTY(FramebufferObjectFormat fbo_format MEMBER fbo_format NOTIFY fboFormatChanged)
    Q_PROPERTY(Visualization visualization MEMBER visualization NOTIFY visualizationChanged)
    Q_PROPERTY(bool use_interpolator_down MEMBER use_interpolator_down NOTIFY useInterpolatorDownChanged)
public:
    AppState();

    MrlState state;

    QPointF win_pos;
    QSize win_size;

    QPointF last_win_pos;
    QSize last_win_size;

    // tool state
    bool auto_exit = false;
    bool playlist_visible = false;
    bool playlist_shuffled = false;
    bool playlist_repeat = false;
    bool history_visible = false;
    bool playinfo_visible = false;
    // window state
    StaysOnTop win_stays_on_top = StaysOnTop::Playing;
    bool win_frameless = false;

    // misc
    bool ask_system_tray = true, use_interpolator_down = false;
    FramebufferObjectFormat fbo_format = FramebufferObjectFormat::Auto;
    Visualization visualization = Visualization::Off;
    QString dvd_device, bluray_device;

    auto updateWindowGeometry(const MainWindow *w) -> void;
    auto restoreWindowGeometry(MainWindow *w) -> void;

    auto updateLastWindowGeometry(const MainWindow *w) -> void;
    auto restoreLastWindowGeometry(MainWindow *w) -> void;

    auto load() -> void;
    auto save() const -> void;
signals:
    void useInterpolatorDownChanged(bool use);
    void winStaysOnTopChanged(StaysOnTop top);
    void fboFormatChanged(FramebufferObjectFormat format);
    void visualizationChanged(Visualization vis);
};

#endif // APPSTATE_HPP
