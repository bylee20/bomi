#ifndef MEDIAMISC_HPP
#define MEDIAMISC_HPP

#include "mrl.hpp"
#include "enum/colorrange.hpp"
#include "enum/colorspace.hpp"

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

class CodecInfoObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString family READ family NOTIFY familyChanged)
    Q_PROPERTY(QString type READ type NOTIFY typeChanged)
    Q_PROPERTY(QString description READ description NOTIFY descriptionChanged)
public:
    auto parse(const QString &info) -> void;
    auto family() const -> QString { return m_family.toUpper(); }
    auto type() const -> QString { return m_type; }
    auto description() const -> QString { return m_desc; }
signals:
    void familyChanged();
    void typeChanged();
    void descriptionChanged();
private:
    QString m_family, m_type, m_desc;
};

class AvCommonFormatObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString type READ type NOTIFY typeChanged)
    Q_PROPERTY(int bitrate READ bitrate NOTIFY bitrateChanged)
    Q_PROPERTY(int depth READ depth NOTIFY depthChanged)
public:
    auto type() const -> QString { return m_type; }
    auto bitrate() const -> int { return m_bitrate; }
    auto setType(const QString &type) -> void
        { if (_Change(m_type, type.toUpper())) emit typeChanged(); }
    auto setBitrate(int bps) -> void
        { if (_Change(m_bitrate, bps)) emit bitrateChanged(); }
    auto depth() const -> int { return m_depth; }
    auto setDepth(int depth) -> void
        { if (_Change(m_depth, depth)) emit depthChanged(); }
signals:
    void typeChanged();
    void bitrateChanged();
    void depthChanged();
private:
    QString m_type;
    int m_bitrate = 0, m_depth = 0;
};

class AudioFormat;

class AudioFormatInfoObject : public AvCommonFormatObject {
    Q_OBJECT
    Q_PROPERTY(int samplerate READ samplerate NOTIFY samplerateChanged)
    Q_PROPERTY(QString channels READ channels NOTIFY channelsChanged)
public:
    auto samplerate() const -> int { return m_srate; }
    auto channels() const -> QString { return m_channels; }
    auto setFormat(const AudioFormat &format) -> void;
    auto setSampleRate(int s, bool bitrate) -> void
    {
        if (_Change(m_srate, s)) {
            emit samplerateChanged();
            if (bitrate)
                setBitrate(m_srate * m_nch * depth());
        }
    }
    auto setChannels(const QString &ch, int n) -> void
        { if (_Change(m_channels, ch) | _Change(m_nch, n)) emit channelsChanged(); }
signals:
    void samplerateChanged();
    void channelsChanged();
private:
    int m_srate = 0, m_nch = 0;
    QString m_channels;
};

class VideoHwAccInfoObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(int state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString driver READ driver NOTIFY driverChanged)
public:
    auto state() const -> int { return m_state; }
    auto driver() const -> QString { return m_driver; }
    auto setDriver(const QString &driver) -> void
    { if (_Change(m_driver, driver.toUpper())) emit driverChanged(); }
    auto setState(int state) -> void
    { if (_Change(m_state, state)) emit stateChanged(); }
signals:
    void stateChanged();
    void driverChanged();
private:
    int m_state = 0;
    QString m_driver;
};

class VideoFormatInfoObject : public AvCommonFormatObject {
    Q_OBJECT
    Q_PROPERTY(qreal fps READ fps NOTIFY fpsChanged)
    Q_PROPERTY(QString space READ spaceText NOTIFY spaceChanged)
    Q_PROPERTY(QString range READ rangeText NOTIFY rangeChanged)
    Q_PROPERTY(QSize size READ size NOTIFY sizeChanged)
    Q_PROPERTY(int bpp READ bpp NOTIFY bppChanged)
public:
    auto bpp() const -> int { return m_bpp; }
    auto fps() const -> qreal { return m_fps; }
    auto space() const -> ColorSpace { return m_space; }
    auto range() const -> ColorRange { return m_range; }
    auto spaceText() const -> QString;
    auto rangeText() const -> QString;
    auto setSpace(ColorSpace space) -> void
        { if (_Change(m_space, space)) emit spaceChanged(); }
    auto setRange(ColorRange range) -> void
        { if (_Change(m_range, range)) emit rangeChanged(); }
    auto setImgFmt(int imgfmt) -> void;
    auto size() const -> QSize { return m_size; }
    auto width() const -> int { return m_size.width(); }
    auto height() const -> int { return m_size.height(); }
    auto setWidth(int w) -> void { setSize({w, height()}); }
    auto setHeight(int h) -> void { setSize({width(), h}); }
    auto setSize(const QSize &s) -> void
        { if (_Change(m_size, s)) emit sizeChanged(); }
    auto setBppSize(const QSize &s) -> void
        { if (_Change(m_bppSize, s)) updateBitrate(); }
public slots:
    void setFps(double fps)
        { if (_Change(m_fps, fps)) { emit fpsChanged(); updateBitrate(); } }
signals:
    void fpsChanged();
    void spaceChanged();
    void rangeChanged();
    void bppChanged();
    void sizeChanged();
private:
    auto updateBitrate() -> void
        { setBitrate(m_bpp * width() * height() * m_fps); }
    int m_bpp = 0; double m_fps = 0;
    QSize m_bppSize, m_size;
    ColorSpace m_space = ColorSpace::Auto;
    ColorRange m_range = ColorRange::Auto;
};

class AvCommonInfoObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(CodecInfoObject *codec READ codec CONSTANT FINAL)
    Q_PROPERTY(int track READ track NOTIFY trackChanged)
public:
    auto codec() const -> const CodecInfoObject* { return &m_codec; }
    auto codec() -> CodecInfoObject* { return &m_codec; }
    auto track() const -> int {return m_track;}
public slots:
    void setTrack(int t) { if (_Change(m_track, t)) emit trackChanged(); }
signals:
    void trackChanged();
private:
    CodecInfoObject m_codec;
    int m_track = 0;
};

class VideoInfoObject : public AvCommonInfoObject {
    Q_OBJECT
    Q_PROPERTY(VideoFormatInfoObject *input READ input CONSTANT FINAL)
    Q_PROPERTY(VideoFormatInfoObject *output READ output CONSTANT FINAL)
    Q_PROPERTY(VideoFormatInfoObject *renderer READ renderer CONSTANT FINAL)
    Q_PROPERTY(VideoHwAccInfoObject *hwacc READ hwacc CONSTANT FINAL)
    Q_PROPERTY(int deinterlacer READ deinterlacer NOTIFY deinterlacerChanged)
    Q_PROPERTY(int droppedFrames READ droppedFrames NOTIFY droppedFramesChanged)
    Q_PROPERTY(int delayedFrames READ delayedFrames NOTIFY delayedFramesChanged)
public:
    auto input() const -> const VideoFormatInfoObject* { return &m_input; }
    auto renderer() const -> const VideoFormatInfoObject* { return &m_renderer; }
    auto output() const -> const VideoFormatInfoObject* { return &m_output; }
    auto input() -> VideoFormatInfoObject* { return &m_input; }
    auto renderer() -> VideoFormatInfoObject* { return &m_renderer; }
    auto output() -> VideoFormatInfoObject* { return &m_output; }
    auto hwacc() -> VideoHwAccInfoObject* { return &m_hwacc; }
    auto hwacc() const -> const VideoHwAccInfoObject* { return &m_hwacc; }
    auto deinterlacer() const -> int { return m_deint; }
    auto setDeinterlacer(int deint) -> void
        { if (_Change(m_deint, deint)) emit deinterlacerChanged(); }
    auto droppedFrames() const -> int { return m_dropped; }
    auto delayedFrames() const -> int { return m_delayed; }
public slots:
    void setDroppedFrames(int f)
        { if (_Change(m_dropped, f)) emit droppedFramesChanged(); }
    void setDelayedFrames(int f)
        { if (_Change(m_delayed, f)) emit delayedFramesChanged(); }
signals:
    void deinterlacerChanged();
    void droppedFramesChanged();
    void delayedFramesChanged();
private:
    VideoFormatInfoObject m_input, m_output, m_renderer;
    VideoHwAccInfoObject m_hwacc;
    int m_deint = 0, m_dropped = 0, m_delayed = 0;
};

class AudioInfoObject : public AvCommonInfoObject {
    Q_OBJECT
    Q_PROPERTY(AudioFormatInfoObject *input READ input CONSTANT FINAL)
    Q_PROPERTY(AudioFormatInfoObject *output READ output CONSTANT FINAL)
    Q_PROPERTY(AudioFormatInfoObject *renderer READ renderer CONSTANT FINAL)
    Q_PROPERTY(double normalizer READ normalizer NOTIFY normalizerChanged)
    Q_PROPERTY(QString driver READ driver NOTIFY driverChanged)
    Q_PROPERTY(QString device READ device NOTIFY deviceChanged)
public:
    auto input() const -> const AudioFormatInfoObject* { return &m_input; }
    auto output() const -> const AudioFormatInfoObject* { return &m_output; }
    auto renderer() const -> const AudioFormatInfoObject* { return &m_renderer; }
    auto input() -> AudioFormatInfoObject* { return &m_input; }
    auto output() -> AudioFormatInfoObject* { return &m_output; }
    auto renderer() -> AudioFormatInfoObject* { return &m_renderer; }
    auto normalizer() const -> double { return m_gain; }
    auto setNormalizer(double gain) -> void
        { if (_Change(m_gain, gain)) emit normalizerChanged(); }
    auto device() const -> QString;
    auto driver() const -> QString { return m_driver.toUpper(); }
public slots:
    void setDriver(const QString &driver);
    void setDevice(const QString &device);
signals:
    void normalizerChanged();
    void driverChanged();
    void deviceChanged();
private:
    AudioFormatInfoObject m_input, m_output, m_renderer;
    double m_gain = -1.0;
    QString m_driver, m_device;
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
