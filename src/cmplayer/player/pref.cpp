#include "pref.hpp"
#include "misc/log.hpp"
#include "mrlstate.hpp"
#include "video/hwacc.hpp"
#include "misc/record.hpp"
#include "misc/json.hpp"
#include "misc/jsonstorage.hpp"

DECLARE_LOG_CONTEXT(Pref)

template<>
struct JsonIO<QMetaProperty> {
    auto toJson(const QMetaProperty &prop) const -> QJsonValue
    { return _L(prop.name()); }
    auto fromJson(QMetaProperty &prop, const QJsonValue &json) const -> bool
    {
        const auto &mo = MrlState::staticMetaObject;
        const int idx = mo.indexOfProperty(json.toString().toLatin1());
        if (idx < 0)
            return false;
        prop = mo.property(idx);
        return true;
    }
    SCA qt_type = QJsonValue::String;
};

template<>
struct JsonIO<HwAcc::Type> {
    auto toJson(HwAcc::Type type) const -> QJsonValue
    { return HwAcc::backendName(type); }
    auto fromJson(HwAcc::Type &type, const QJsonValue &json) const -> bool
    {
        const auto t = HwAcc::backend(json.toString());
        if (t != HwAcc::None)
            type = t;
        return t != HwAcc::None;
    }
    SCA qt_type = QJsonValue::String;
};

#define PREF_FILE_PATH QString(_WritablePath(Location::Config) % "/pref.json"_a)

auto translator_default_encoding() -> QString;

template<class T>
static QStringList toStringList(const QList<T> &list) {
    QStringList ret;
    ret.reserve(list.size());
    for (int i=0; i<list.size(); ++i)
        ret.push_back(list[i].toString());
    return ret;
}

template<class T>
static QList<T> fromStringList(const QStringList &list) {
    QList<T> ret;
    ret.reserve(list.size());
    for (int i=0; i<list.size(); ++i)
        ret.push_back(T::fromString(list[i]));
    return ret;
}

