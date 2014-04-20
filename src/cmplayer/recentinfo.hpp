#ifndef RECENTINFO_HPP
#define RECENTINFO_HPP

#include "stdafx.hpp"
#include "mrl.hpp"

class Playlist;        class QDateTime;
class PlayEngine;

class RecentInfo : public QObject {
    Q_OBJECT
public:
    RecentInfo();
    ~RecentInfo();
    QList<Mrl> openList() const;
    void stack(const Mrl &mrl);
    void setLastPlaylist(const Playlist &list);
    void setLastMrl(const Mrl &mrl);
    Mrl lastMrl() const;
    Playlist lastPlaylist() const;
public slots:
    void clear();
signals:
    void openListChanged(const QList<Mrl> &list);
private:
    void save() const;
    void load();
    struct Data;
    Data *d;
};

#endif // RECENTINFO_HPP
