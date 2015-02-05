#include "pref.hpp"
#include "misc/log.hpp"
#include "mrlstate.hpp"
#include "video/hwacc.hpp"
#include "misc/jsonstorage.hpp"
#include "pref_helper.hpp"
#include "configure.hpp"

DECLARE_LOG_CONTEXT(Pref)

static bool init = false;

auto Pref::initialize() -> void
{
    if (init)
        return;
    init = true;
    qRegisterMetaType<const PrefFieldInfo*>();
    qRegisterMetaType<QVector<QMetaProperty>>();
    qRegisterMetaType<ChannelLayoutMap>();
    qRegisterMetaType<QList<MatchString>>();
    qRegisterMetaType<MouseActionMap>();
    qRegisterMetaType<AudioNormalizerOption>();
    qRegisterMetaType<DeintCaps>();
    qRegisterMetaType<Shortcuts>();
    qRegisterMetaType<OsdStyle>();
    qRegisterMetaType<HwAcc::Type>();
    auto &mo = *this->metaObject();
    for (int i =  mo.methodOffset(); i < mo.methodCount(); ++i) {
        auto method = mo.method(i);
        if (method.name().startsWith("init_"))
            method.invoke(this, Qt::DirectConnection);
    }
}

auto Pref::fields() -> const QVector<const PrefFieldInfo*>&
{
    return PrefFieldInfo::getList();
}

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
    keys[u"play/state"_q] << Qt::Key_Semicolon;
    keys[u"play/speed/reset"_q] << Qt::Key_Backspace;
    keys[u"play/speed/increase"_q] << Qt::Key_Plus << Qt::Key_Equal;
    keys[u"play/speed/decrease"_q] << Qt::Key_Minus;
    keys[u"play/repeat/range"_q] << Qt::Key_R;
    keys[u"play/repeat/subtitle"_q] << Qt::Key_E;
    keys[u"play/repeat/quit"_q] << Qt::Key_Escape;
    keys[u"play/seek/begin"_q] << Qt::CTRL + Qt::Key_Home;
    keys[u"play/seek/forward1"_q] << Qt::Key_Right;
    keys[u"play/seek/forward2"_q] << Qt::Key_PageDown;
    keys[u"play/seek/forward3"_q] << Qt::Key_End;
    keys[u"play/seek/backward1"_q] << Qt::Key_Left;
    keys[u"play/seek/backward2"_q] << Qt::Key_PageUp;
    keys[u"play/seek/backward3"_q] << Qt::Key_Home;
    keys[u"play/seek/prev-frame"_q] << Qt::ALT + Qt::Key_Left;
    keys[u"play/seek/next-frame"_q] << Qt::ALT + Qt::Key_Right;
    keys[u"play/seek/black-frame"_q] << Qt::ALT + Qt::Key_B;
    keys[u"play/seek/prev-subtitle"_q] << Qt::Key_Comma;
    keys[u"play/seek/current-subtitle"_q] << Qt::Key_Period;
    keys[u"play/seek/next-subtitle"_q] << Qt::Key_Slash;

    keys[u"subtitle/track/open"_q] << Qt::SHIFT + Qt::Key_F;
    keys[u"subtitle/track/reload"_q] << Qt::SHIFT + Qt::Key_R;
    keys[u"subtitle/track/cycle"_q] << Qt::SHIFT + Qt::Key_N;
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
    keys[u"video/deinterlacing/cycle"_q] << Qt::CTRL + Qt::Key_D;
    keys[u"video/color/reset"_q] << Qt::Key_O;
    keys[u"video/color/brightness+"_q] << Qt::Key_T;
    keys[u"video/color/brightness-"_q] << Qt::Key_G;
    keys[u"video/color/contrast+"_q] << Qt::Key_Y;
    keys[u"video/color/contrast-"_q] << Qt::Key_H;
    keys[u"video/color/saturation+"_q] << Qt::Key_U;
    keys[u"video/color/saturation-"_q] << Qt::Key_J;
    keys[u"video/color/hue+"_q] << Qt::Key_I;
    keys[u"video/color/hue-"_q] << Qt::Key_K;
    keys[u"video/interpolator/advanced"_q] << Qt::CTRL + Qt::SHIFT + Qt::Key_I;
    keys[u"video/interpolator/cycle"_q] << Qt::CTRL + Qt::Key_I;
    keys[u"video/dithering/cycle"_q] << Qt::CTRL + Qt::Key_T;

    keys[u"audio/track/cycle"_q] << Qt::CTRL + Qt::Key_A;
    keys[u"audio/volume/increase"_q] << Qt::Key_Up;
    keys[u"audio/volume/decrease"_q] << Qt::Key_Down;
    keys[u"audio/volume/mute"_q] << Qt::Key_M;
    keys[u"audio/normalizer"_q] << Qt::Key_N;
    keys[u"audio/tempo-scaler"_q] << Qt::Key_Z;
    keys[u"audio/amp/increase"_q] << Qt::CTRL + Qt::Key_Up;
    keys[u"audio/amp/decrease"_q] << Qt::CTRL + Qt::Key_Down;
    keys[u"audio/equalizer"_q] << Qt::ALT + Qt::Key_E;
    keys[u"audio/channel/cycle"_q] << Qt::ALT + Qt::Key_C;
    keys[u"audio/sync/reset"_q] << Qt::Key_Backslash;
    keys[u"audio/sync/increase"_q] << Qt::Key_BracketRight;
    keys[u"audio/sync/decrease"_q] << Qt::Key_BracketLeft;


    keys[u"tool/undo"_q] << Qt::CTRL + Qt::Key_Z;
    keys[u"tool/redo"_q] << Qt::CTRL + Qt::SHIFT + Qt::Key_Z;
    keys[u"tool/playlist/toggle"_q] << Qt::Key_L;
    keys[u"tool/history/toggle"_q] << Qt::Key_C;
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

