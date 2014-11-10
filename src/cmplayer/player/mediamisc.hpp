#ifndef MEDIAMISC_HPP
#define MEDIAMISC_HPP

#include "mrl.hpp"

class PlayEngine;

class MetaData {
public:
    auto operator == (const MetaData &rhs) const -> bool
    {
        return m_title == rhs.m_title && m_artist == rhs.m_artist
                && m_album == rhs.m_album && m_genre == rhs.m_genre
                && m_date == rhs.m_date && m_mrl == rhs.m_mrl
                && m_duration == rhs.m_duration;
    }
    auto operator != (const MetaData &rhs) const -> bool
        { return !operator == (rhs); }
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

struct CodecInfo {
    auto operator == (const CodecInfo &rhs) const -> bool
        { return name == rhs.name; }
    auto operator != (const CodecInfo &rhs) const -> bool
        { return !operator == (rhs); }
    QString name, description;
};

struct AudioInfo {
    struct Format {
        auto operator == (const Format &rhs) const -> bool
        {
            return samplerate == rhs.samplerate && bits == rhs.bits
                    && channels == rhs.channels && type == rhs.type
                    && bitrate == rhs.bitrate;
        }
        auto operator != (const Format &rhs) const -> bool
        { return !operator == (rhs); }
        double samplerate = 0;
        int bits = 0;
        QString channels;
        QString type;
        int bitrate = 0;
    };
    Format input, output;
    CodecInfo codec;
    QString device;
};

class AudioInfoFormatObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString type READ type CONSTANT FINAL)
    Q_PROPERTY(int bitrate READ bitrate CONSTANT FINAL)
    Q_PROPERTY(int bits READ bits CONSTANT FINAL)
    Q_PROPERTY(double samplerate READ samplerate CONSTANT FINAL)
    Q_PROPERTY(QString channels READ channels CONSTANT FINAL)
public:
    AudioInfoFormatObject(AudioInfo::Format *m, QObject *parent = nullptr)
        : QObject(parent), m(m) {}
    auto samplerate() const -> double {return m->samplerate;}
    auto bits() const -> int {return m->bits;}
    auto channels() const -> QString {return m->channels;}
    auto type() const -> QString {return m->type;}
    auto bitrate() const -> int {return m->bitrate;}
private:
    AudioInfo::Format *m = nullptr;
};

class AudioInfoObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString codec READ codecDescription NOTIFY codecChanged)
    Q_PROPERTY(QString device READ device NOTIFY deviceChanged)
    Q_PROPERTY(const AudioInfoFormatObject *input READ input NOTIFY inputChanged)
    Q_PROPERTY(const AudioInfoFormatObject *output READ output NOTIFY outputChanged)
public:
    AudioInfoObject(QObject *parent = nullptr)
        : QObject(parent)
        , m_info()
        , m_input(&m_info.input)
        , m_output(&m_info.output)
    { }
    auto codecDescription() const -> QString
        { return m_info.codec.description; }
    auto codec() const -> const CodecInfo& { return m_info.codec; }
    auto input() const -> const AudioInfoFormatObject* {return &m_input;}
    auto output() const -> const AudioInfoFormatObject* {return &m_output;}
    auto device() const -> QString { return m_info.device; }
    auto set(const AudioInfo &info) -> void
    {
        if (_Change(m_info.device, info.device))
            emit deviceChanged();
        if (_Change(m_info.codec, info.codec))
            emit codecChanged();
        if (_Change(m_info.input, info.input))
            emit inputChanged();
        if (_Change(m_info.output, info.output))
            emit outputChanged();
    }
signals:
    void codecChanged();
    void deviceChanged();
    void inputChanged();
    void outputChanged();
private:
    AudioInfo m_info;
    AudioInfoFormatObject m_input, m_output;
};

struct VideoInfo {
    struct Format {
        auto operator == (const Format &rhs) -> bool
        {
            return size == rhs.size && fps == rhs.fps
                    && bitrate == rhs.bitrate && type == rhs.type;
        }
        auto operator != (const Format &rhs) -> bool
        { return !operator == (rhs); }
        QSize size;
        double fps = 24;
        int bitrate = 0;
        QString type;
    };
    Format input, output;
    CodecInfo codec;
    bool hwacc = false;
};

class VideoInfoFormatObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QSize size READ size CONSTANT FINAL)
    Q_PROPERTY(QString type READ type CONSTANT FINAL)
    Q_PROPERTY(int bitrate READ bitrate CONSTANT FINAL)
    Q_PROPERTY(double fps READ fps CONSTANT FINAL)
public:
    VideoInfoFormatObject(VideoInfo::Format *m, QObject *parent = nullptr)
        : QObject(parent), m(m) { }
    auto size() const -> QSize { return m->size; }
    auto type() const -> QString { return m->type; }
    auto bitrate() const -> int { return m->bitrate; }
    auto fps() const -> double { return m->fps; }
private:
    friend class VideoInfoObject;
    VideoInfo::Format *m = nullptr;
};

class VideoInfoObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString codec READ codecDescription NOTIFY codecChanged)
    Q_PROPERTY(const VideoInfoFormatObject *input READ input NOTIFY inputChanged)
    Q_PROPERTY(const VideoInfoFormatObject *output READ output NOTIFY outputChanged)
public:
    VideoInfoObject(QObject *parent = nullptr)
        : QObject(parent)
        , m_info(), m_input(&m_info.input), m_output(&m_info.output){ }
    auto codecDescription() const -> QString
        { return m_info.codec.description; }
    auto codec() const -> const CodecInfo& { return m_info.codec; }
    auto input() const -> const VideoInfoFormatObject* {return &m_input;}
    auto output() const -> const VideoInfoFormatObject* {return &m_output;}
//    auto hwacc() const -> QString { return m_info.device; }
    auto set(const VideoInfo &info) -> void
    {
        if (_Change(m_info.hwacc, info.hwacc))
            emit hwaccChanged();
        if (_Change(m_info.codec, info.codec))
            emit codecChanged();
        if (_Change(m_info.input, info.input))
            emit inputChanged();
        if (_Change(m_info.output, info.output))
            emit outputChanged();
    }
signals:
    void codecChanged();
    void hwaccChanged();
    void inputChanged();
    void outputChanged();
private:
    auto setHwAcc(int acc) -> void;
    VideoInfo m_info;
    VideoInfoFormatObject m_input, m_output;
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
    Q_PROPERTY(QString device READ device CONSTANT FINAL)
    Q_PROPERTY(QString hwacc READ device CONSTANT FINAL)
public:
    AvInfoObject(QObject *parent = nullptr);
    auto codecDescription() const -> QString {return m_codecDescription;}
    auto codec() const -> QString { return m_codec; }
    AvIoFormat *input() const {return m_input;}
    AvIoFormat *output() const {return m_output;}
    auto device() const -> QString { return m_device; }
private:
    auto setHwAcc(int acc) -> void;
    AvIoFormat *m_input = new AvIoFormat(this);
    AvIoFormat *m_output = new AvIoFormat(this);
    QString m_codecDescription, m_hwacc, &m_device = m_hwacc, m_codec;
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
