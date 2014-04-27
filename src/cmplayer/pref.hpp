#ifndef PREF_HPP
#define PREF_HPP

#include "stdafx.hpp"
#include "global.hpp"
#include "enums.hpp"
#include "video/deintinfo.hpp"
#include "record.hpp"
#include "subtitle/subtitlestyle.hpp"
#include "mrlstate.hpp"
#include "video/hwacc.hpp"

class QLocale;

typedef QHash<QString, QList<QKeySequence>> Shortcuts;

struct MouseActionInfo {
    MouseActionInfo(): enabled(false) {}
    MouseActionInfo(bool e, const QString &id): enabled(e), id(id) {}
    bool enabled; QString id;
};

class Pref {
    Q_DECLARE_TR_FUNCTIONS(Pref)
public:
    enum ShortcutPreset {CMPlayer, Movist};
//    static const Pref &instance() {return get();}
    struct KeyModifierMap {
        typedef KeyModifier Modifier;
        typedef QMap<Modifier, MouseActionInfo> Map;
        typedef MouseActionInfo Info;
        KeyModifierMap() {
            const auto &list = EnumInfo<Modifier>::items();
            for (auto &item : list) m_map[item.value];
        }
        Info &operator[](Modifier m) {return m_map[m];}
        const Info operator[](Modifier m) const {return m_map[m];}
        const Info operator[](int id) const {return m_map[EnumInfo<Modifier>::from(id, Modifier::None)];}
        void save(Record &r, const QString &group) const {
            r.beginGroup(group);
            const auto &items = EnumInfo<Modifier>::items();
            for (auto &item : items) {
                const auto &info = m_map[item.value];
                r.beginGroup(item.name);
                r.write(info.enabled, "enabled");
                r.write(info.id, "id");
                r.endGroup();
            }
            r.endGroup();
        }
        void load(Record &r, const QString &group) {
            r.beginGroup(group);
            const auto &items = EnumInfo<Modifier>::items();
            for (auto &item : items) {
                auto &info = m_map[item.value];
                r.beginGroup(item.name);
                r.read(info.enabled, "enabled");
                r.read(info.id, "id");
                r.endGroup();
            }
            r.endGroup();
        }
    private:
        Map m_map;
    };

    struct OpenMedia {
        OpenMedia(bool sp, const PlaylistBehaviorWhenOpenMedia &pb)
        : start_playback(sp), playlist_behavior(pb) {}
        bool start_playback = true;
        PlaylistBehaviorWhenOpenMedia playlist_behavior = PlaylistBehaviorWhenOpenMedia::ClearAndGenerateNewPlaylist;
        void save(Record &r, const QString &group) const {
            r.beginGroup(group);
            r.write(start_playback, "start_playback");
            r.write(playlist_behavior, "playlist_behavior");
            r.endGroup();
        }
        void load(Record &r, const QString &group) {
            r.beginGroup(group);
            r.read(start_playback, "start_playback");
            r.read(playlist_behavior, "playlist_behavior");
            r.endGroup();
        }
    };

    OpenMedia open_media_from_file_manager = {true, PlaylistBehaviorWhenOpenMedia::ClearAndAppendToPlaylist};
    OpenMedia open_media_by_drag_and_drop = {true, PlaylistBehaviorWhenOpenMedia::AppendToPlaylist};

    bool fit_to_video = false, use_mpris2 = true;
    bool pause_minimized = true, pause_video_only = true, pause_to_play_next_image = true;
    bool remember_stopped = true, ask_record_found = true, remember_image = false;
    bool enable_generate_playist = true;
    QList<QMetaProperty> restore_properties = defaultRestoreProperties();
    GeneratePlaylist generate_playlist = GeneratePlaylist::Folder;
    bool hide_cursor = true, disable_screensaver = true, lion_style_fullscreen = false;
    bool hide_cursor_fs_only = false;
    int hide_cursor_delay = 3000, image_duration = 0;
    bool show_osd_on_action = true, show_osd_on_resized = true;
    bool show_logo = true; QColor bg_color = Qt::black;
    int blur_kern_c = 1, blur_kern_n = 2, blur_kern_d = 1;
    int sharpen_kern_c = 5, sharpen_kern_n = -1, sharpen_kern_d = 0;
    int remap_luma_min = 16, remap_luma_max = 235;
    ChannelLayoutMap channel_manipulation = ChannelLayoutMap::default_();

