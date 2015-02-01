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

class ChapterInfoObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(int id READ id NOTIFY idChanged)
    Q_PROPERTY(int time READ time NOTIFY timeChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(qreal rate READ rate NOTIFY rateChanged)
public:
    ChapterInfoObject(QObject *parent = nullptr): QObject(parent) { }
    auto id() const -> int { return m_id; }
    auto time() const -> int { return m_time; }
    auto name() const -> QString { return m_name; }
    auto rate() const -> qreal { return m_rate; }
signals:
    void idChanged();
    void timeChanged();
    void nameChanged();
    void rateChanged();
private:
    auto setRate(qreal rate) -> void
        { if (_Change(m_rate, rate)) emit rateChanged(); }
    auto update() -> void
    {
        emit idChanged();
        emit timeChanged();
        emit nameChanged();
    }
    friend class PlayEngine;
    int m_id = -1;
    int m_time = 0;
    qreal m_rate = 0.0;
    QString m_name;
};

#endif // MEDIAMISC_HPP