auto Pref::defaultShortcuts() -> Shortcuts
{
    Shortcuts keys;

    keys[u"open/file"_q] << Qt::CTRL + Qt::Key_F;
    keys[u"open/folder"_q] << Qt::CTRL + Qt::Key_G;

    keys[u"play/pause"_q] << Qt::Key_Space;
    keys[u"play/prev"_q] << Qt::CTRL + Qt::Key_Left;
    keys[u"play/next"_q] << Qt::CTRL + Qt::Key_Right;
    keys[u"play/speed/reset"_q] << Qt::Key_Backspace;
    keys[u"play/speed/increase"_q] << Qt::Key_Plus << Qt::Key_Equal;
    keys[u"play/speed/decrease"_q] << Qt::Key_Minus;
    keys[u"play/repeat/range"_q] << Qt::Key_R;
    keys[u"play/repeat/subtitle"_q] << Qt::Key_E;
    keys[u"play/repeat/quit"_q] << Qt::Key_Escape;
    keys[u"play/seek/forward1"_q] << Qt::Key_Right;
    keys[u"play/seek/forward2"_q] << Qt::Key_PageDown;
    keys[u"play/seek/forward3"_q] << Qt::Key_End;
    keys[u"play/seek/backward1"_q] << Qt::Key_Left;
    keys[u"play/seek/backward2"_q] << Qt::Key_PageUp;
    keys[u"play/seek/backward3"_q] << Qt::Key_Home;
    keys[u"play/seek/prev-frame"_q] << Qt::ALT + Qt::Key_Left;
    keys[u"play/seek/next-frame"_q] << Qt::ALT + Qt::Key_Right;
    keys[u"play/seek/prev-subtitle"_q] << Qt::Key_Comma;
    keys[u"play/seek/current-subtitle"_q] << Qt::Key_Period;
    keys[u"play/seek/next-subtitle"_q] << Qt::Key_Slash;

    keys[u"subtitle/track/open"_q] << Qt::SHIFT + Qt::Key_F;
    keys[u"subtitle/track/reload"_q] << Qt::SHIFT + Qt::Key_R;
    keys[u"subtitle/track/next"_q] << Qt::SHIFT + Qt::Key_N;
    keys[u"subtitle/track/all"_q] << Qt::SHIFT + Qt::Key_B;
    keys[u"subtitle/track/hide"_q] << Qt::SHIFT + Qt::Key_H;
    keys[u"subtitle/position/increase"_q] << Qt::Key_W;
    keys[u"subtitle/position/decrease"_q] << Qt::Key_S;
    keys[u"subtitle/sync/increase"_q] << Qt::Key_D;
    keys[u"subtitle/sync/reset"_q] << Qt::Key_Q;
    keys[u"subtitle/sync/decrease"_q] << Qt::Key_A;

    keys[u"video/snapshot/quick"_q] << Qt::CTRL + Qt::Key_S;
    keys[u"video/snapshot/tool"_q] << Qt::CTRL + Qt::SHIFT + Qt::Key_S;
    keys[u"video/move/reset"_q] << Qt::SHIFT + Qt::Key_X;
    keys[u"video/move/up"_q] << Qt::SHIFT + Qt::Key_W;
    keys[u"video/move/down"_q] << Qt::SHIFT + Qt::Key_S;
    keys[u"video/move/left"_q] << Qt::SHIFT + Qt::Key_A;
    keys[u"video/move/right"_q] << Qt::SHIFT + Qt::Key_D;
    keys[u"video/deinterlacing/toggle"_q] << Qt::CTRL + Qt::Key_D;
    keys[u"video/color/reset"_q] << Qt::Key_O;
    keys[u"video/color/brightness+"_q] << Qt::Key_T;
    keys[u"video/color/brightness-"_q] << Qt::Key_G;
    keys[u"video/color/contrast+"_q] << Qt::Key_Y;
    keys[u"video/color/contrast-"_q] << Qt::Key_H;
    keys[u"video/color/saturation+"_q] << Qt::Key_U;
    keys[u"video/color/saturation-"_q] << Qt::Key_J;
    keys[u"video/color/hue+"_q] << Qt::Key_I;
    keys[u"video/color/hue-"_q] << Qt::Key_K;
    keys[u"video/interpolator/next"_q] << Qt::CTRL+ Qt::Key_I;
    keys[u"video/dithering/next"_q] << Qt::CTRL+ Qt::Key_T;

    keys[u"audio/track/next"_q] << Qt::CTRL + Qt::Key_A;
    keys[u"audio/volume/increase"_q] << Qt::Key_Up;
    keys[u"audio/volume/decrease"_q] << Qt::Key_Down;
    keys[u"audio/volume/mute"_q] << Qt::Key_M;
    keys[u"audio/normalizer"_q] << Qt::Key_N;
    keys[u"audio/tempo-scaler"_q] << Qt::Key_Z;
    keys[u"audio/amp/increase"_q] << Qt::CTRL + Qt::Key_Up;
    keys[u"audio/amp/decrease"_q] << Qt::CTRL + Qt::Key_Down;
    keys[u"audio/channel/next"_q] << Qt::ALT + Qt::Key_C;
    keys[u"audio/sync/reset"_q] << Qt::Key_Backslash;
    keys[u"audio/sync/increase"_q] << Qt::Key_BracketRight;
    keys[u"audio/sync/decrease"_q] << Qt::Key_BracketLeft;

    keys[u"tool/playlist/toggle"_q] << Qt::Key_L;
    keys[u"tool/history"_q] << Qt::Key_C;
    keys[u"tool/subtitle"_q] << Qt::SHIFT + Qt::Key_V;
    keys[u"tool/find-subtitle"_q] << Qt::SHIFT + Qt::CTRL + Qt::Key_F;
    keys[u"tool/pref"_q] << Qt::Key_P;
    keys[u"tool/reload-skin"_q] << Qt::Key_R + Qt::CTRL;
    keys[u"tool/playinfo"_q] << Qt::Key_Tab;

    keys[u"window/proper"_q] << Qt::Key_QuoteLeft;
    keys[u"window/100%"_q] << Qt::Key_1;
    keys[u"window/200%"_q] << Qt::Key_2;
    keys[u"window/300%"_q] << Qt::Key_3;
    keys[u"window/400%"_q] << Qt::Key_4;
    keys[u"window/full"_q] << Qt::Key_Enter << Qt::Key_Return << Qt::Key_F;
    keys[u"window/close"_q] << Qt::CTRL + Qt::Key_W;

#ifndef Q_OS_MAC
    keys[u"exit"_q] << Qt::CTRL + Qt::Key_Q;
#endif
    return keys;
}

