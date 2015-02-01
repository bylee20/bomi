#ifndef STREAMTRACK_HPP
#define STREAMTRACK_HPP

enum StreamType { StreamAudio = 0, StreamVideo, StreamSubtitle, StreamInclusiveSubtitle, StreamUnknown };

class SubComp;

class StreamTrack {
public:
    auto name() const -> QString;
    auto id() const -> int {return m_id;}
    auto operator == (const StreamTrack &rhs) const -> bool
        { return compareMetaData(rhs) && m_selected == rhs.m_selected; }
    auto compareMetaData(const StreamTrack &rhs) const -> bool
    {
        return m_title == rhs.m_title && m_lang == rhs.m_lang
               && m_codec == rhs.m_codec && m_id == rhs.m_id
               && m_type == rhs.m_type;
    }
    auto isSelected() const -> bool { return m_selected; }
    auto encoding() const { return m_encoding; }
    auto codec() const -> const QString& { return m_codec; }
    auto title() const -> const QString& { return m_title; }
    auto type() const -> StreamType { return m_type; }
    auto isExternal() const -> bool { return !m_file.isEmpty(); }
    auto isDefault() const -> bool { return m_default; }
    auto isAlbumArt() const -> bool { return m_albumart; }
    auto language() const -> QString { return m_lang; }
    auto isExclusive() const -> bool {
        if (m_type != StreamSubtitle)
            return true;
        return !isExternal() || m_file.endsWith(u".ass"_q, Qt::CaseInsensitive);
    }
    auto isInclusive() const -> bool { return !isExclusive(); }
    auto isExternalByMpv() const -> bool { return isExclusive() && isExternal(); }
    auto file() const -> QString { return m_file; }
    static auto fromMpvData(const QVariant &mpv) -> StreamTrack;
    static auto fromSubComp(const SubComp &comp) -> StreamTrack;
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
    static auto fromJson(const QJsonObject &json) -> StreamTrack;
private:
    friend class PlayEngine;
    friend class StreamList;
    StreamType m_type = StreamUnknown;
    int m_id = -1;
    QString m_title, m_lang, m_file, m_codec, m_displayLang, m_encoding;
    bool m_selected = false, m_default = false, m_albumart = false;
};

Q_DECLARE_METATYPE(StreamTrack);

class StreamList {
    using List = QVector<StreamTrack>;
public:
    StreamList() { }
    StreamList(StreamType type): m_type(type) { }
    using iterator = List::iterator;
    using const_iterator = List::const_iterator;
    auto operator == (const StreamList &rhs) const -> bool
        { return m_type == rhs.m_type && m_tracks == rhs.m_tracks; }
    auto operator != (const StreamList &rhs) const -> bool
        { return !operator == (rhs); }
    auto operator += (const StreamList &rhs) -> StreamList&
        { Q_ASSERT(m_type == rhs.m_type); m_tracks += rhs.m_tracks; return *this; }
    auto operator + (const StreamList &rhs) const -> StreamList
        { StreamList l(*this); return l += rhs; }
    auto contains(int id) const -> bool { return track(id); }
    auto track(int id) const -> const StreamTrack*
        { return const_cast<StreamList*>(this)->track(id); }
    auto selection() const -> const StreamTrack*;
    auto selectionId() const -> int
        { auto s = selection(); return s ? s->id() : -1; }
    auto insert(const StreamTrack &track) -> void;
    auto erase(iterator it) -> iterator { return m_tracks.erase(it); }
    auto take(iterator it) -> StreamTrack {
        StreamTrack track = std::move(*it);
        m_tracks.erase(it); return track;
    }
    auto type() const -> StreamType { return (StreamType)m_type; }
    auto fileIds() const -> QList<int>;
    auto deselect(int id) -> bool;
    auto select(int id) -> bool;
    auto isEmpty() const { return m_tracks.isEmpty(); }
    auto size() const { return m_tracks.size(); }
    auto begin() const { return m_tracks.begin(); }
    auto begin() { return m_tracks.begin(); }
    auto end() const { return m_tracks.end(); }
    auto end() { return m_tracks.end(); }
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
    static auto fromJson(const QJsonObject &json) -> StreamList
        { StreamList s; s.setFromJson(json); return s; }
    auto isValid() const -> bool { return m_type != StreamUnknown; }
private:
    auto track(int id) -> StreamTrack*;
    StreamType m_type = StreamUnknown;
    List m_tracks;
};

Q_DECLARE_METATYPE(StreamList);

#endif // STREAMTRACK_HPP
