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

class MediaInfoObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
public:
    MediaInfoObject(QObject *parent = nullptr): QObject(parent) {}
    auto name() const -> QString {return m_name;}
    auto setName(const QString &name) -> void
        { if (_Change(m_name, name)) emit nameChanged(m_name); }
signals:
    void nameChanged(const QString &name);
private:
    QString m_name;
};

class EditionChapterObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(int number READ number CONSTANT FINAL)
    Q_PROPERTY(int time READ time CONSTANT FINAL)
    Q_PROPERTY(QString name READ name CONSTANT FINAL)
    Q_PROPERTY(qreal rate READ rate NOTIFY rateChanged)
public:
    auto number() const -> int { return m.number; }
    auto time() const -> int { return m.time; }
    auto name() const -> QString { return m.name; }
    auto rate() const -> qreal { return m.rate; }
    auto isValid() const -> bool { return m.number > -2; }
signals:
    void rateChanged();
private:
    auto setRate(qreal rate) -> void
        { if (_Change(m.rate, rate)) emit rateChanged(); }
    auto copyFrom(const EditionChapterObject *rhs) -> void { m = rhs->m; }
    auto invalidate() -> void { setRate(0); m = M(); }
    friend class PlayEngine;
    struct M {
        int number = -2, time = 0;
        qreal rate = 0.0;
        QString name;
    }; M m;
};

#endif // MEDIAMISC_HPP
