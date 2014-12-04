#ifndef PREF_HPP
#define PREF_HPP

#include "openmediainfo.hpp"
#include "misc/matchstring.hpp"
#include "video/hwacc.hpp"
#include "video/deintcaps.hpp"
#include "misc/keymodifieractionmap.hpp"
#include "misc/osdtheme.hpp"
#include "player/mrlstate.hpp"
#include "audio/channellayoutmap.hpp"
#include "audio/audionormalizeroption.hpp"
#include "enum/generateplaylist.hpp"
#include "enum/subtitleautoload.hpp"
#include "enum/subtitleautoselect.hpp"
#include "enum/audiodriver.hpp"
#include "enum/clippingmethod.hpp"
#include "enum/verticalalignment.hpp"
#include "enum/quicksnapshotsave.hpp"
#include "enum/mousebehavior.hpp"
#include "pref_helper.hpp"

using Shortcuts = QHash<QString, QList<QKeySequence>>;
enum class KeyMapPreset {CMPlayer, Movist};

class Pref : public QObject {
    Q_OBJECT
/***************************************************************/
#define P_(type, name, def, info_type, ...) \
public: \
    type name = def; \
private: \
    Q_PROPERTY(type name MEMBER name WRITE set_##name) \
    auto set_##name(const type &t) { name = t; }\
    Q_INVOKABLE void init_##name##_info() { static const info_type info{this, PrefFieldIO<type>::toJson, PrefFieldIO<type>::fromJson, __VA_ARGS__}; } \
public:
#define P0(type, var, def) P_(type, var, def, PrefFieldInfo, #var, PrefEditorProperty<type>::name)
#define P1(type, var, def, editorProp) P_(type, var, def, PrefFieldInfo, #var, editorProp) // same editor name, custom editor property
#define P2(type, var, def, editorName) P_(type, var, def, PrefFieldInfo, #var, editorName, PrefEditorProperty<type>::name) // custom editor name, default editor property
/***************************************************************/
public:
    P0(OpenMediaInfo, open_media_from_file_manager, {OpenMediaBehavior::NewPlaylist})
    P0(OpenMediaInfo, open_media_by_drag_and_drop, {OpenMediaBehavior::Append})
    P1(QString, quick_snapshot_format, u"png"_q, "currentText")
    P0(QString, quick_snapshot_folder, QDir::homePath())
    P0(int, quick_snapshot_quality, -1)
    P0(QuickSnapshotSave, quick_snapshot_save, QuickSnapshotSave::Fixed)

    P0(bool, fit_to_video, false)
    P0(bool, use_mpris2, true)
    P0(bool, pause_minimized, true)
    P0(bool, pause_video_only, true)
    P0(bool, remember_stopped, true)
    P0(bool, ask_record_found, true)
    P0(bool, remember_image, false)
    P0(bool, enable_generate_playlist, true)
    P0(QVector<QMetaProperty>, restore_properties, defaultRestoreProperties())
    P0(GeneratePlaylist, generate_playlist, GeneratePlaylist::Folder)
    P0(bool, hide_cursor, true)
    P0(bool, disable_screensaver, true)
    P0(bool, lion_style_fullscreen, false)
    P0(bool, hide_cursor_fs_only, false)
    P0(int, hide_cursor_delay_sec, 3)
    P0(bool, show_logo, true)
    P0(QColor, bg_color, Qt::black)
    P0(bool, use_heartbeat, false)
    P0(bool, exclude_images, true)
    P0(QString, heartbeat_command, {})
    P0(int, heartbeat_interval, 60)
    P0(QStringList, sub_priority, {})
    P0(QStringList, audio_priority, {})

    P0(bool, show_osd_on_action, true)
    P0(bool, show_osd_on_resized, true)
    P0(bool, show_osd_timeline, true)
    P0(OsdTheme, osd_style, defaultOsdStyle())

    P0(int, blur_kern_c, 1)
    P0(int, blur_kern_n, 2)
    P0(int, blur_kern_d, 1)
    P0(int, sharpen_kern_c, 5)
    P0(int, sharpen_kern_n, -1)
    P0(int, sharpen_kern_d, 0)

    P0(ChannelLayoutMap, channel_manipulation, ChannelLayoutMap::default_())

    P2(QList<MatchString>, sub_search_paths_v2, {}, "sub_search_paths")
    P0(bool, sub_enable_autoload, true)
    P0(bool, sub_enable_autoselect, true)
    P0(bool, sub_enc_autodetection, true)
    P0(SubtitleAutoload, sub_autoload, SubtitleAutoload::Contain)
    P0(SubtitleAutoselect, sub_autoselect, SubtitleAutoselect::Matched)
    P1(QString, sub_enc, defaultSubtitleEncoding(), "encoding")
    P1(QString, sub_ext, {}, "value")
    P0(int, sub_enc_accuracy, defaultSubtitleEncodingDetectionAccuracy())
    P0(int, ms_per_char, 500)
    P0(OsdTheme, sub_style, {})

    P0(bool, enable_system_tray, true)
    P0(bool, hide_rather_close, true)
    P0(MouseActionMap, mouse_action_map, defaultMouseActionMap())
    P0(bool, invert_wheel, false)
    P0(int, seek_step1_sec, 5)
    P0(int, seek_step2_sec, 30)
    P0(int, seek_step3_sec, 60)

    P0(int, speed_step, 10)
    P0(int, brightness_step, 1)
    P0(int, saturation_step, 1)
    P0(int, contrast_step, 1)
    P0(int, hue_step, 1)
    P0(int, volume_step, 2)
    P0(double, sub_sync_step_sec, 0.5)
    P0(double, audio_sync_step_sec, 0.2)
    P0(int, amp_step, 10)
    P0(int, sub_pos_step, 1)
    P0(bool, enable_hwaccel, false)
    P0(HwAcc::Type, hwaccel_backend, defaultHwAccBackend())
    P0(QVector<int>, hwaccel_codecs, defaultHwAccCodecs())
    QVector<DeintMethod> hwdeints = defaultHwAccDeints();
    P0(DeintCaps, deint_hwdec, DeintCaps::default_(DecoderDevice::GPU))
    P0(DeintCaps, deint_swdec, DeintCaps::default_(DecoderDevice::CPU))

    P0(AudioNormalizerOption, audio_normalizer, AudioNormalizerOption::default_())

    P1(QString, skin_name, defaultSkinName(), "currentText")

    P0(Shortcuts, shortcuts, defaultShortcuts())

    P1(QString, audio_device, u"auto"_q, "currentText")
    P0(ClippingMethod, clipping_method, ClippingMethod::Auto)

    P0(int, cache_local, 0)
    P0(int, cache_network, 25000)
    P0(int, cache_disc, 0)
    P0(int, cache_min_playback, 0)
    P0(int, cache_min_seeking, 2)
    P0(QStringList, network_folders, {})

    P0(QString, yt_user_agent, u"Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20100101 Firefox/10.0 (Chrome)"_q)
    P0(QString, yt_program, u"youtube-dl"_q)
    int yt_timeout = 60000;

    QStringList sub_search_paths;

    static auto preset(KeyMapPreset id) -> Shortcuts;

    auto save() const -> void;
    auto load() -> void;

    auto initialize() -> void;

    static auto fields() -> const QVector<const PrefFieldInfo*>&;
private:
    auto loadFromRecord() -> void;

    static auto defaultHwAccBackend() -> HwAcc::Type;
    static auto defaultRestoreProperties() -> QVector<QMetaProperty>;
    static auto defaultOsdStyle() -> OsdTheme;
    static auto defaultSkinName() -> QString;
    static auto defaultSubtitleEncoding() -> QString;
    static auto defaultSubtitleEncodingDetectionAccuracy() -> int;
    static auto defaultHwAccCodecs() -> QVector<int>;
    static auto defaultHwAccDeints() -> QVector<DeintMethod>;
    static auto defaultShortcuts() -> Shortcuts;
    static auto defaultMouseActionMap() -> MouseActionMap;
};
#undef P_
#undef P0
#undef P1
#undef P2

Q_DECLARE_METATYPE(QVector<QMetaProperty>);
Q_DECLARE_METATYPE(QList<MatchString>);
Q_DECLARE_METATYPE(Shortcuts);

#endif // PREF_HPP
