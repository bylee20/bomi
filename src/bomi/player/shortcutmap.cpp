#include "shortcutmap.hpp"
#include "misc/log.hpp"
#include "misc/json.hpp"
#include "player/rootmenu.hpp"

DECLARE_LOG_CONTEXT(Shortcut)

struct ShortcutMap::Data : public QSharedData {
    QMap<QString, Shortcut> map;
    auto set(const QMap<QString, QList<Key>> &keys, bool def) -> void
    {
        map.clear();
        for (auto it = keys.begin(); it != keys.end(); ++it) {
            auto &s = map[it.key()];
            s.m_default = def;
            s.m_id = it.key();
            s.m_keys = it.value();
        }
    }
};

ShortcutMap::ShortcutMap()
    : d(new Data)
{

}

ShortcutMap::ShortcutMap(const ShortcutMap &other)
    : d(other.d)
{

}

ShortcutMap::ShortcutMap(ShortcutMap &&other)
{
    d.swap(other.d);
}

ShortcutMap::~ShortcutMap()
{

}

auto ShortcutMap::operator = (const ShortcutMap &rhs) -> ShortcutMap&
{
    d = rhs.d;
    return *this;
}

auto ShortcutMap::operator = (ShortcutMap &&rhs) -> ShortcutMap&
{
    d.swap(rhs.d);
    return *this;
}

auto ShortcutMap::preset(Preset p) -> ShortcutMap
{
    if (p == Default)
        return ShortcutMap();
    QMap<QString, QList<Key>> keys;
    if (p == Movist) {
        keys[u"open/file"_q] << Qt::CTRL + Qt::Key_O;
        keys[u"window/close"_q] << Qt::CTRL + Qt::Key_W;
        keys[u"tool/playlist/save"_q] << Qt::CTRL + Qt::Key_S << Qt::CTRL + Qt::SHIFT + Qt::Key_S;
        keys[u"tool/playlist/append-file"_q] << Qt::CTRL + Qt::ALT + Qt::Key_L;
        keys[u"play/prev"_q] << Qt::ALT + Qt::CTRL + Qt::Key_Left;
        keys[u"play/next"_q] << Qt::ALT + Qt::CTRL + Qt::Key_Right;
        keys[u"play/play-pause"_q] << Qt::Key_Space;
        keys[u"play/seek/backword1"_q] << Qt::Key_Left;
        keys[u"play/seek/forward1"_q] << Qt::Key_Right;
        keys[u"play/repeat/quit"_q] << Qt::CTRL + Qt::Key_Backslash;
        keys[u"play/seek/range"_q] << Qt::CTRL + Qt::Key_BracketLeft << Qt::CTRL + Qt::Key_BracketRight;
        keys[u"play/speed/reset"_q] << Qt::SHIFT + Qt::CTRL + Qt::Key_Backslash;
        keys[u"play/repeat/faster"_q] << Qt::SHIFT + Qt::CTRL + Qt::Key_Right;
        keys[u"play/speed/slower"_q] << Qt::SHIFT + Qt::CTRL + Qt::Key_Left;
        keys[u"window/toggle-fs"_q] << Qt::META + Qt::CTRL + Qt::Key_F;
        keys[u"window/size4"_q] << Qt::CTRL + Qt::Key_0;
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
    }
    ShortcutMap map; map.d->set(keys, false);
    return map;
}

