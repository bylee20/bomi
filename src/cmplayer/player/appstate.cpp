#include "appstate.hpp"
#include "misc/record.hpp"
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
    JE(history_visible),
    JE(playinfo_visible),
    JE(win_stays_on_top),
    JE(ask_system_tray),
    JE(dvd_device),
    JE(bluray_device),
    JE(state)
);

#define APP_STATE_FILE QString(_WritablePath() % "/appstate.json")

AppState::AppState() {
    JsonStorage storage(APP_STATE_FILE);
    const auto json = storage.read();
    if (storage.hasError()) {
        if (storage.error() == JsonStorage::NoFile)
            loadFromRecord();
        return;
    }
    if (!jio.fromJson(*this, json))
        _Error("Cannot convert JSON object to AppState");
}

auto AppState::loadFromRecord() -> void
{
    Record r("app-state");
#define READ(a) r.read(a, #a)
    READ(win_stays_on_top);

    READ(ask_system_tray);

    READ(auto_exit);

    READ(win_pos);
    READ(win_size);

    READ(playlist_visible);
    READ(history_visible);
    READ(playinfo_visible);
    READ(dvd_device);
    READ(bluray_device);
#undef READ
}

auto AppState::save() const -> void
{
    JsonStorage storage(APP_STATE_FILE);
    storage.write(jio.toJson(*this));
}
