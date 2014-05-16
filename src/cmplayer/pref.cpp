#include "pref.hpp"
#include "translator.hpp"
#include "video/hwacc.hpp"
#include "info.hpp"

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

auto Pref::defaultShortcuts() -> QHash<QString, QList<QKeySequence> >
{
    QHash<QString, QList<QKeySequence> > keys;

    keys[_L("open/file")] << Qt::CTRL + Qt::Key_F;
    keys[_L("open/folder")] << Qt::CTRL + Qt::Key_G;

    keys[_L("play/pause")] << Qt::Key_Space;
    keys[_L("play/prev")] << Qt::CTRL + Qt::Key_Left;
    keys[_L("play/next")] << Qt::CTRL + Qt::Key_Right;
    keys[_L("play/speed/reset")] << Qt::Key_Backspace;
    keys[_L("play/speed/increase")] << Qt::Key_Plus << Qt::Key_Equal;
    keys[_L("play/speed/decrease")] << Qt::Key_Minus;
    keys[_L("play/repeat/range")] << Qt::Key_R;
    keys[_L("play/repeat/subtitle")] << Qt::Key_E;
    keys[_L("play/repeat/quit")] << Qt::Key_Escape;
    keys[_L("play/seek/forward1")] << Qt::Key_Right;
    keys[_L("play/seek/forward2")] << Qt::Key_PageDown;
    keys[_L("play/seek/forward3")] << Qt::Key_End;
    keys[_L("play/seek/backward1")] << Qt::Key_Left;
    keys[_L("play/seek/backward2")] << Qt::Key_PageUp;
    keys[_L("play/seek/backward3")] << Qt::Key_Home;
    keys[_L("play/seek/prev-frame")] << Qt::ALT + Qt::Key_Left;
    keys[_L("play/seek/next-frame")] << Qt::ALT + Qt::Key_Right;
    keys[_L("play/seek/prev-subtitle")] << Qt::Key_Comma;
    keys[_L("play/seek/current-subtitle")] << Qt::Key_Period;
    keys[_L("play/seek/next-subtitle")] << Qt::Key_Slash;

    keys[_L("subtitle/track/open")] << Qt::SHIFT + Qt::Key_F;
    keys[_L("subtitle/track/reload")] << Qt::SHIFT + Qt::Key_R;
    keys[_L("subtitle/track/next")] << Qt::SHIFT + Qt::Key_N;
    keys[_L("subtitle/track/all")] << Qt::SHIFT + Qt::Key_B;
    keys[_L("subtitle/track/hide")] << Qt::SHIFT + Qt::Key_H;
    keys[_L("subtitle/position/increase")] << Qt::Key_W;
    keys[_L("subtitle/position/decrease")] << Qt::Key_S;
    keys[_L("subtitle/sync/increase")] << Qt::Key_D;
    keys[_L("subtitle/sync/reset")] << Qt::Key_Q;
    keys[_L("subtitle/sync/decrease")] << Qt::Key_A;

    keys[_L("video/snapshot")] << Qt::CTRL + Qt::Key_S;
    keys[_L("video/move/reset")] << Qt::SHIFT + Qt::Key_X;
    keys[_L("video/move/up")] << Qt::SHIFT + Qt::Key_W;
    keys[_L("video/move/down")] << Qt::SHIFT + Qt::Key_S;
    keys[_L("video/move/left")] << Qt::SHIFT + Qt::Key_A;
    keys[_L("video/move/right")] << Qt::SHIFT + Qt::Key_D;
    keys[_L("video/deinterlacing/toggle")] << Qt::CTRL + Qt::Key_D;
    keys[_L("video/color/reset")] << Qt::Key_O;
    keys[_L("video/color/brightness+")] << Qt::Key_T;
    keys[_L("video/color/brightness-")] << Qt::Key_G;
    keys[_L("video/color/contrast+")] << Qt::Key_Y;
    keys[_L("video/color/contrast-")] << Qt::Key_H;
    keys[_L("video/color/saturation+")] << Qt::Key_U;
    keys[_L("video/color/saturation-")] << Qt::Key_J;
    keys[_L("video/color/hue+")] << Qt::Key_I;
    keys[_L("video/color/hue-")] << Qt::Key_K;
    keys[_L("video/interpolator/next")] << Qt::CTRL+ Qt::Key_I;
    keys[_L("video/dithering/next")] << Qt::CTRL+ Qt::Key_T;

    keys[_L("audio/track/next")] << Qt::CTRL + Qt::Key_A;
    keys[_L("audio/volume/increase")] << Qt::Key_Up;
    keys[_L("audio/volume/decrease")] << Qt::Key_Down;
    keys[_L("audio/volume/mute")] << Qt::Key_M;
    keys[_L("audio/normalizer")] << Qt::Key_N;
    keys[_L("audio/tempo-scaler")] << Qt::Key_Z;
    keys[_L("audio/amp/increase")] << Qt::CTRL + Qt::Key_Up;
    keys[_L("audio/amp/decrease")] << Qt::CTRL + Qt::Key_Down;
    keys[_L("audio/channel/next")] << Qt::ALT + Qt::Key_C;
    keys[_L("audio/sync/reset")] << Qt::Key_Backslash;
    keys[_L("audio/sync/increase")] << Qt::Key_BracketRight;
    keys[_L("audio/sync/decrease")] << Qt::Key_BracketLeft;

    keys[_L("tool/playlist/toggle")] << Qt::Key_L;
    keys[_L("tool/history")] << Qt::Key_C;
    keys[_L("tool/subtitle")] << Qt::SHIFT + Qt::Key_V;
    keys[_L("tool/find-subtitle")] << Qt::SHIFT + Qt::CTRL + Qt::Key_F;
    keys[_L("tool/pref")] << Qt::Key_P;
    keys[_L("tool/reload-skin")] << Qt::Key_R + Qt::CTRL;
    keys[_L("tool/playinfo")] << Qt::Key_Tab;

    keys[_L("window/proper")] << Qt::Key_QuoteLeft;
    keys[_L("window/100%")] << Qt::Key_1;
    keys[_L("window/200%")] << Qt::Key_2;
    keys[_L("window/300%")] << Qt::Key_3;
    keys[_L("window/400%")] << Qt::Key_4;
    keys[_L("window/full")] << Qt::Key_Enter << Qt::Key_Return << Qt::Key_F;
    keys[_L("window/close")] << Qt::CTRL + Qt::Key_W;

#ifndef Q_OS_MAC
    keys[_L("exit")] << Qt::CTRL + Qt::Key_Q;
#endif
    return keys;
}

