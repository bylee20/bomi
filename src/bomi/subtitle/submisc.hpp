#ifndef SUBMISC_HPP
#define SUBMISC_HPP

//class SubComp;

//struct SubtitleFileInfo {
//    SubtitleFileInfo() {}
//    SubtitleFileInfo(const QString &path, const QString &encoding)
//        : path(path), encoding(encoding) {}
//    auto operator == (const SubtitleFileInfo &rhs) const -> bool
//        { return path == rhs.path && encoding == rhs.encoding; }
//    auto operator < (const SubtitleFileInfo &rhs) const -> bool
//        { return path < rhs.path; }
//    auto toString() const -> QString { return path % '#'_q % encoding; }
//    static auto fromString(const QString &text) -> SubtitleFileInfo {
//        const int index = text.lastIndexOf('#'_q);
//        if (index < 0)
//            return SubtitleFileInfo();
//        return SubtitleFileInfo(text.mid(0, index), text.mid(index+1));
//    }
//    auto toJson() const -> QJsonObject;
//    static auto fromJson(const QJsonObject &json) -> SubtitleFileInfo;
//    QString path, encoding;
//};

//template<class T> struct JsonIO;

//struct SubtitleStateInfo {
//    struct Comp {
//        Comp() {}
//        Comp(int id, bool selected): id(id), selected(selected) {}
//        auto operator == (const Comp &rhs) const -> bool
//            { return id == rhs.id && selected == rhs.selected; }
//        int id = -1; bool selected = false;
//    };
//    auto operator == (const SubtitleStateInfo &rhs) const -> bool
//        { return m_track == rhs.m_track && m_mpv == rhs.m_mpv && m_bomi == rhs.m_bomi; }
//    auto operator != (const SubtitleStateInfo &rhs) const -> bool
//        { return !operator == (rhs); }
//    auto toString() const -> QString;
//    auto toJson() const -> QJsonObject;
//    auto setFromJson(const QJsonObject &json) -> bool;
//    static auto fromJson(const QJsonObject &json) -> SubtitleStateInfo;
//    static auto fromJson(const QString &str) -> SubtitleStateInfo {
//        QJsonParseError error;
//        auto json = QJsonDocument::fromJson(str.toUtf8(), &error).object();
//        if (error.error != QJsonParseError::NoError)
//            return SubtitleStateInfo();
//        return fromJson(json);
//    }
//    static auto fromString(const QString &str) -> SubtitleStateInfo;
//    auto append(const SubComp &comp) -> void;
//    auto bomi() const -> const QMap<SubtitleFileInfo, QVector<Comp>>&
//        { return m_bomi; }
//    auto mpv() const -> const QVector<SubtitleFileInfo>& { return m_mpv; }
//    auto mpv() -> QVector<SubtitleFileInfo>& { return m_mpv; }
//    auto setTrack(int track) -> void { m_track = track; m_valid = true; }
//    auto getTrack() const -> int { return m_track; }
//    auto load() const -> QVector<SubComp>;
//    auto isValid() const -> bool { return m_valid; }
//private:
//    bool m_valid = false;
//    int m_track = 0;
//    QVector<SubtitleFileInfo> m_mpv;
//    QMap<SubtitleFileInfo, QVector<Comp>> m_bomi;
//    friend struct JsonIO<SubtitleStateInfo>;
//};

//Q_DECLARE_METATYPE(SubtitleStateInfo)

#endif // SUBMISC_HPP
