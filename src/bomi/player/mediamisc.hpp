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

class MediaObject : public QObject {
    Q_OBJECT
    Q_ENUMS(Type)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString typeText READ typeText NOTIFY typeChanged)
    Q_PROPERTY(Type type READ type NOTIFY typeChanged)
public:
    enum Type{ NoMedia, File, Url, Dvd, Bluray };
    MediaObject(QObject *parent = nullptr): QObject(parent) {}
    auto name() const -> QString { return m_name; }
    auto type() const -> Type { return m_type; }
    auto typeText() const -> QString;
    auto setName(const QString &name) -> void
        { if (_Change(m_name, name)) emit nameChanged(m_name); }
    auto setType(Type type) -> void
        { if (_Change(m_type, type)) emit typeChanged(m_type); }
signals:
    void nameChanged(const QString &name);
    void typeChanged(Type type);
private:
    QString m_name;
    Type m_type = NoMedia;
};

class StreamingFormatObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT FINAL)
    Q_PROPERTY(QString id READ id CONSTANT FINAL)
public:
    StreamingFormatObject() = default;
    StreamingFormatObject(const QString &id, const QString &name)
        : m_id(id), m_name(name) { }
    auto name() const -> QString { return m_name; }
    auto id() const -> QString { return m_id; }
    auto set(const QString &id, const QString &name) { m_id = id; m_name = name; }
private:
    QString m_id, m_name;
};

class EditionChapterObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(int number READ number CONSTANT FINAL)
    Q_PROPERTY(int time READ time CONSTANT FINAL)
    Q_PROPERTY(QString name READ name CONSTANT FINAL)
    Q_PROPERTY(qreal rate READ rate NOTIFY rateChanged)
public:
    struct Data { int number = -2, time = 0; qreal rate = 0.0; QString name; };
    EditionChapterObject() = default;
    ~EditionChapterObject();
    EditionChapterObject(const Data &d): m(d) { }
    EditionChapterObject(Data &&d): m(std::move(d)) { }
    auto number() const -> int { return m.number; }
    auto time() const -> int { return m.time; }
    auto name() const -> QString { return m.name; }
    auto rate() const -> qreal { return m.rate; }
    auto isValid() const -> bool { return m.number > -2; }
signals:
    void rateChanged();
private:
    auto set(const Data &d) -> void { m = d; emit rateChanged(); }
    auto setRate(qreal rate) -> void;
    auto invalidate() -> void { setRate(0); m = Data(); }
    friend class PlayEngine;
    Data m;
};

using EditionObject = EditionChapterObject;
using ChapterObject = EditionChapterObject;
using EditionData = EditionObject::Data;
using ChapterData = ChapterObject::Data;
using EditionChapterData = EditionChapterObject::Data;
//using EditionPtr = QSharedPointer<EditionObject>;
//using ChapterPtr = QSharedPointer<ChapterObject>;
using EditionChapterPtr = QSharedPointer<EditionChapterObject>;

class CacheInfoObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(int size READ size NOTIFY sizeChanged)
    Q_PROPERTY(int used READ used NOTIFY usedChanged)
    Q_PROPERTY(int time READ time NOTIFY timeChanged)
public:
    auto size() const -> int { return m_size; }
    auto used() const -> int { return m_used; }
    auto time() const -> int { return m_time; }
signals:
    void sizeChanged(int size);
    void usedChanged(int used);
    void timeChanged(int time);
private:
    friend class PlayEngine;
    auto setSize(int s) -> void { if (_Change(m_size, s)) emit sizeChanged(s); }
    auto setUsed(int s) -> void { if (_Change(m_used, s)) emit usedChanged(s); }
    Q_INVOKABLE void setTime(int s)
        { if (_Change(m_time, s)) emit timeChanged(s); }
    int m_size = 0, m_used = 0, m_time = 0;
};

#endif // MEDIAMISC_HPP