auto Pref::preset(KeyMapPreset id) -> Shortcuts
{
    Shortcuts keys;
    if (id == KeyMapPreset::Movist) {
        keys[u"open/file"_q] << Qt::CTRL + Qt::Key_O;
        keys[u"window/close"_q] << Qt::CTRL + Qt::Key_W;
        keys[u"tool/playlist/save"_q] << Qt::CTRL + Qt::Key_S << Qt::CTRL + Qt::SHIFT + Qt::Key_S;
        keys[u"tool/playlist/append-file"_q] << Qt::CTRL + Qt::ALT + Qt::Key_L;
        keys[u"play/prev"_q] << Qt::ALT + Qt::CTRL + Qt::Key_Left;
        keys[u"play/next"_q] << Qt::ALT + Qt::CTRL + Qt::Key_Right;
        keys[u"play/pause"_q] << Qt::Key_Space;
        keys[u"play/seek/backword1"_q] << Qt::Key_Left;
        keys[u"play/seek/forward1"_q] << Qt::Key_Right;
        keys[u"play/repeat/quit"_q] << Qt::CTRL + Qt::Key_Backslash;
        keys[u"play/seek/range"_q] << Qt::CTRL + Qt::Key_BracketLeft << Qt::CTRL + Qt::Key_BracketRight;
        keys[u"play/speed/reset"_q] << Qt::SHIFT + Qt::CTRL + Qt::Key_Backslash;
        keys[u"play/repeat/faster"_q] << Qt::SHIFT + Qt::CTRL + Qt::Key_Right;
        keys[u"play/speed/slower"_q] << Qt::SHIFT + Qt::CTRL + Qt::Key_Left;
        keys[u"window/full"_q] << Qt::META + Qt::CTRL + Qt::Key_F;
        keys[u"window/100%"_q] << Qt::CTRL + Qt::Key_1;
        keys[u"window/200%"_q] << Qt::CTRL + Qt::Key_2;
        keys[u"audio/track/next"_q] << Qt::META + Qt::ALT + Qt::CTRL + Qt::Key_S;
        keys[u"audio/sync-reset"_q] << Qt::META + Qt::ALT + Qt::CTRL + Qt::Key_Backslash;
        keys[u"audio/sync-add"_q] << Qt::META + Qt::ALT + Qt::CTRL + Qt::Key_Right;
        keys[u"audio/sync-sub"_q] << Qt::META + Qt::ALT + Qt::CTRL + Qt::Key_Left;
        keys[u"audio/volume/increase"_q] << Qt::ALT + Qt::Key_Up << Qt::Key_Up;
        keys[u"audio/volume/decrease"_q] << Qt::ALT + Qt::Key_Down << Qt::Key_Down;
        keys[u"audio/volume/mute"_q] << Qt::ALT + Qt::CTRL + Qt::Key_Down;
        keys[u"subtitle/track/next"_q] << Qt::META + Qt::CTRL + Qt::Key_S;
        keys[u"subtitle/track/hide"_q] << Qt::META + Qt::CTRL + Qt::Key_V;
        keys[u"subtitle/sync-reset"_q] << Qt::META + Qt::SHIFT + Qt::Key_Equal;
        keys[u"subtitle/sync-add"_q] << Qt::META + Qt::SHIFT + Qt::Key_Right;
        keys[u"subtitle/sync-sub"_q] << Qt::META + Qt::SHIFT + Qt::Key_Left;
        keys[u"window/sot-always"_q] << Qt::CTRL + Qt::Key_T;
        keys[u"window/sot-playing"_q] << Qt::CTRL + Qt::ALT + Qt::Key_T;
        keys[u"window/minimize"_q] << Qt::ALT + Qt::CTRL + Qt::Key_M;
        keys[u"tool/playlist/toggle"_q] << Qt::CTRL + Qt::Key_L;
        keys[u"tool/playinfo"_q] << Qt::CTRL + Qt::Key_P;
    } else
        keys = defaultShortcuts();
    return keys;
}

