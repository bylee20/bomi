#include "pref.hpp"
#include "translator.hpp"
#include "video/hwacc.hpp"
#include "info.hpp"

template<typename T>
static QStringList toStringList(const QList<T> &list) {
    QStringList ret;
    ret.reserve(list.size());
    for (int i=0; i<list.size(); ++i)
        ret.push_back(list[i].toString());
    return ret;
}

template<typename T>
static QList<T> fromStringList(const QStringList &list) {
    QList<T> ret;
    ret.reserve(list.size());
    for (int i=0; i<list.size(); ++i)
        ret.push_back(T::fromString(list[i]));
    return ret;
}

QHash<QString, QList<QKeySequence> > Pref::defaultShortcuts() {
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

Shortcuts Pref::preset(KeyMapPreset id) {
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

void Pref::save() const {
    Record r(PREF_GROUP);
#define WRITE(a) r.write(a, #a)
    WRITE(fit_to_video);
    WRITE(remember_stopped);
    WRITE(ask_record_found);
    WRITE(pause_minimized);
    WRITE(pause_video_only);
    WRITE(hide_cursor);
    WRITE(hide_cursor_fs_only);
    WRITE(hide_cursor_delay);
    WRITE(enable_system_tray);
    WRITE(hide_rather_close);

    QList<QByteArray> restore_properties;
    restore_properties.reserve(this->restore_properties.size());
    for (auto &property : this->restore_properties)
        restore_properties.append(property.name());
    WRITE(restore_properties);

    WRITE(invert_wheel);
    WRITE(disable_screensaver);
    WRITE(sub_enc);
    WRITE(sub_priority);
    WRITE(sub_enc_autodetection);
    WRITE(sub_enc_accuracy);
    WRITE(ms_per_char);
    WRITE(seek_step1);
    WRITE(seek_step2);
    WRITE(seek_step3);
    WRITE(speed_step);
    WRITE(volume_step);
    WRITE(amp_step);
    WRITE(sub_pos_step);
    WRITE(volume_step);
    WRITE(sub_sync_step);
    WRITE(brightness_step);
    WRITE(saturation_step);
    WRITE(contrast_step);
    WRITE(hue_step);
    WRITE(sub_ext);
    WRITE(blur_kern_c);
    WRITE(blur_kern_n);
    WRITE(blur_kern_d);
    WRITE(sharpen_kern_c);
    WRITE(sharpen_kern_n);
    WRITE(sharpen_kern_d);
    WRITE(remap_luma_min);
    WRITE(remap_luma_max);
    WRITE(channel_manipulation);

    WRITE(enable_generate_playist);
    WRITE(sub_enable_autoload);
    WRITE(sub_enable_autoselect);
    WRITE(generate_playlist);
    WRITE(sub_autoload);
    WRITE(sub_autoselect);

    WRITE(enable_hwaccel);
    WRITE(skin_name);
    WRITE(hwaccel_codecs);
    WRITE(hwdeints);
    r.write(HwAcc::backendName(hwaccel_backend), "hwaccel_backend");
    WRITE(normalizer_silence);
    WRITE(normalizer_target);
    WRITE(normalizer_min);
    WRITE(normalizer_max);

    WRITE(lion_style_fullscreen);

    WRITE(show_logo);
    WRITE(bg_color);

    WRITE(deint_hwdec);
    WRITE(deint_swdec);

    WRITE(audio_driver);
    WRITE(clipping_method);

    WRITE(cache_local);
    WRITE(cache_disc);
    WRITE(cache_network);
    WRITE(cache_min_playback);
    WRITE(cache_min_seeking);
    WRITE(network_folders);
    WRITE(use_mpris2);
    WRITE(show_osd_on_action);
    WRITE(show_osd_on_resized);
    WRITE(use_heartbeat);
    WRITE(heartbeat_command);
    WRITE(heartbeat_interval);
#undef WRITE

#define WRITE2(a) a.save(r, #a);
    WRITE2(open_media_from_file_manager);
    WRITE2(open_media_by_drag_and_drop);
    WRITE2(sub_style);
    WRITE2(double_click_map);
    WRITE2(middle_click_map);
    WRITE2(scroll_up_map);
    WRITE2(scroll_down_map);
#undef WRITE2

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

void Pref::load() {
    Record r(PREF_GROUP);
#define READ(a) r.read(a, #a)
    READ(fit_to_video);
    READ(remember_stopped);
    READ(ask_record_found);
    READ(pause_minimized);
    READ(pause_video_only);
    READ(hide_cursor);
    READ(hide_cursor_fs_only);
    READ(hide_cursor_delay);
    READ(blur_kern_c);
    READ(blur_kern_n);
    READ(blur_kern_d);
    READ(sharpen_kern_c);
    READ(sharpen_kern_n);
    READ(sharpen_kern_d);
    READ(remap_luma_min);
    READ(remap_luma_max);
    READ(channel_manipulation);

    QList<QByteArray> restore_properties;
    READ(restore_properties);
    this->restore_properties.clear();
    this->restore_properties.reserve(restore_properties.size());
    for (auto &name : _C(restore_properties)) {
        auto &mo = MrlState::staticMetaObject;
        const int idx = mo.indexOfProperty(name.constData());
        if (idx != -1)
            this->restore_properties.append(mo.property(idx));
    }

    READ(invert_wheel);
    READ(enable_system_tray);
    READ(hide_rather_close);
    READ(disable_screensaver);
    READ(sub_enc);
    READ(sub_enc_autodetection);
    READ(sub_enc_accuracy);
    READ(ms_per_char);
    READ(sub_priority);
    READ(seek_step1);
    READ(seek_step2);
    READ(seek_step3);
    READ(speed_step);
    READ(volume_step);
    READ(amp_step);
    READ(sub_pos_step);
    READ(sub_sync_step);
    READ(brightness_step);
    READ(saturation_step);
    READ(contrast_step);
    READ(hue_step);
    READ(sub_ext);

    READ(skin_name);
    READ(enable_hwaccel);
    READ(hwaccel_codecs);
    READ(hwdeints);
    QString backend;
    r.read(backend, "hwaccel_backend");
    hwaccel_backend = HwAcc::backend(backend);

    READ(enable_generate_playist);
    READ(sub_enable_autoload);
    READ(sub_enable_autoselect);
    READ(generate_playlist);
    READ(sub_autoload);
    READ(sub_autoselect);

    READ(normalizer_silence);
    READ(normalizer_target);
    READ(normalizer_min);
    READ(normalizer_max);

    READ(lion_style_fullscreen);

    READ(show_logo);
    READ(bg_color);

    READ(deint_hwdec);
    READ(deint_swdec);

    READ(audio_driver);
    READ(clipping_method);

    READ(cache_local);
    READ(cache_disc);
    READ(cache_network);
    READ(cache_min_playback);
    READ(cache_min_seeking);
    READ(network_folders);
    READ(use_mpris2);
    READ(show_osd_on_action);
    READ(show_osd_on_resized);
    READ(use_heartbeat);
    READ(heartbeat_command);
    READ(heartbeat_interval);
#undef READ

#define READ2(a) a.load(r, #a)
    READ2(open_media_from_file_manager);
    READ2(open_media_by_drag_and_drop);
    READ2(sub_style);
    READ2(double_click_map);
    READ2(middle_click_map);
    READ2(scroll_up_map);
    READ2(scroll_down_map);
#undef READ2

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

QList<DeintMethod> Pref::defaultHwAccDeints() {
    QList<DeintMethod> deints;
    for (auto deint : HwAcc::fullDeintList()) {
        if (HwAcc::supports(deint))
            deints << deint;
    }
    return deints;
}

QList<int> Pref::defaultHwAccCodecs() {
    QList<int> codecs;
    for (auto codec : HwAcc::fullCodecList())
        codecs.push_back(codec);
    return codecs;
}

#undef PREF_GROUP

QString Pref::defaultSubtitleEncoding() {
    return Translator::defaultEncoding();
}

QString Pref::defaultSkinName() {
    return "GaN";
}

QList<QMetaProperty> Pref::defaultRestoreProperties() {
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

int Pref::defaultSubtitleEncodingDetectionAccuracy() {
    const QString value = tr("70",
        "This is default value for accuracy to enfoce auto-detected subtitle encoding in preferences. "
        "Higher value means that auto-detection will be applied only if the result is more reliable.");
    bool ok = false;
    const int accuracy = value.toInt(&ok);
    return ok ? qBound(0, accuracy, 100) : 70;
}
