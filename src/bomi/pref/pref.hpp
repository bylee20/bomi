#ifndef PREF_HPP
#define PREF_HPP

#include "player/openmediainfo.hpp"
#include "player/mrlstate.hpp"
#include "audio/channellayoutmap.hpp"
#include "audio/audionormalizeroption.hpp"
#include "video/deintcaps.hpp"
#include "video/deintoption.hpp"
#include "video/motionintrploption.hpp"
#include "misc/keymodifieractionmap.hpp"
#include "misc/osdstyle.hpp"
#include "misc/autoloader.hpp"
#include "misc/locale.hpp"
#include "misc/logoption.hpp"
#include "misc/stepinfo.hpp"
#include "enum/generateplaylist.hpp"
#include "enum/autoselectmode.hpp"
#include "enum/audiodriver.hpp"
#include "enum/verticalalignment.hpp"
#include "enum/quicksnapshotsave.hpp"
#include "enum/mousebehavior.hpp"
#include "enum/codecid.hpp"
#include "quick/playlistthemeobject.hpp"
#include "quick/osdthemeobject.hpp"
#include "os/os.hpp"
#include "pref_helper.hpp"
#include "player/shortcutmap.hpp"
#include "enum/jrconnection.hpp"
#include "enum/jrprotocol.hpp"
#include "misc/windowsize.hpp"

using Shortcuts = QMap<QString, QList<QKeySequence>>; // keep for backward compat

