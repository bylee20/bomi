#ifndef SUBMISC_HPP
#define SUBMISC_HPP

#include "stdafx.hpp"

class SubComp;

struct JsonKeyValue {
    JsonKeyValue(const QString &key, const QJsonValue &value): key(key), value(value) {}
    JsonKeyValue(const char *key, const QJsonValue &value): key(_L(key)), value(value) {}
    JsonKeyValue(const char *key, const char *value): key(_L(key)), value(_L(value)) {}
    QString key; QJsonValue value;
};

static inline QJsonObject _MakeJson(std::initializer_list<JsonKeyValue> &&list) {
    QJsonObject json;
    for (auto &item : list)
        json.insert(item.key, item.value);
    return json;
}

struct SubtitleFileInfo {
    SubtitleFileInfo() {}
    SubtitleFileInfo(const QString &path, const QString &encoding): path(path), encoding(encoding) {}
    bool operator == (const SubtitleFileInfo &rhs) const { return path == rhs.path && encoding == rhs.encoding; }
    bool operator < (const SubtitleFileInfo &rhs) const { return path < rhs.path; }
    QString toString() const { return path % _L('#') % encoding; }
    static SubtitleFileInfo fromString(const QString &text) {
        const int index = text.lastIndexOf(_L('#'));
        if (index < 0)
            return SubtitleFileInfo();
        return SubtitleFileInfo(text.mid(0, index), text.mid(index+1));
    }
    QJsonObject toJson() const { return _MakeJson({{"path", path}, {"encoding", encoding}}); }
    static SubtitleFileInfo fromJson(const QJsonObject &json) {
        auto path = json.value("path");
        auto encoding = json.value("encoding");
        if (path.isUndefined() || encoding.isUndefined())
            return SubtitleFileInfo();
        return SubtitleFileInfo(path.toString(), encoding.toString());
    }
    QString path, encoding;
};

struct SubtitleStateInfo {
    static const int InvalidTrack = -100;
    struct Comp {
        Comp() {}
        Comp(int id, bool selected): id(id), selected(selected) {}
        bool operator == (const Comp &rhs) const { return id == rhs.id && selected == rhs.selected; }
        int id = -1; bool selected = false;
    };
    bool operator == (const SubtitleStateInfo &rhs) const {
        return m_track == rhs.m_track && m_mpv == rhs.m_mpv && m_cmplayer == rhs.m_cmplayer;
    }
    bool operator != (const SubtitleStateInfo &rhs) const { return !operator == (rhs); }
    QString toString() const;
    QJsonObject toJson() const;
    static SubtitleStateInfo fromJson(const QJsonObject &json);
    static SubtitleStateInfo fromJson(const QString &str) {
        QJsonParseError error;
        auto json = QJsonDocument::fromJson(str.toUtf8(), &error).object();
        if (error.error != QJsonParseError::NoError)
            return SubtitleStateInfo();
        return fromJson(json);
    }
    static SubtitleStateInfo fromString(const QString &str);
    void append(const SubComp &comp);
    bool isValid() const { return m_track != InvalidTrack; }
    const QMap<SubtitleFileInfo, QList<Comp>> &cmplayer() const { return m_cmplayer; }
    const QList<SubtitleFileInfo> &mpv() const { return m_mpv; }
    QList<SubtitleFileInfo> &mpv() { return m_mpv; }
    int track() const { return m_track; }
    int &track() { return m_track; }
    QList<SubComp> load() const;
private:
    int m_track = InvalidTrack;
    QList<SubtitleFileInfo> m_mpv;
    QMap<SubtitleFileInfo, QList<Comp>> m_cmplayer;
};

Q_DECLARE_METATYPE(SubtitleStateInfo)

#endif // SUBMISC_HPP
