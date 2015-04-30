#include "pref.hpp"
#include "player/app.hpp"
#include "enum/codecid.hpp"
#include "misc/jsonstorage.hpp"
#include "pref_helper.hpp"
#include "configure.hpp"
#include <QFontDatabase>

DECLARE_LOG_CONTEXT(Pref)

static bool init = false;

Pref::Pref()
{
    m_app_style = cApp.defaultStyleName();
    m_app_font = OS::defaultFont();
    m_app_fixed_font = OS::defaultFixedFont();
}

auto Pref::initialize() -> void
{
    if (init)
        return;
    init = true;
}

#define PREF_FILE_PATH QString(_WritablePath(Location::Config) % "/pref.json"_a)

auto translator_default_encoding() -> EncodingInfo;

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

auto Pref::save() const -> void
{
    JsonStorage storage(PREF_FILE_PATH);
    QJsonObject json = _JsonFromQObject(this);
    storage.write(json);
    cApp.setUnique(m_app_unique);
    cApp.setUseLocalConfig(m_app_use_local_config);
    cApp.setLocale(m_app_locale);
    cApp.setStyleName(m_app_style);
    cApp.setLogOption(m_app_log_option);
    cApp.setFont(m_app_font);
    cApp.setFixedFont(m_app_fixed_font);
    cApp.save();
}

auto Pref::load() -> void
{
    JsonStorage storage(PREF_FILE_PATH);
    auto json = storage.read();
    if (storage.hasError())
        ;
    else {
        bool res = _JsonToQObject(json, this);
        if (!res)
            _Warn("Failed to read some fields from JSON object.");

        if (json.contains(u"seek_step1_sec"_q)) {
#define RES_STEP(var, id) m_steps.var.set(json.value(u"" #id ""_q).toDouble(m_steps.var.get()))
            RES_STEP(seek1_sec, seek_step1_sec);
            RES_STEP(seek2_sec, seek_step2_sec);
            RES_STEP(seek3_sec, seek_step3_sec);
            RES_STEP(speed_pct, speed_step);
            RES_STEP(aspect_ratio,aspect_ratio_step);
            RES_STEP(color_pct, brightness_step);
            RES_STEP(volume_pct, volume_step);
            RES_STEP(amp_pct, amp_step);
            RES_STEP(sub_sync_sec, sub_sync_step_sec);
            RES_STEP(audio_sync_sec, audio_sync_step_sec);
            RES_STEP(sub_pos_pct, sub_pos_step);
#undef RES_STEP
        }

        if (json.contains(u"sub_enable_autoload"_q)) {
            m_sub_autoload_v2.enabled = json[u"sub_enable_autoload"_q].toBool();
            m_sub_autoload_v2.search_paths = _FromJson<QList<MatchString>>(json[u"sub_search_paths_v2"_q]);
            m_sub_autoload_v2.mode = _FromJson<AutoloadMode>(json[u"sub_autoload"_q]);
        }
        int idx = m_restore_properties.indexOf(u"audio_track"_q);
        if (idx != -1)
            m_restore_properties[idx] = u"audio_tracks"_q;
        idx = m_restore_properties.indexOf(u"sub_track"_q);
        if (idx != -1) {
            m_restore_properties[idx] = u"sub_tracks"_q;
            m_restore_properties.append(u"sub_tracks_inclusive"_q);
        }
        if (json.contains(u"shortcuts"_q)) {
            const auto &old = _FromJson<Shortcuts>(json[u"shortcuts"_q]);
            ShortcutMap map;
            for (auto it = old.begin(); it != old.end(); ++it)
                map.import(it.key(), *it);
            m_shortcut_map = map;
        }

        if (!m_mouse_action_map.contains(MouseBehavior::RightClick))
            m_mouse_action_map[MouseBehavior::RightClick]
                = defaultMouseActionMap()[MouseBehavior::RightClick];

        if (json.contains(u"playlist_theme"_q)) {
            m_controls_theme.showOnMouseMoved = json.value(u"show_controls_when_mouse_moved"_q).toBool(true);

            PlaylistTheme playlist;
            playlist.setFromJson(json[u"playlist_theme"_q].toObject());
            m_controls_theme.showLocationsInPlaylist = playlist.showLocation;
        }

        if (json.contains(u"cache_local"_q)) {
            m_cache_local_mb = json[u"cache_local"_q].toInt() / 1024.;
            m_cache_network_mb = json[u"cache_network"_q].toInt() / 1024.;
            m_cache_disc_mb = json[u"cache_disc"_q].toInt() / 1024.;
        }
    }
    m_app_unique = cApp.isUnique();
    m_app_use_local_config = cApp.useLocalConfig();
    m_app_locale = cApp.locale();
    m_app_style = cApp.styleName().toLower();
    m_app_log_option = cApp.logOption();
    m_app_font = cApp.font();
    m_app_fixed_font = cApp.fixedFont();
}

#undef PREF_GROUP

auto Pref::defaultSubtitleEncoding() -> EncodingInfo
{
    return translator_default_encoding();
}

auto Pref::defaultSkinName() -> QString
{
    QString name = QString::fromLatin1(BOMI_DEFAULT_SKIN);
    if (name.isEmpty())
        name = u"Tethys"_q;
    return name;
}

auto Pref::defaultRestoreProperties() -> QStringList
{
    QStringList list;
    auto &mo = MrlState::staticMetaObject;
    const int count = mo.propertyCount();
    for (int i=mo.propertyOffset(); i<count; ++i) {
        const auto property = mo.property(i);
        if (property.revision())
            list.append(_L(property.name()));
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
    auto &style = theme.style;
    style.font.size = 0.03;
    style.font.color = Qt::white;
    style.outline.color = Qt::black;
    return theme;
}

auto Pref::defaultMouseActionMap() -> MouseActionMap
{
    MouseActionMap map;
    map[MouseBehavior::DoubleClick][KeyModifier::None] = u"window/toggle-fs"_q;
    auto &right = map[MouseBehavior::RightClick];
    for (auto item : KeyModifierInfo::items())
        right[item.value] = u"context-menu"_q;
    map[MouseBehavior::MiddleClick][KeyModifier::None] = u"play/play-pause"_q;
    map[MouseBehavior::ScrollUp][KeyModifier::None] = u"audio/volume/increase"_q;
    map[MouseBehavior::ScrollUp][KeyModifier::Ctrl] = u"audio/amp/increase"_q;
    map[MouseBehavior::ScrollDown][KeyModifier::None] = u"audio/volume/decrease"_q;
    map[MouseBehavior::ScrollDown][KeyModifier::Ctrl] = u"audio/amp/decrease"_q;
    map[MouseBehavior::Extra1Click][KeyModifier::None] = u"play/seek/backward1"_q;
    map[MouseBehavior::Extra2Click][KeyModifier::None] = u"play/seek/forward1"_q;
    return map;
}

auto Pref::defaultSubtitleAutoload() -> Autoloader
{
    Autoloader al;
    al.enabled = true;
    al.mode = AutoloadMode::Contain;
    return al;
}

auto Pref::defaultAutioAutoload() -> Autoloader
{
    Autoloader al;
    al.enabled = true;
    al.mode = AutoloadMode::Matched;
    al.search_paths << MatchString(u".*"_q, true);
    return al;
}

auto Pref::defaultFileNameFormat() -> QString
{
    return u"%MEDIA_NAME%"_q % '-'_q % u"%SUBTITLE_NAME%"_q % '.'_q % u"%SUBTITLE_EXT%"_q;
}

auto Pref::defaultFallbackFolder() -> QString
{
    return _WritablePath(Location::Download);
}