auto Pref::save() const -> void
{
    JsonStorage storage(PREF_FILE_PATH);
    QJsonObject json = _JsonFromQObject(this);
    storage.write(json);
}

auto Pref::load() -> void
{
    JsonStorage storage(PREF_FILE_PATH);
    auto json = storage.read();
    if (storage.hasError())
        return;
    bool res = _JsonToQObject(json, this);
    if (!res)
        _Error("Error: Cannot convert JSON object to preferences");

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
}

auto Pref::defaultHwAccDeints() -> QVector<DeintMethod>
{
    QVector<DeintMethod> deints;
    for (auto deint : HwAcc::fullDeintList()) {
        if (HwAcc::supports(deint))
            deints << deint;
    }
    return deints;
}

auto Pref::defaultHwAccCodecs() -> QStringList
{
    QStringList codecs;
    for (auto codec : HwAcc::fullCodecList()) {
        if (HwAcc::supports(codec))
            codecs.push_back(codec);
    }
    return codecs;
}

#undef PREF_GROUP

auto Pref::defaultSubtitleEncoding() -> QString
{
    return translator_default_encoding();
}

auto Pref::defaultSkinName() -> QString
{
    QString name = QString::fromLatin1(BOMI_DEFAULT_SKIN);
    if (name.isEmpty())
        name = u"Ardis"_q;
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
    style.font.setUnderline(false);
    style.font.setStrikeOut(false);
    style.font.setItalic(false);
    style.font.setBold(false);
    style.font.setFamily(qApp->font().family());
    style.font.size = 0.03;
//    theme.style = TextThemeStyle::Outline;
    style.font.color = Qt::white;
    style.outline.color = Qt::black;
    return theme;
}

auto Pref::defaultMouseActionMap() -> MouseActionMap
{
    MouseActionMap map;
    map[MouseBehavior::DoubleClick][KeyModifier::None] = u"window/full"_q;
    map[MouseBehavior::MiddleClick][KeyModifier::None] = u"play/pause"_q;
    map[MouseBehavior::ScrollUp][KeyModifier::None] = u"audio/volume/increase"_q;
    map[MouseBehavior::ScrollUp][KeyModifier::Ctrl] = u"audio/amp/increase"_q;
    map[MouseBehavior::ScrollDown][KeyModifier::None] = u"audio/volume/decrease"_q;
    map[MouseBehavior::ScrollDown][KeyModifier::Ctrl] = u"audio/amp/decrease"_q;
    map[MouseBehavior::Extra1Click][KeyModifier::None] = u"play/seek/backward1"_q;
    map[MouseBehavior::Extra2Click][KeyModifier::None] = u"play/seek/forward1"_q;
    return map;
}

auto Pref::defaultHwAccBackend() -> HwAcc::Type
{
    return HwAcc::VaApiGLX;
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
