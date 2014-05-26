#include "submisc.hpp"
#include "subtitle.hpp"
#include "misc/json.hpp"

#define JSON_CLASS SubtitleFileInfo
static const auto fileIO = JIO( JE(path), JE(encoding) );
#undef JSON_CLASS

#define JSON_CLASS SubtitleStateInfo::Comp
static const auto compIO = JIO( JE(id), JE(selected) );
#undef JSON_CLASS

auto json_io(SubtitleFileInfo*) -> decltype(&fileIO) { return &fileIO; }
auto json_io(SubtitleStateInfo::Comp*) -> decltype(&compIO) { return &compIO; }

auto SubtitleFileInfo::toJson() const -> QJsonObject
{
    return fileIO.toJson(*this);
}

auto SubtitleFileInfo::fromJson(const QJsonObject &json) -> SubtitleFileInfo
{
    SubtitleFileInfo info;
    fileIO.fromJson(info, json);
    return info;
}

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
                QString::SplitBehavior b = QString::KeepEmptyParts) -> QVector<T>
{
    auto strs = text.split(sep, b);
    QVector<T> list; list.reserve(strs.size());
    for (int i=0; i<strs.size(); ++i)
        list.append(fromString(strs[i]));
    return list;
}

template<>
inline auto json_key_from(const SubtitleFileInfo &file) -> QString
{ return _JsonToString(file.toJson()); }
template<>
inline auto json_key_to(const QString &key) -> SubtitleFileInfo
{ return SubtitleFileInfo::fromJson(_JsonFromString(key)); }

#define JSON_CLASS SubtitleStateInfo
static const auto stateIO = JIO(
    JE(track),
    JE(mpv),
    JE(cmplayer, &SubtitleStateInfo::setCMPlayer)
);
#undef JSON_CLASS

auto SubtitleStateInfo::toJson() const -> QJsonObject
{
    return stateIO.toJson(*this);
}

auto SubtitleStateInfo::fromJson(const QJsonObject &json) -> SubtitleStateInfo
{
    SubtitleStateInfo info;
    if (stateIO.fromJson(info, json))
        return info;
    return SubtitleStateInfo();
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

auto SubtitleStateInfo::load() const -> QVector<SubComp>
{
    auto selected = [] (const QVector<Comp> &list, int id) {
        for (auto &c : list) { if (c.id == id) return c.selected; }
        return false;
    };
    QVector<SubComp> loaded;
    Subtitle sub;
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
