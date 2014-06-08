#ifndef MEDIAMISC_HPP
#define MEDIAMISC_HPP

#include "mrl.hpp"

class PlayEngine;

class MetaData {
public:
    auto title() const -> QString { return m_title; }
    auto artist() const -> QString { return m_artist; }
    auto album() const -> QString { return m_album; }
    auto genre() const -> QString { return m_genre; }
    auto date() const -> QString { return m_date; }
    auto mrl() const -> Mrl { return m_mrl; }
    auto duration() const -> int { return m_duration; }
private:
    friend class PlayEngine;
    QString m_title, m_artist, m_album, m_genre, m_date;
    Mrl m_mrl;
    int m_duration = 0;
};

class AvIoFormat : public QObject {
    Q_OBJECT
    Q_PROPERTY(QSize size READ size CONSTANT FINAL)
    Q_PROPERTY(QString type READ type CONSTANT FINAL)
    Q_PROPERTY(int bitrate READ bitrate NOTIFY bitrateChanged)
    Q_PROPERTY(double fps READ fps CONSTANT FINAL)
    Q_PROPERTY(int bits READ bits CONSTANT FINAL)
    Q_PROPERTY(double samplerate READ samplerate CONSTANT FINAL)
    Q_PROPERTY(QString channels READ channels CONSTANT FINAL)
public:
    AvIoFormat(QObject *parent = nullptr): QObject(parent) {}
    auto size() const -> QSize {return m_size;}
    auto samplerate() const -> double {return m_samplerate;}
    auto bits() const -> int {return m_bits;}
    auto channels() const -> QString {return m_channels;}
    auto type() const -> QString {return m_type;}
    auto bitrate() const -> int {return m_bitrate;}
    auto fps() const -> double {return m_fps;}
signals:
    void bitrateChanged();
private:
    auto setBps(int bitrate) -> void
    { if (_Change(m_bitrate, bitrate)) emit bitrateChanged(); }
    friend class AvInfoObject;
    friend class PlayEngine;
    QSize m_size;
    QString m_type;
    int m_bitrate = 0;
    double m_fps = 0.0;
    double &m_samplerate = m_fps;
    int &m_bits = m_size.rwidth();
    QString m_channels;
};

class AvInfoObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString codec READ codecDescription CONSTANT FINAL)
    Q_PROPERTY(AvIoFormat *input READ input CONSTANT FINAL)
    Q_PROPERTY(AvIoFormat *output READ output CONSTANT FINAL)
    Q_PROPERTY(QString driver READ driver CONSTANT FINAL)
    Q_PROPERTY(QString hwacc READ driver CONSTANT FINAL)
public:
    AvInfoObject(QObject *parent = nullptr);
    auto codecDescription() const -> QString {return m_codecDescription;}
    auto codec() const -> QString { return m_codec; }
    AvIoFormat *input() const {return m_input;}
    AvIoFormat *output() const {return m_output;}
    auto driver() const -> QString { return m_driver; }
private:
    auto setHwAcc(int acc) -> void;
    AvIoFormat *m_input = new AvIoFormat(this);
    AvIoFormat *m_output = new AvIoFormat(this);
    QString m_codecDescription, m_hwacc, &m_driver = m_hwacc, m_codec;
    friend class PlayEngine;
};

class MediaInfoObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
public:
    MediaInfoObject(QObject *parent = nullptr): QObject(parent) {}
    auto name() const -> QString {return m_name;}
    auto setName(const QString &name) -> void { if (_Change(m_name, name)) emit nameChanged(m_name); }
signals:
    void nameChanged(const QString &name);
private:
    QString m_name;
};

