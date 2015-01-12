#include "appstate.hpp"
#include "misc/log.hpp"
#include "misc/json.hpp"
#include "misc/jsonstorage.hpp"

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

#define APP_STATE_FILE QString(_WritablePath() % "/appstate.json"_a)

AppState::AppState() {
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