    bool sub_enable_autoload = true, sub_enable_autoselect = true, sub_enc_autodetection = true;
    SubtitleAutoload sub_autoload = SubtitleAutoload::Contain;
    SubtitleAutoselect sub_autoselect = SubtitleAutoselect::Matched;
    QString sub_enc = defaultSubtitleEncoding(), sub_ext;
    int sub_enc_accuracy = defaultSubtitleEncodingDetectionAccuracy(), ms_per_char = 500;
    SubtitleStyle sub_style;        QStringList sub_priority;

    bool enable_system_tray = true, hide_rather_close = true;
    KeyModifierMap double_click_map = defaultDoubleClick();
    KeyModifierMap middle_click_map = defaultMiddleClick();
    KeyModifierMap scroll_up_map = defaultWheelUpAction();
    KeyModifierMap scroll_down_map = defaultWheelDownAction();
    bool invert_wheel = false;
    int seek_step1 = 5000, seek_step2 = 30000, seek_step3 = 60000, speed_step = 10;
    int brightness_step = 1, saturation_step = 1, contrast_step = 1, hue_step = 1;
    int volume_step = 2, sub_sync_step = 500, amp_step = 10, sub_pos_step = 1, audio_sync_step = 200;
    bool enable_hwaccel = false;
#ifdef Q_OS_LINUX
    HwAcc::Type hwaccel_backend = HwAcc::VaApiGLX;
#endif
#ifdef Q_OS_MAC
    HwAcc::Type hwaccel_backend = HwAcc::Vda;
#endif
    QList<int> hwaccel_codecs = defaultHwAccCodecs();
    QList<DeintMethod> hwdeints = defaultHwAccDeints();
    DeintCaps deint_hwdec = DeintCaps::default_(DecoderDevice::GPU);
    DeintCaps deint_swdec = DeintCaps::default_(DecoderDevice::CPU);

    double normalizer_silence = 0.0001, normalizer_target = 0.07, normalizer_min = 0.1, normalizer_max = 10.0;
    double normalizer_length = 5.0;

    QString skin_name = defaultSkinName();

    Shortcuts shortcuts = defaultShortcuts();

    AudioDriver audio_driver = AudioDriver::Auto;
    ClippingMethod clipping_method = ClippingMethod::Auto;

    int cache_local = 0, cache_network = 2048, cache_disc = 0;
    int cache_min_playback = 20, cache_min_seeking = 50;
    QStringList network_folders;

    static Shortcuts preset(ShortcutPreset id);

    void save() const;
    void load();
private:
//    static Pref &get();
    static QList<QMetaProperty> defaultRestoreProperties();
    static QString defaultSkinName();
    static QString defaultSubtitleEncoding();
    static int defaultSubtitleEncodingDetectionAccuracy();
    static QList<int> defaultHwAccCodecs();

    static QList<DeintMethod> defaultHwAccDeints();
    static Shortcuts defaultShortcuts();
    static KeyModifierMap defaultDoubleClick() {
        KeyModifierMap map;
        map[KeyModifier::None] = { true, "window/full" };
        return map;
    }
    static KeyModifierMap defaultMiddleClick() {
        KeyModifierMap map;
        map[KeyModifier::None] = { true, "play/pause" };
        return map;
    }
    static KeyModifierMap defaultWheelUpAction() {
        KeyModifierMap map;
        map[KeyModifier::None] = { true, "audio/volume/increase" };
        map[KeyModifier::Ctrl] = { true, "audio/amp/increase" };
        return map;
    }
    static KeyModifierMap defaultWheelDownAction() {
        KeyModifierMap map;
        map[KeyModifier::None] = { true, "audio/volume/decrease" };
        map[KeyModifier::Ctrl] = { true, "audio/amp/decrease" };
        return map;
    }
};

#endif // PREF_HPP
