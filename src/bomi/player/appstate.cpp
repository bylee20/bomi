#include "appstate.hpp"
#include "mainwindow.hpp"
#include "misc/log.hpp"
#include "misc/json.hpp"
#include "misc/jsonstorage.hpp"
#include "os/os.hpp"
#include "videosettings.hpp"
#include "opengl/openglmisc.hpp"
#include <QScreen>

static_assert(tmp::is_enum_class<StaysOnTop>(), "!!!");

static_assert(detail::jsonValueType<StaysOnTop>() == detail::JsonValueType::Enum, "!!!");

DECLARE_LOG_CONTEXT(App)

#define JSON_CLASS AppState
static const auto jio = JIO(
    JE(win_pos),
    JE(win_size),
    JE(last_win_pos),
    JE(last_win_size),
    JE(auto_exit),
    JE(playlist_visible),
    JE(playlist_shuffled),
    JE(playlist_repeat),
    JE(history_visible),
    JE(playinfo_visible),
    JE(win_stays_on_top),
    JE(win_frameless),
    JE(ask_system_tray),
    JE(dvd_device),
    JE(bluray_device),
    JE(state),
    JE(fbo_format),
    JE(visualization), JE(use_interpolator_down)
);

#define APP_STATE_FILE QString(_WritablePath(Location::Config) % "/appstate.json"_a)

AppState::AppState()
{
    load();
}

auto AppState::load() -> void
{
    JsonStorage storage(APP_STATE_FILE);
    const auto json = storage.read();
    if (storage.hasError()) {
        const auto s = VideoSettings::preset(OGL::is16bitFramebufferFormatSupported()
                                             ? VideoSettings::Normal : VideoSettings::Basic);
        s.fill(&state);
        fbo_format = s.fboFormat;
        use_interpolator_down = s.useIntrplDown;
        return;
    }
    if (!jio.fromJson(*this, json))
        _Error("Cannot convert JSON object to AppState");
}

auto AppState::save() const -> void
{
    JsonStorage storage(APP_STATE_FILE);
    storage.write(jio.toJson(*this));
}

static const QSize s_maximized{-759, -526};
static const QSize s_fullScreen{-333, -333};

static auto saveWindowGeometry(const MainWindow *w, QPointF *p, QSize *s) -> void
{
    *s = w->size();
    auto screen = w->screen()->availableVirtualSize();
    p->rx() = qBound(0.0, w->x()/(double)screen.width(), 1.0);
    p->ry() = qBound(0.0, w->y()/(double)screen.height(), 1.0);
}

static auto loadWindowGeometry(MainWindow *w, const QPointF &p, const QSize &s) -> void
{
    if (s.isValid()) {
        auto screen = w->screen()->availableVirtualSize();
        const int x = screen.width() * p.x();
        const int y = screen.height() * p.y();
        w->setGeometry(QRect({x, y}, s));
    } else
        w->resize(400, 300);
}


auto AppState::updateLastWindowGeometry(const MainWindow *w) -> void
{
    saveWindowGeometry(w, &last_win_pos, &last_win_size);
}

auto AppState::restoreLastWindowGeometry(MainWindow *w) -> void
{
    loadWindowGeometry(w, last_win_pos, last_win_size);
}

auto AppState::updateWindowGeometry(const MainWindow *w) -> void
{
    if (w->adapter()->state() == Qt::WindowMinimized || !w->isVisible())
        return;
    if (w->isFullScreen())
        win_size = s_fullScreen;
    else if (w->adapter()->state() == Qt::WindowMaximized)
        win_size = s_maximized;
    else
        saveWindowGeometry(w, &win_pos, &win_size);
}

auto AppState::restoreWindowGeometry(MainWindow *w) -> void
{
    if (win_size == s_maximized)
        w->adapter()->showMaximized();
    else if (win_size == s_fullScreen)
        w->setFullScreen(true, false);
    else
        loadWindowGeometry(w, win_pos, win_size);
}
