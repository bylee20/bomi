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
    JE(bluray_device)
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

AppStateOld::AppStateOld() {
    Record r("app-state");

#define READ(a) r.read(a, #a)
    READ(playback_speed);
    READ(video_aspect_ratio);
    READ(video_crop_ratio);
    READ(video_vertical_alignment);
    READ(video_horizontal_alignment);
    READ(video_offset);
    READ(video_effects);
    READ(video_color);
    READ(video_deinterlacing);
    READ(video_interpolator);
    READ(video_chroma_upscaler);
    READ(video_dithering);
    READ(video_range);

    READ(audio_volume);
    READ(audio_amplifier);
    READ(audio_volume_normalizer);
    READ(audio_tempo_scaler);
    READ(audio_muted);
    READ(audio_sync);
    READ(audio_channel_layout);

    READ(sub_position);
    READ(sub_display);
    READ(sub_alignment);
    READ(sub_sync);

    READ(window_stays_on_top);

    READ(open_folder_types);
    READ(open_last_folder);
    READ(open_last_file);
    READ(open_url_list);
    READ(open_url_enc);
    READ(ask_system_tray);

    READ(auto_exit);

    READ(win_pos);
    READ(win_size);

    READ(playlist_visible);
    READ(history_visible);
    READ(playinfo_visible);
    READ(dvd_menu);
    READ(dvd_device);
#undef READ
}

auto AppStateOld::save() const -> void
{
    Record r("app-state");
#define WRITE(a) r.write(a, #a);
    WRITE(playback_speed);

    WRITE(video_aspect_ratio);
    WRITE(video_crop_ratio);
    WRITE(video_vertical_alignment);
    WRITE(video_horizontal_alignment);
    WRITE(video_offset);
    WRITE(video_effects);
    WRITE(video_color);
    WRITE(video_deinterlacing);
    WRITE(video_chroma_upscaler);
    WRITE(video_interpolator);
    WRITE(video_dithering);
    WRITE(video_range);

    WRITE(audio_volume);
    WRITE(audio_volume_normalizer);
    WRITE(audio_tempo_scaler);
    WRITE(audio_muted);
    WRITE(audio_amplifier);
    WRITE(audio_sync);
    WRITE(audio_channel_layout);

    WRITE(sub_position);
    WRITE(sub_display);
    WRITE(sub_alignment);
    WRITE(sub_sync);

    WRITE(window_stays_on_top);

    WRITE(open_folder_types);
    WRITE(open_last_folder);
    WRITE(open_last_file);
    WRITE(ask_system_tray);
    WRITE(open_url_list);
    WRITE(open_url_enc);

    WRITE(auto_exit);

    WRITE(win_pos);
    WRITE(win_size);

    WRITE(playlist_visible);
    WRITE(history_visible);
    WRITE(playinfo_visible);
    WRITE(dvd_menu);
    WRITE(dvd_device);
#undef WRITE
}
