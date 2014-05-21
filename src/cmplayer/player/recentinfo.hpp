#ifndef RECENTINFO_HPP
#define RECENTINFO_HPP

#include "stdafx.hpp"

class Playlist;                         class Mrl;

class RecentInfo : public QObject {
    Q_OBJECT
public:
    RecentInfo();
    ~RecentInfo();
    auto openList() const -> QList<Mrl>;
    auto stack(const Mrl &mrl) -> void;
    auto setLastPlaylist(const Playlist &list) -> void;
    auto setLastMrl(const Mrl &mrl) -> void;
    auto lastMrl() const -> Mrl;
    auto lastPlaylist() const -> Playlist;
    auto clear() -> void;
signals:
    void openListChanged(const QList<Mrl> &list);
private:
    auto save() const -> void;
    auto load() -> void;
    struct Data;
    Data *d;
};

#endif // RECENTINFO_HPP