#define JSON_CLASS Pref
static const auto jio = JIO(
    JE(fit_to_video),
    JE(remember_stopped),
    JE(ask_record_found),
    JE(pause_minimized),
    JE(pause_video_only),
    JE(hide_cursor),
    JE(hide_cursor_fs_only),
    JE(hide_cursor_delay),
    JE(enable_system_tray),
    JE(hide_rather_close),
    JE(restore_properties),
    JE(invert_wheel),
    JE(disable_screensaver),
    JE(sub_enc),
    JE(sub_priority),
    JE(sub_enc_autodetection),
    JE(sub_enc_accuracy),
    JE(ms_per_char),
    JE(seek_step1),
    JE(seek_step2),
    JE(seek_step3),
    JE(speed_step),
    JE(volume_step),
    JE(amp_step),
    JE(sub_pos_step),
    JE(volume_step),
    JE(sub_sync_step),
    JE(brightness_step),
    JE(saturation_step),
    JE(contrast_step),
    JE(hue_step),
    JE(sub_ext),
    JE(blur_kern_c),
    JE(blur_kern_n),
    JE(blur_kern_d),
    JE(sharpen_kern_c),
    JE(sharpen_kern_n),
    JE(sharpen_kern_d),
    JE(remap_luma_min),
    JE(remap_luma_max),
    JE(channel_manipulation),
    JE(enable_generate_playist),
    JE(sub_enable_autoload),
    JE(sub_enable_autoselect),
    JE(generate_playlist),
    JE(sub_autoload),
    JE(sub_autoselect),
    JE(enable_hwaccel),
    JE(hwaccel_backend),
    JE(skin_name),
    JE(hwaccel_codecs),
    JE(hwdeints),
    JE(normalizer_silence),
    JE(normalizer_target),
    JE(normalizer_min),
    JE(normalizer_max),
    JE(lion_style_fullscreen),
    JE(show_logo),
    JE(bg_color),
    JE(deint_hwdec),
    JE(deint_swdec),
    JE(audio_driver),
    JE(clipping_method),
    JE(cache_local),
    JE(cache_disc),
    JE(cache_network),
    JE(cache_min_playback),
    JE(cache_min_seeking),
    JE(network_folders),
    JE(use_mpris2),
    JE(show_osd_on_action),
    JE(show_osd_on_resized),
    JE(show_osd_timeline),
    JE(use_heartbeat),
    JE(heartbeat_command),
    JE(heartbeat_interval),
    JE(open_media_from_file_manager),
    JE(open_media_by_drag_and_drop),
    JE(osd_theme),
    JE(sub_style),
    JE(double_click_map),
    JE(middle_click_map),
    JE(scroll_up_map),
    JE(scroll_down_map),
    JE(shortcuts),
    JE(quick_snapshot_folder),
    JE(save_quick_snapshot_current)
);

#define PREF_GROUP u"preference"_q