class Pref : public QObject {
    Q_OBJECT
/***************************************************************/
#define P_(type, name, def, editor_property, editor_name) \
public: \
    auto name() const -> type { return m_##name; } \
private: \
    type m_##name = def; \
    Q_PROPERTY(type name READ name WRITE set_##name) \
    auto set_##name(const type &t) { m_##name = t; } \
    Q_INVOKABLE QString editor_property_##name() const { return QString::fromLatin1(editor_property); } \
    Q_INVOKABLE QString editor_name_##name() const { return QString::fromLatin1(editor_name); } \
    Q_INVOKABLE bool compare_##name(const QVariant &var) const { return m_##name == var.value<type>(); } \
public:
#define P0(type, var, def) P_(type, var, def, PrefEditorProperty<type>::name, #var)
#define P1(type, var, def, editor) P_(type, var, def, editor, #var)
#define P2(type, var, def, editor) P_(type, var, def, #var, editor)
/***************************************************************/
public:
    Pref();
private:
    P0(OpenMediaInfo, open_media_from_file_manager, {OpenMediaBehavior::NewPlaylist})
    P0(OpenMediaInfo, open_media_by_drag_and_drop, {OpenMediaBehavior::Append})
    P0(QString, quick_snapshot_template, u"bomi-snapshot-%YEAR%-%MONTH_0%-%DAY_0%-%HOUR_0%-%MIN_0%-%SEC_0%"_q)
    P1(QString, quick_snapshot_format, u"png"_q, "currentText")
    P0(QString, quick_snapshot_folder, _WritablePath(Location::Pictures))
    P0(int, quick_snapshot_quality, -1)
    P0(QuickSnapshotSave, quick_snapshot_save, QuickSnapshotSave::Fixed)

    P0(bool, jr_use, false)
    P0(JrConnection, jr_connection, JrConnection::Tcp)
    P0(JrProtocol, jr_protocol, JrProtocol::Raw)
    P0(QString, jr_address, u"localhost"_q)
    P0(int, jr_port, 2020)

    P0(bool, load_last, true)
    P0(bool, fit_to_video, false)
    P0(bool, use_mpris2, true)
    P0(bool, pause_minimized, true)
    P0(bool, pause_video_only, true)
    P0(bool, remember_stopped, true)
    P0(bool, resume_ignore_in_playlist, false)
    P0(bool, precise_seeking, false)
    P0(bool, remember_image, false)
    P0(bool, enable_generate_playlist, true)
    P0(QStringList, restore_properties, defaultRestoreProperties())
    P0(GeneratePlaylist, generate_playlist, GeneratePlaylist::Folder)
    P0(bool, hide_cursor, true)
    P0(bool, disable_screensaver, true)
    P1(QString, screensaver_method, u"auto"_q, "currentText")
    P0(bool, lion_style_fullscreen, false)
    P0(bool, hide_cursor_fs_only, false)
    P0(double, hide_cursor_delay_sec, 3)
    P0(bool, preserve_downloaded_subtitles, true)
    P0(QString, preserve_file_name_format, defaultFileNameFormat())
    P0(QString, preserve_fallback_folder, defaultFallbackFolder())
    P0(bool, show_logo, true)
    P0(QColor, bg_color, Qt::black)
    P0(bool, exclude_images, true)
    P0(QStringList, sub_priority, {})
    P0(QStringList, audio_priority, {})
    P0(int, volume_scale, 60)

    P0(OsdTheme, osd_theme, defaultOsdTheme())
    P0(ControlsTheme, controls_theme, {})

    P0(MotionIntrplOption, motion_interpolation, {})

    P0(ChannelLayoutMap, channel_manipulation, ChannelLayoutMap::default_())

    P0(Autoloader, sub_autoload_v2, defaultSubtitleAutoload())
    P0(Autoloader, audio_autoload, defaultAutioAutoload())
    P0(bool, sub_enable_autoselect, true)
    P0(bool, sub_enc_autodetection, true)
    P0(AutoselectMode, sub_autoselect, AutoselectMode::Matched)
    P1(EncodingInfo, sub_enc, defaultSubtitleEncoding(), "encoding")
    P1(QString, sub_ext, {}, "value")
    P0(int, sub_enc_accuracy, defaultSubtitleEncodingDetectionAccuracy())
    P0(int, ms_per_char, 500)
    P0(OsdStyle, sub_style, {})
    P0(bool, sub_prefer_external, true)

    P0(bool, enable_system_tray, true)
    P0(bool, hide_rather_close, true)
    P0(MouseActionMap, mouse_action_map, defaultMouseActionMap())
    P0(bool, invert_wheel, false)

    P0(Steps, steps, {})
    P0(QList<WindowSize>, window_sizes, WindowSize::defaults())

    P0(bool, enable_hwaccel, false)
    P0(QList<CodecId>, hwaccel_codecs, OS::hwAcc()->fullCodecList())
    P0(DeintOptionSet, deinterlacing, {})

    P0(bool, audio_filter_resync, true)
    P0(AudioNormalizerOption, audio_normalizer, AudioNormalizerOption::default_())

    P1(QString, skin_name, defaultSkinName(), "currentText")

//    P0(Shortcuts, shortcuts, defaultShortcuts())
    P0(ShortcutMap, shortcut_map, {})

    P1(QString, audio_device, u"auto"_q, "currentText")
    P0(bool, soft_clip, true)
    P0(bool, auto_unmute, false)

    P0(double, cache_local_mb, 0)
    P0(double, cache_network_mb, 150)
    P0(double, cache_disc_mb, 0)
    P0(double, cache_local_sec, 10)
    P0(double, cache_network_sec, 60)
    P0(double, cache_disc_sec, 10)
    P0(bool, cache_local_file, false)
    P0(bool, cache_network_file, true)
    P0(bool, cache_disc_file, false)
    P0(int, cache_min_playback_kb, 0)
    P0(int, cache_min_seeking_kb, 500)
    P0(double, cache_file_size_mb, 1024)
    P0(QStringList, network_folders, {})

    P0(QString, yt_user_agent, u"Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20100101 Firefox/10.0 (Chrome)"_q)
    P0(QString, yt_program, u"youtube-dl"_q)
    P0(int, yt_height, 720)
    P0(int, yt_fps, 0)
    P1(QString, yt_container, u"webm"_q, "currentText");
    P0(QString, yle_program, u"yle-dl"_q)
    int yt_timeout = 60000;

    P0(QString, smb_username, {});
    P0(QString, smb_password, {});

    QStringList sub_search_paths;

    P0(bool, app_unique, true);
    P0(bool, app_use_local_config, false);
    P1(Locale, app_locale, {}, "locale");
    P1(QString, app_style, {}, "value");
    P0(LogOption, app_log_option, LogOption::default_())
    P1(QFont, app_font, {}, "currentFont")
    P1(QFont, app_fixed_font, {}, "currentFont")

//    static auto preset(KeyMapPreset id) -> Shortcuts;
public:
    auto save() const -> void;
    auto load() -> void;

    auto initialize() -> void;
private:
    static auto defaultSubtitleAutoload() -> Autoloader;
    static auto defaultAutioAutoload() -> Autoloader;
    static auto defaultRestoreProperties() -> QStringList;
    static auto defaultOsdTheme() -> OsdTheme;
    static auto defaultSkinName() -> QString;
    static auto defaultSubtitleEncoding() -> EncodingInfo;
    static auto defaultSubtitleEncodingDetectionAccuracy() -> int;
    static auto defaultMouseActionMap() -> MouseActionMap;
    static auto defaultFileNameFormat() -> QString;
    static auto defaultFallbackFolder() -> QString;
};
#undef P_
#undef P0
#undef P1
#undef P2

Q_DECLARE_METATYPE(QVector<QMetaProperty>);
Q_DECLARE_METATYPE(QList<MatchString>);
Q_DECLARE_METATYPE(Shortcuts);

#endif // PREF_HPP
