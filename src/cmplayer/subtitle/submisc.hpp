#ifndef SUBMISC_HPP
#define SUBMISC_HPP

#include "stdafx.hpp"

class SubComp;

struct SubtitleFileInfo {
    SubtitleFileInfo() {}
    SubtitleFileInfo(const QString &path, const QString &encoding): path(path), encoding(encoding) {}
    bool operator == (const SubtitleFileInfo &rhs) const { return path == rhs.path && encoding == rhs.encoding; }
    bool operator < (const SubtitleFileInfo &rhs) const { return path < rhs.path; }
    auto toString() const -> QString { return path % _L('#') % encoding; }
    static auto fromString(const QString &text) -> SubtitleFileInfo {
        const int index = text.lastIndexOf(_L('#'));
        if (index < 0)
            return SubtitleFileInfo();
        return SubtitleFileInfo(text.mid(0, index), text.mid(index+1));
    }
    auto toJson() const -> QJsonObject;
    static auto fromJson(const QJsonObject &json) -> SubtitleFileInfo;
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
    auto toString() const -> QString;
    auto toJson() const -> QJsonObject;
    static auto fromJson(const QJsonObject &json) -> SubtitleStateInfo;
    static auto fromJson(const QString &str) -> SubtitleStateInfo {
        QJsonParseError error;
        auto json = QJsonDocument::fromJson(str.toUtf8(), &error).object();
        if (error.error != QJsonParseError::NoError)
            return SubtitleStateInfo();
        return fromJson(json);
    }
    static auto fromString(const QString &str) -> SubtitleStateInfo;
    auto append(const SubComp &comp) -> void;
    auto isValid() const -> bool { return m_track != InvalidTrack; }
    const QMap<SubtitleFileInfo, QVector<Comp>> &cmplayer() const { return m_cmplayer; }
    void setCMPlayer(const QMap<SubtitleFileInfo, QVector<Comp>> &map) { m_cmplayer = map; }
    const QVector<SubtitleFileInfo> &mpv() const { return m_mpv; }
    QVector<SubtitleFileInfo> &mpv() { return m_mpv; }
    auto track() const -> int { return m_track; }
    int &track() { return m_track; }
    QVector<SubComp> load() const;
private:
    int m_track = InvalidTrack;
    QVector<SubtitleFileInfo> m_mpv;
    QMap<SubtitleFileInfo, QVector<Comp>> m_cmplayer;
};

Q_DECLARE_METATYPE(SubtitleStateInfo)

#endif // SUBMISC_HPP