#define DO(FUNC1, FUNC2) { \
    FUNC1(fit_to_video); \
    FUNC1(remember_stopped); \
    FUNC1(ask_record_found); \
    FUNC1(pause_minimized); \
    FUNC1(pause_video_only); \
    FUNC1(hide_cursor); \
    FUNC1(hide_cursor_fs_only); \
    FUNC1(hide_cursor_delay); \
    FUNC1(enable_system_tray); \
    FUNC1(hide_rather_close); \
    FUNC1(restore_properties); \
    FUNC1(invert_wheel); \
    FUNC1(disable_screensaver); \
    FUNC1(sub_enc); \
    FUNC1(sub_priority); \
    FUNC1(sub_enc_autodetection); \
    FUNC1(sub_enc_accuracy); \
    FUNC1(ms_per_char); \
    FUNC1(seek_step1); \
    FUNC1(seek_step2); \
    FUNC1(seek_step3); \
    FUNC1(speed_step); \
    FUNC1(volume_step); \
    FUNC1(amp_step); \
    FUNC1(sub_pos_step); \
    FUNC1(volume_step); \
    FUNC1(sub_sync_step); \
    FUNC1(brightness_step); \
    FUNC1(saturation_step); \
    FUNC1(contrast_step); \
    FUNC1(hue_step); \
    FUNC1(sub_ext); \
    FUNC1(blur_kern_c); \
    FUNC1(blur_kern_n); \
    FUNC1(blur_kern_d); \
    FUNC1(sharpen_kern_c); \
    FUNC1(sharpen_kern_n); \
    FUNC1(sharpen_kern_d); \
    FUNC1(remap_luma_min); \
    FUNC1(remap_luma_max); \
    FUNC1(channel_manipulation); \
    FUNC1(enable_generate_playist); \
    FUNC1(sub_enable_autoload); \
    FUNC1(sub_enable_autoselect); \
    FUNC1(generate_playlist); \
    FUNC1(sub_autoload); \
    FUNC1(sub_autoselect); \
    FUNC1(enable_hwaccel); \
    FUNC1(skin_name); \
    FUNC1(hwaccel_codecs); \
    FUNC1(hwdeints); \
    FUNC1(normalizer_silence); \
    FUNC1(normalizer_target); \
    FUNC1(normalizer_min); \
    FUNC1(normalizer_max); \
    FUNC1(lion_style_fullscreen); \
    FUNC1(show_logo); \
    FUNC1(bg_color); \
    FUNC1(deint_hwdec); \
    FUNC1(deint_swdec); \
    FUNC1(audio_driver); \
    FUNC1(clipping_method); \
    FUNC1(cache_local); \
    FUNC1(cache_disc); \
    FUNC1(cache_network); \
    FUNC1(cache_min_playback); \
    FUNC1(cache_min_seeking); \
    FUNC1(network_folders); \
    FUNC1(use_mpris2); \
    FUNC1(show_osd_on_action); \
    FUNC1(show_osd_on_resized); \
    FUNC1(show_osd_timeline); \
    FUNC1(use_heartbeat); \
    FUNC1(heartbeat_command); \
    FUNC1(heartbeat_interval); \
    FUNC1(open_media_from_file_manager); \
    FUNC1(open_media_by_drag_and_drop); \
    FUNC1(osd_theme); \
    FUNC2(sub_style); \
    FUNC2(double_click_map); \
    FUNC2(middle_click_map); \
    FUNC2(scroll_up_map); \
    FUNC2(scroll_down_map); \
}

auto Pref::save() const -> void
{
    JsonStorage storage(PREF_FILE_PATH);
    storage.write(jio.toJson(*this));
}

auto Pref::loadFromRecord() -> void
{
    Record r(PREF_GROUP);
    QList<QByteArray> restore_properties;
#define READ1(a) r.read(a, #a)
#define READ2(a) a.load(r, u###a##_q)
    DO(READ1, READ2);

    QString backend;
    r.read(backend, "hwaccel_backend");
    hwaccel_backend = HwAcc::backend(backend);

    this->restore_properties.clear();
    this->restore_properties.reserve(restore_properties.size());
    for (auto &name : _C(restore_properties)) {
        auto &mo = MrlState::staticMetaObject;
        const int idx = mo.indexOfProperty(name.constData());
        if (idx != -1)
            this->restore_properties.append(mo.property(idx));
    }

    const auto size = r.beginReadArray(u"shortcuts"_q);
    if (size > 0) {
        shortcuts.clear();
        for (int i=0; i<size; ++i) {
            r.setArrayIndex(i);
            const auto id = r.value(u"id"_q).toString();
            if (!id.isEmpty()) {
                const auto keys = fromStringList<QKeySequence>(r.value(u"keys"_q).toStringList());
                if (!keys.isEmpty())
                    shortcuts[id] = keys;
            }
        }
    }
    r.endArray();
}

