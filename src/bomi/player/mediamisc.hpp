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

#endif // MEDIAMISC_HPP
