#ifndef STREAMTRACK_HPP
#define STREAMTRACK_HPP

enum StreamType { StreamAudio = 0, StreamVideo, StreamSubtitle, StreamUnknown };

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
    auto codec() const -> const QString& { return m_codec; }
    auto title() const -> const QString& { return m_title; }
    auto type() const -> StreamType { return m_type; }
    auto isExternal() const -> bool { return !m_fileName.isEmpty(); }
    auto isDefault() const -> bool { return m_default; }
    auto isAlbumArt() const -> bool { return m_albumart; }
    auto language() const -> QString { return m_lang; }
    static auto fromMpvData(const QVariant &mpv) -> StreamTrack;
private:
    friend class MpMessage;
    friend class PlayEngine;
    StreamType m_type = StreamUnknown;
    int m_id = -1;
    QString m_title, m_lang, m_fileName, m_codec, m_displayLang;
    bool m_selected = false, m_default = false, m_albumart = false;
};

using StreamList = QMap<int, StreamTrack>;

#endif // STREAMTRACK_HPP