auto Pref::load() -> void
{
    JsonStorage storage(PREF_FILE_PATH);
    auto json = storage.read();
    if (storage.hasError()) {
        if (storage.error() != JsonStorage::NoFile)
            return;
        storage = JsonStorage(_WritablePath(Location::Data) % "/pref.json"_a);
        json = storage.read();
        if (storage.hasError()) {
            if (storage.error() == JsonStorage::NoFile)
                loadFromRecord();
            return;
        }
    }
    if (!jio.fromJson(*this, json))
        _Error("Error: Cannot convert JSON object to preferences");
}

#ifdef Q_OS_LINUX
#include "video/hwacc_vaapi.hpp"
#endif

auto Pref::defaultHwAccDeints() -> QVector<DeintMethod>
{
    QVector<DeintMethod> deints;
    for (auto deint : HwAcc::fullDeintList()) {
        if (HwAcc::supports(deint))
            deints << deint;
    }
    return deints;
}

auto Pref::defaultHwAccCodecs() -> QVector<int>
{
    QVector<int> codecs;
    for (auto codec : HwAcc::fullCodecList())
        codecs.push_back(codec);
    return codecs;
}

#undef PREF_GROUP

auto Pref::defaultSubtitleEncoding() -> QString
{
    return translator_default_encoding();
}

auto Pref::defaultSkinName() -> QString
{
    return u"GaN"_q;
}

auto Pref::defaultRestoreProperties() -> QVector<QMetaProperty>
{
    QVector<QMetaProperty> list;
    auto &mo = MrlState::staticMetaObject;
    const int count = mo.propertyCount();
    for (int i=mo.propertyOffset(); i<count; ++i) {
        const auto property = mo.property(i);
        if (property.revision())
            list.append(property);
    }
    return list;
}

auto Pref::defaultSubtitleEncodingDetectionAccuracy() -> int
{
    const QString value = tr("70",
        "This is default value for accuracy to enfoce auto-detected subtitle encoding in preferences. "
        "Higher value means that auto-detection will be applied only if the result is more reliable.");
    bool ok = false;
    const int accuracy = value.toInt(&ok);
    return ok ? qBound(0, accuracy, 100) : 70;
}

auto Pref::defaultOsdTheme() -> OsdTheme
{
    OsdTheme theme;
    theme.underline = theme.strikeout = theme.italic = theme.bold = false;
    theme.font = qApp->font().family();
    theme.scale = 0.03;
    theme.style = TextThemeStyle::Outline;
    theme.color = Qt::white;
    theme.styleColor = Qt::black;
    return theme;
}

auto Pref::defaultDoubleClick() -> KeyModifierActionMap
{
    KeyModifierActionMap map;
    map[KeyModifier::None] = { true, u"window/full"_q };
    return map;
}

auto Pref::defaultMiddleClick() -> KeyModifierActionMap
{
    KeyModifierActionMap map;
    map[KeyModifier::None] = { true, u"play/pause"_q };
    return map;
}

auto Pref::defaultWheelUpAction() -> KeyModifierActionMap
{
    KeyModifierActionMap map;
    map[KeyModifier::None] = { true, u"audio/volume/increase"_q };
    map[KeyModifier::Ctrl] = { true, u"audio/amp/increase"_q };
    return map;
}

auto Pref::defaultWheelDownAction() -> KeyModifierActionMap
{
    KeyModifierActionMap map;
    map[KeyModifier::None] = { true, u"audio/volume/decrease"_q };
    map[KeyModifier::Ctrl] = { true, u"audio/amp/decrease"_q };
    return map;
}