auto Pref::preset(KeyMapPreset id) -> Shortcuts
{
    Shortcuts keys;
    if (id == KeyMapPreset::Movist) {
        keys[_L("open/file")] << Qt::CTRL + Qt::Key_O;
        keys[_L("window/close")] << Qt::CTRL + Qt::Key_W;
        keys[_L("tool/playlist/save")] << Qt::CTRL + Qt::Key_S << Qt::CTRL + Qt::SHIFT + Qt::Key_S;
        keys[_L("tool/playlist/append-file")] << Qt::CTRL + Qt::ALT + Qt::Key_L;
        keys[_L("play/prev")] << Qt::ALT + Qt::CTRL + Qt::Key_Left;
        keys[_L("play/next")] << Qt::ALT + Qt::CTRL + Qt::Key_Right;
        keys[_L("play/pause")] << Qt::Key_Space;
        keys[_L("play/seek/backword1")] << Qt::Key_Left;
        keys[_L("play/seek/forward1")] << Qt::Key_Right;
        keys[_L("play/repeat/quit")] << Qt::CTRL + Qt::Key_Backslash;
        keys[_L("play/seek/range")] << Qt::CTRL + Qt::Key_BracketLeft << Qt::CTRL + Qt::Key_BracketRight;
        keys[_L("play/speed/reset")] << Qt::SHIFT + Qt::CTRL + Qt::Key_Backslash;
        keys[_L("play/repeat/faster")] << Qt::SHIFT + Qt::CTRL + Qt::Key_Right;
        keys[_L("play/speed/slower")] << Qt::SHIFT + Qt::CTRL + Qt::Key_Left;
        keys[_L("window/full")] << Qt::META + Qt::CTRL + Qt::Key_F;
        keys[_L("window/100%")] << Qt::CTRL + Qt::Key_1;
        keys[_L("window/200%")] << Qt::CTRL + Qt::Key_2;
        keys[_L("audio/track/next")] << Qt::META + Qt::ALT + Qt::CTRL + Qt::Key_S;
        keys[_L("audio/sync-reset")] << Qt::META + Qt::ALT + Qt::CTRL + Qt::Key_Backslash;
        keys[_L("audio/sync-add")] << Qt::META + Qt::ALT + Qt::CTRL + Qt::Key_Right;
        keys[_L("audio/sync-sub")] << Qt::META + Qt::ALT + Qt::CTRL + Qt::Key_Left;
        keys[_L("audio/volume/increase")] << Qt::ALT + Qt::Key_Up << Qt::Key_Up;
        keys[_L("audio/volume/decrease")] << Qt::ALT + Qt::Key_Down << Qt::Key_Down;
        keys[_L("audio/volume/mute")] << Qt::ALT + Qt::CTRL + Qt::Key_Down;
        keys[_L("subtitle/track/next")] << Qt::META + Qt::CTRL + Qt::Key_S;
        keys[_L("subtitle/track/hide")] << Qt::META + Qt::CTRL + Qt::Key_V;
        keys[_L("subtitle/sync-reset")] << Qt::META + Qt::SHIFT + Qt::Key_Equal;
        keys[_L("subtitle/sync-add")] << Qt::META + Qt::SHIFT + Qt::Key_Right;
        keys[_L("subtitle/sync-sub")] << Qt::META + Qt::SHIFT + Qt::Key_Left;
        keys[_L("window/sot-always")] << Qt::CTRL + Qt::Key_T;
        keys[_L("window/sot-playing")] << Qt::CTRL + Qt::ALT + Qt::Key_T;
        keys[_L("window/minimize")] << Qt::ALT + Qt::CTRL + Qt::Key_M;
        keys[_L("tool/playlist/toggle")] << Qt::CTRL + Qt::Key_L;
        keys[_L("tool/playinfo")] << Qt::CTRL + Qt::Key_P;
    } else
        keys = defaultShortcuts();
    return keys;
}

