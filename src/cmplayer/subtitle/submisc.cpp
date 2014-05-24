#include "submisc.hpp"
#include "subtitle.hpp"

static const QString sep1("[::1::]"), sep2("[::2::]"), sep3("[::3::]");

template<class List, class F>
auto _Join(const List &list, F toString, const QString &sep) -> QString
{
    QStringList l; l.reserve(list.size());
    for (auto &one : list)
        l.append(toString(one));
    return l.join(sep);
}

template<class T, class F>
auto _Split(const QString &text, F fromString, const QString &sep,
                QString::SplitBehavior b = QString::KeepEmptyParts) -> QList<T>
{
    auto strs = text.split(sep, b);
    QList<T> list; list.reserve(strs.size());
    for (int i=0; i<strs.size(); ++i) {
        list.append(fromString(strs[i]));
    }
    return list;
}

auto SubtitleStateInfo::toJson() const -> QJsonObject
{
    QJsonArray mpv;
    for (auto &item : m_mpv)
        mpv.push_back(item.toJson());
    QJsonArray cmplayer;
    for (auto it = m_cmplayer.begin(); it != m_cmplayer.end(); ++it) {
        QJsonArray list;
        for (auto &comp : *it)
            list.push_back(_MakeJson({{"id", comp.id}, {"selected", comp.selected}}));
        cmplayer.push_back(_MakeJson({{"file", it.key().toJson()}, {"list", list}}));
    }
    return _MakeJson({{"track", m_track}, {"mpv", mpv}, {"cmplayer", cmplayer}});
}

auto SubtitleStateInfo::fromJson(const QJsonObject &json) -> SubtitleStateInfo
{
    SubtitleStateInfo info; QJsonValue value;
    auto get = [&] (const char *key)
        { value = json.value(_L(key)); return !value.isUndefined(); };
#define CHECK(a) { if (!get(a)) return SubtitleStateInfo(); }
    CHECK("track");
    info.m_track = value.toInt(InvalidTrack);
    CHECK("mpv");
    auto array = value.toArray();
    for (auto item : array)
        info.m_mpv.push_back(SubtitleFileInfo::fromJson(item.toObject()));
    CHECK("cmplayer");
    array = value.toArray();
    for (auto item : array) {
        auto cmp = item.toObject();
        auto file = SubtitleFileInfo::fromJson(cmp.value("file").toObject());
        auto list = cmp.value("list").toArray();
        if (file.path.isEmpty())
            return SubtitleStateInfo();
        if (list.isEmpty())
            continue;
        auto &comps = info.m_cmplayer[file];
        for (auto comp : list) {
            const auto obj = comp.toObject();
            const auto id = obj.value("id");
            const auto se = obj.value("selected");
            if (id.isUndefined() || se.isUndefined())
                return SubtitleStateInfo();
            comps.push_back({id.toInt(), se.toBool()});
        }
    }
#undef CHECK
    return info;
}

auto SubtitleStateInfo::toString() const -> QString
{
    QStringList list;
    list.append(_N(m_track));
    list.append(_Join(m_mpv, [] (const SubtitleFileInfo &info) {
        return info.toString();
    }, sep2));
    for (auto it = m_cmplayer.begin(); it != m_cmplayer.end(); ++it) {
        QStringList item;
        item.append(it.key().toString());
        for (auto &comp : *it) {
            item.append(_N(comp.id));
            item.append(_N(comp.selected));
        }
        list.append(item.join(sep2));
    }
    return list.join(sep1);
}

auto SubtitleStateInfo::fromString(const QString &string) -> SubtitleStateInfo
{
    if (string.isEmpty())
        return SubtitleStateInfo();
    auto list = string.split(sep1, QString::KeepEmptyParts);
    if (list.size() < 2)
        return SubtitleStateInfo();
    SubtitleStateInfo info;
    info.m_track = list[0].toInt();
    auto str2sfi = [] (const QString &s)
        { return SubtitleFileInfo::fromString(s); };
    info.m_mpv = _Split<SubtitleFileInfo>(list[1], str2sfi, sep2,
                                          QString::SkipEmptyParts);
    for (int i=2; i<list.size(); ++i) {
        auto item = list[i].split(sep2, QString::KeepEmptyParts);
        if (item.size() % 2 != 1)
            return SubtitleStateInfo();
        auto &map = info.m_cmplayer[str2sfi(item[0])];
        map.reserve(item.size()/2);
        Comp comp;
        for (int j=1; j<item.size();) {
            comp.id = item[j++].toInt();
            comp.selected = item[j++].toInt();
            map.append(comp);
        }
    }
    return info;
}

auto SubtitleStateInfo::append(const SubComp &c) -> void
{
    m_cmplayer[c.fileInfo()].append({c.id(), c.selection()});
}

auto SubtitleStateInfo::load() const -> QList<SubComp>
{
    auto selected = [] (const QList<Comp> &list, int id) {
        for (auto &c : list) { if (c.id == id) return c.selected; }
        return false;
    };
    QList<SubComp> loaded;         Subtitle sub;
    for (auto it = m_cmplayer.begin(); it != m_cmplayer.end(); ++it) {
        if (!sub.load(it.key().path, it.key().encoding, -1))
            continue;
        if (sub.size() != it->size())
            continue;
        for (int i=0; i<sub.size(); ++i) {
            loaded.append(sub[i]);
            loaded.last().selection() = selected(*it, loaded.last().id());
        }
    }
    return loaded;
}