auto ShortcutMap::default_(const QString &id) -> QList<Key>
{
    using namespace Qt;
    static const QMap<QString, QList<Key>> def = [] () {
       QMap<QString, QList<Key>> map;
       map[u"open/file"_q] << Qt::CTRL + Qt::Key_F;
       map[u"open/folder"_q] << Qt::CTRL + Qt::Key_G;
       map[u"open/clipboard"_q] << Qt::CTRL + Qt::Key_V;

       map[u"play/play-pause"_q] << Qt::Key_Space;
       map[u"play/prev"_q] << Qt::CTRL + Qt::Key_Left;
       map[u"play/next"_q] << Qt::CTRL + Qt::Key_Right;
       map[u"play/state"_q] << Qt::Key_Semicolon;
       map[u"play/speed/reset"_q] << Qt::Key_Backspace;
       map[u"play/speed/increase"_q] << Qt::Key_Plus << Qt::Key_Equal;
       map[u"play/speed/decrease"_q] << Qt::Key_Minus;
       map[u"play/repeat/range"_q] << Qt::Key_R;
       map[u"play/repeat/subtitle"_q] << Qt::Key_E;
       map[u"play/repeat/quit"_q] << Qt::Key_Escape;
       map[u"play/seek/begin"_q] << Qt::CTRL + Qt::Key_Home;
       map[u"play/seek/record"_q] << Qt::ALT + Qt::Key_Home;
       map[u"play/seek/forward1"_q] << Qt::Key_Right;
       map[u"play/seek/forward2"_q] << Qt::Key_PageDown;
       map[u"play/seek/forward3"_q] << Qt::Key_End;
       map[u"play/seek/backward1"_q] << Qt::Key_Left;
       map[u"play/seek/backward2"_q] << Qt::Key_PageUp;
       map[u"play/seek/backward3"_q] << Qt::Key_Home;
       map[u"play/seek/prev-frame"_q] << Qt::ALT + Qt::Key_Left;
       map[u"play/seek/next-frame"_q] << Qt::ALT + Qt::Key_Right;
       map[u"play/seek/black-frame"_q] << Qt::ALT + Qt::Key_B;
       map[u"play/seek/prev-subtitle"_q] << Qt::Key_Comma;
       map[u"play/seek/current-subtitle"_q] << Qt::Key_Period;
       map[u"play/seek/next-subtitle"_q] << Qt::Key_Slash;

       map[u"subtitle/track/open"_q] << Qt::SHIFT + Qt::Key_F;
       map[u"subtitle/track/reload"_q] << Qt::SHIFT + Qt::Key_R;
       map[u"subtitle/track/cycle"_q] << Qt::SHIFT + Qt::Key_N;
       map[u"subtitle/track/all"_q] << Qt::SHIFT + Qt::Key_B;
       map[u"subtitle/track/hide"_q] << Qt::SHIFT + Qt::Key_H;
       map[u"subtitle/override"_q] << SHIFT + Key_O;
       map[u"subtitle/scale/decrease"_q] << SHIFT + Key_K;
       map[u"subtitle/scale/increase"_q] << SHIFT + Key_L;
       map[u"subtitle/position/increase"_q] << Qt::Key_S;
       map[u"subtitle/position/decrease"_q] << Qt::Key_W;
       map[u"subtitle/sync/increase"_q] << Qt::Key_D;
       map[u"subtitle/sync/reset"_q] << Qt::Key_Q;
       map[u"subtitle/sync/decrease"_q] << Qt::Key_A;
       map[u"subtitle/sync/prev"_q] << SHIFT + Key_Left;
       map[u"subtitle/sync/next"_q] << SHIFT + Key_Right;

       map[u"video/snapshot/quick"_q] << Qt::CTRL + Qt::Key_S;
       map[u"video/snapshot/tool"_q] << Qt::CTRL + Qt::SHIFT + Qt::Key_S;
       map[u"video/clip/range"_q] << Qt::CTRL + Qt::Key_C;
       map[u"video/clip/advanced"_q] << Qt::CTRL + Qt::SHIFT + Qt::Key_C;
       map[u"video/aspect/source"_q] << CTRL+SHIFT+Key_R;
       map[u"video/aspect/increase"_q] << CTRL+SHIFT+Key_A;
       map[u"video/aspect/decrease"_q] << CTRL+SHIFT+Key_D;
       map[u"video/zoom/reset"_q] << SHIFT + Key_R;
       map[u"video/zoom/increase"_q] << SHIFT + Key_T;
       map[u"video/zoom/decrease"_q] << SHIFT + Key_G;
       map[u"video/move/reset"_q] << Qt::SHIFT + Qt::Key_X;
       map[u"video/move/vertical+"_q] << Qt::SHIFT + Qt::Key_S;
       map[u"video/move/vertical-"_q] << Qt::SHIFT + Qt::Key_W;
       map[u"video/move/horizontal-"_q] << Qt::SHIFT + Qt::Key_A;
       map[u"video/move/horizontal+"_q] << Qt::SHIFT + Qt::Key_D;
       map[u"video/deinterlacing/cycle"_q] << Qt::CTRL + Qt::Key_D;
       map[u"video/color/editor"_q] << Key_K;
       map[u"video/color/reset"_q] << Qt::Key_O;
       map[u"video/interpolator/advanced"_q] << Qt::CTRL + Qt::SHIFT + Qt::Key_I;
       map[u"video/interpolator/cycle"_q] << Qt::CTRL + Qt::Key_I;
       map[u"video/dithering/cycle"_q] << Qt::CTRL + Qt::Key_T;
       map[u"video/motion"_q] << Qt::CTRL + Qt::Key_M;

       map[u"audio/track/cycle"_q] << Qt::CTRL + Qt::Key_A;
       map[u"audio/volume/increase"_q] << Qt::Key_Up;
       map[u"audio/volume/decrease"_q] << Qt::Key_Down;
       map[u"audio/volume/mute"_q] << Qt::Key_M;
       map[u"audio/normalizer"_q] << Qt::Key_N;
       map[u"audio/tempo-scaler"_q] << Qt::Key_Z;
       map[u"audio/amp/increase"_q] << Qt::CTRL + Qt::Key_Up;
       map[u"audio/amp/decrease"_q] << Qt::CTRL + Qt::Key_Down;
       map[u"audio/equalizer"_q] << Qt::ALT + Qt::Key_E;
       map[u"audio/channel/cycle"_q] << Qt::ALT + Qt::Key_C;
       map[u"audio/sync/reset"_q] << Qt::Key_Backslash;
       map[u"audio/sync/increase"_q] << Qt::Key_BracketRight;
       map[u"audio/sync/decrease"_q] << Qt::Key_BracketLeft;


       map[u"tool/undo"_q] << Qt::CTRL + Qt::Key_Z;
       map[u"tool/redo"_q] << Qt::CTRL + Qt::SHIFT + Qt::Key_Z;
       map[u"tool/playlist/toggle"_q] << Qt::Key_L;
       map[u"tool/history/toggle"_q] << Qt::Key_C;
       map[u"tool/subtitle"_q] << Qt::SHIFT + Qt::Key_V;
       map[u"tool/find-subtitle"_q] << Qt::SHIFT + Qt::CTRL + Qt::Key_F;
       map[u"tool/pref"_q] << Qt::Key_P;
       map[u"tool/reload-skin"_q] << Qt::Key_R + Qt::CTRL;
       map[u"tool/playinfo"_q] << Qt::Key_Tab;
       map[u"tool/log"_q] << Qt::ALT + Qt::Key_L;

       map[u"window/size0"_q] << Qt::Key_QuoteLeft;
       map[u"window/size1"_q] << Qt::Key_1;
       map[u"window/size2"_q] << Qt::Key_2;
       map[u"window/size3"_q] << Qt::Key_3;
       map[u"window/size4"_q] << Qt::Key_0;
       map[u"window/toggle-fs"_q] << Qt::Key_Enter << Qt::Key_Return << Qt::Key_F;
       map[u"window/close"_q] << Qt::CTRL + Qt::Key_W;

#ifndef Q_OS_MAC
#ifdef Q_OS_WIN
       map[u"exit"_q] << Qt::ALT + Qt::Key_F4;
#endif
       map[u"exit"_q] << Qt::CTRL + Qt::Key_Q;
#endif
       return map;
    }();
    return def.value(id);
}