#define PREF_GROUP _L("preference")

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
    FUNC1(use_heartbeat); \
    FUNC1(heartbeat_command); \
    FUNC1(heartbeat_interval); \
    \
    FUNC2(open_media_from_file_manager); \
    FUNC2(open_media_by_drag_and_drop); \
    FUNC2(sub_style); \
    FUNC2(double_click_map); \
    FUNC2(middle_click_map); \
    FUNC2(scroll_up_map); \
    FUNC2(scroll_down_map); \
    FUNC2(osd_theme); }

auto Pref::save() const -> void
{
    Record r(PREF_GROUP);
    QList<QByteArray> restore_properties;
    restore_properties.reserve(this->restore_properties.size());
    for (auto &property : this->restore_properties)
        restore_properties.append(property.name());
#define WRITE1(a) r.write(a, #a)
#define WRITE2(a) a.save(r, #a)
    DO(WRITE1, WRITE2);

    r.write(HwAcc::backendName(hwaccel_backend), "hwaccel_backend");

    r.beginWriteArray("shortcuts", shortcuts.size());
    auto it = shortcuts.cbegin();
    for (int i=0; it != shortcuts.cend(); ++it, ++i) {
        r.setArrayIndex(i);
        r.setValue("id", it.key());
        r.setValue("keys", toStringList(it.value()));
    }
    r.endArray();

    r.setValue("version", Info::versionNumber());
}

auto Pref::load() -> void
{
    Record r(PREF_GROUP);
    QList<QByteArray> restore_properties;
#define READ1(a) r.read(a, #a)
#define READ2(a) a.load(r, #a)
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

    const auto size = r.beginReadArray("shortcuts");
    if (size > 0) {
        shortcuts.clear();
        for (int i=0; i<size; ++i) {
            r.setArrayIndex(i);
            const auto id = r.value("id").toString();
            if (!id.isEmpty()) {
                const auto keys = fromStringList<QKeySequence>(r.value("keys").toStringList());
                if (!keys.isEmpty())
                    shortcuts[id] = keys;
            }
        }
    }
    r.endArray();
}

#ifdef Q_OS_LINUX
#include "video/hwacc_vaapi.hpp"
#endif

auto Pref::defaultHwAccDeints() -> QList<DeintMethod>
{
    QList<DeintMethod> deints;
    for (auto deint : HwAcc::fullDeintList()) {
        if (HwAcc::supports(deint))
            deints << deint;
    }
    return deints;
}

auto Pref::defaultHwAccCodecs() -> QList<int>
{
    QList<int> codecs;
    for (auto codec : HwAcc::fullCodecList())
        codecs.push_back(codec);
    return codecs;
}

#undef PREF_GROUP

auto Pref::defaultSubtitleEncoding() -> QString
{
    return Translator::defaultEncoding();
}

auto Pref::defaultSkinName() -> QString
{
    return "GaN";
}

auto Pref::defaultRestoreProperties() -> QList<QMetaProperty>
{
    QList<QMetaProperty> list;
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
    map[KeyModifier::None] = { true, "window/full" };
    return map;
}

auto Pref::defaultMiddleClick() -> KeyModifierActionMap
{
    KeyModifierActionMap map;
    map[KeyModifier::None] = { true, "play/pause" };
    return map;
}

auto Pref::defaultWheelUpAction() -> KeyModifierActionMap
{
    KeyModifierActionMap map;
    map[KeyModifier::None] = { true, "audio/volume/increase" };
    map[KeyModifier::Ctrl] = { true, "audio/amp/increase" };
    return map;
}

auto Pref::defaultWheelDownAction() -> KeyModifierActionMap
{
    KeyModifierActionMap map;
    map[KeyModifier::None] = { true, "audio/volume/decrease" };
    map[KeyModifier::Ctrl] = { true, "audio/amp/decrease" };
    return map;
}