struct Stream {
    enum Type {Audio = 0, Video, Subtitle, Unknown};
    auto name() const -> QString
    {
        QString name = m_title;
        if (!m_lang.isEmpty())
            name += name.isEmpty() ? m_lang : " ("_a % m_lang % ")"_a;
        return name;
    }
    auto id() const -> int {return m_id;}
    auto operator == (const Stream &rhs) const -> bool
    {
        return m_title == rhs.m_title && m_lang == rhs.m_lang
               && m_codec == rhs.m_codec && m_id == rhs.m_id
               && m_type == rhs.m_type && m_selected == rhs.m_selected;
    }
    auto isSelected() const -> bool { return m_selected; }
    auto codec() const -> const QString& { return m_codec; }
    auto title() const -> const QString& { return m_title; }
    auto type() const -> Type { return m_type; }
    auto isExternal() const -> bool { return !m_fileName.isEmpty(); }
    auto isDefault() const -> bool { return m_default; }
    auto isAlbumArt() const -> bool { return m_albumart; }
private:
    friend class MpMessage;
    friend class PlayEngine;
    Type m_type = Unknown;
    int m_id = -1;
    QString m_title, m_lang, m_fileName, m_codec;
    bool m_selected = false, m_default = false, m_albumart = false;
};

using StreamList = QMap<int, Stream>;

struct Chapter {
    auto time() const -> int { return m_time; }
    auto name() const -> QString {return m_name;}
    auto id() const -> int {return m_id;}
    auto operator == (const Chapter &rhs) const -> bool
    { return m_id == rhs.m_id && m_name == rhs.m_name; }
private:
    friend class PlayEngine;
    QString m_name;
    int m_id = -2, m_time = 0;
};

using ChapterList = QVector<Chapter>;

struct Edition {
    auto name() const -> QString { return m_name; }
    auto id() const -> int { return m_id; }
    auto isSelected() const -> bool { return m_selected; }
    auto operator == (const Edition &rhs) const -> bool
    { return m_id == rhs.m_id && m_selected == rhs.m_selected; }
private:
    friend class PlayEngine;
    int m_id = 0;
    QString m_name;
    bool m_selected = false;
};

using EditionList = QVector<Edition>;

class TrackInfoObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(int current READ current NOTIFY currentChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int length READ count NOTIFY countChanged)
    Q_PROPERTY(QString currentText READ currentText NOTIFY currentChanged)
    Q_PROPERTY(QString countText READ countText NOTIFY countChanged)
public:
    TrackInfoObject(QObject *parent = nullptr): QObject(parent) {}
    auto current() const -> int { return m_current; }
    auto count() const -> int { return m_count; }
    auto setCount(int count) -> void
    { if (_Change(m_count, count)) emit countChanged(); }
    auto setCurrent(int current) -> void
    { if (_Change(m_current, current)) emit currentChanged(); }
    auto currentText() const -> QString { return toString(m_current); }
    auto countText() const -> QString { return toString(m_count); }
signals:
    void currentChanged();
    void countChanged();
private:
    static auto toString(int i) -> QString
    { return i < 1 ? u"-"_q : QString::number(i); }
    int m_current = -2;
    int m_count = 0;
};

class ChapterInfoObject : public TrackInfoObject {
    Q_OBJECT
public:
    ChapterInfoObject(const PlayEngine *engine, QObject *parent = nullptr);
    Q_INVOKABLE int time(int i) const;
    Q_INVOKABLE QString name(int i) const;
private:
    const PlayEngine *m_engine = nullptr;
};

class AudioTrackInfoObject : public TrackInfoObject {
    Q_OBJECT
public:
    AudioTrackInfoObject(const PlayEngine *engine, QObject *parent = nullptr);
};

class SubtitleTrackInfoObject : public TrackInfoObject {
    Q_OBJECT
public:
    SubtitleTrackInfoObject(QObject *parent = nullptr): TrackInfoObject(parent) {}
    auto set(const QStringList &tracks) -> void { m_tracks = tracks; setCount(m_tracks.size()); }
    auto setCurrentIndex(int idx) -> void { setCurrent(idx+1); }
    Q_INVOKABLE QString name(int i) const { return m_tracks.value(i); }
private:
    QStringList m_tracks;
};

#endif // MEDIAMISC_HPP
