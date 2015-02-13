#include "appstate.hpp"
#include "mainwindow.hpp"
#include "misc/log.hpp"
#include "misc/json.hpp"
#include "misc/jsonstorage.hpp"
#include "os/os.hpp"
#include <QScreen>

static_assert(tmp::is_enum_class<StaysOnTop>(), "!!!");

static_assert(detail::jsonValueType<StaysOnTop>() == detail::JsonValueType::Enum, "!!!");

DECLARE_LOG_CONTEXT(App)

#define JSON_CLASS AppState
static const auto jio = JIO(
    JE(win_pos),
    JE(win_size),
    JE(auto_exit),
    JE(playlist_visible),
    JE(playlist_shuffled),
    JE(playlist_repeat),
    JE(history_visible),
    JE(playinfo_visible),
    JE(win_stays_on_top),
    JE(ask_system_tray),
    JE(dvd_device),
    JE(bluray_device),
    JE(sub_find_lang_code),
    JE(state)
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
    if (storage.hasError())
        return;
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

auto AppState::updateWindowGeometry(const MainWindow *w) -> void
{
    if (w->isMinimized() || !w->isVisible())
        return;
    if (w->isFullScreen())
        win_size = s_fullScreen;
    else if (w->isMaximized())
        win_size = s_maximized;
    else {
        win_size = w->size();
        auto screen = w->screen()->availableVirtualSize();
        win_pos.rx() = qBound(0.0, w->x()/(double)screen.width(), 1.0);
        win_pos.ry() = qBound(0.0, w->y()/(double)screen.height(), 1.0);
    }
}

auto AppState::restoreWindowGeometry(MainWindow *w) -> void
{
    if (win_size == s_maximized)
        w->showMaximized();
    else if (win_size == s_fullScreen)
        w->setFullScreen(true);
    else if (win_size.isValid()) {
        auto screen = w->screen()->availableVirtualSize();
        const int x = screen.width() * win_pos.x();
        const int y = screen.height() * win_pos.y();
        w->setGeometry(QRect({x, y}, win_size));
    } else
        w->resize(400, 300);
    w->setMinimumSize(QSize(400, 300));
}