auto ShortcutMap::clear(const QString &id) -> void
{
    d->map[id].clear();
}

auto ShortcutMap::reset(const QString &id) -> void
{
    d->map.remove(id);
}

auto ShortcutMap::keys(const QString &id) const -> QList<Key>
{
    auto it = d->map.find(id);
    if (it != d->map.end()) {
        if (!it->isDefault())
            return it->m_keys;
        _Warn("Default shortcut for '%%' is specified.", id);
    }
    return default_(id);
}

auto ShortcutMap::shortcut(const QString &id) const -> Shortcut
{
    auto it = d->map.find(id);
    if (it != d->map.end()) {
        if (!it->isDefault())
            return *it;
        _Warn("Default shortcut for '%%' is specified.", id);
    }
    Shortcut shortcut;
    shortcut.m_id = id;
    shortcut.m_default = true;
    shortcut.m_keys = default_(id);
    return shortcut;
}

auto ShortcutMap::insert(const Shortcut &s) -> void
{
    if (s.id().isEmpty()) {
        _Warn("Ignore invalid shortcut with empty id.");
        return;
    }
    if (s.isDefault())
        d->map.remove(s.id());
    else
        d->map[s.id()] = s;
}

auto ShortcutMap::import(const QString &id, const QList<Key> &keys) -> void
{
    if (keys == default_(id))
        return;
    Shortcut s;
    s.m_default = false;
    s.m_id = id;
    s.m_keys = keys;
    insert(s);
}


auto ShortcutMap::toJson() const -> QJsonObject
{
    QJsonObject json;
    for (auto it = d->map.begin(); it != d->map.end(); ++it) {
        if (it->isDefault())
            continue;
        QJsonObject o;
        o.insert(u"default"_q, false);
        o.insert(u"keys"_q, _ToJson(it->m_keys));
        json.insert(it.key(), o);
    }
    return json;
}

auto ShortcutMap::setFromJson(const QJsonObject &json) -> bool
{
    d->map.clear();
    const auto &r = RootMenu::instance();
    for (auto it = json.begin(); it != json.end(); ++it) {
        const auto o = it.value().toObject();
        if (o[u"default"_q].toBool())
            continue;
        const auto id = r.resolve(it.key());
        auto &s = d->map[id];
        s.m_id = id;
        s.m_default = false;
        s.m_keys = _FromJson<QList<Key>>(o[u"keys"_q]);
    }
    return true;
}

auto ShortcutMap::operator == (const ShortcutMap &rhs) const -> bool
{
    return d->map == rhs.d->map;
}
