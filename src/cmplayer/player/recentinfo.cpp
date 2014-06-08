#include "recentinfo.hpp"
#include "playlist.hpp"

struct RecentInfo::Data {
    int max = 10;
    Playlist openList, lastList;
    Mrl lastMrl;
};

RecentInfo::RecentInfo()
: d(new Data) {
    load();
}

RecentInfo::~RecentInfo() {
    save();
    delete d;
}

auto RecentInfo::openList() const -> QList<Mrl>
{
    return d->openList;
}

auto RecentInfo::stack(const Mrl &mrl) -> void
{
    if (mrl.isEmpty())
        return;
    d->openList.removeAll(mrl);
    d->openList.prepend(mrl);
    while (d->openList.size() > d->max)
        d->openList.pop_back();
    emit openListChanged(d->openList);
}

auto RecentInfo::clear() -> void
{
    d->openList.clear();
    emit openListChanged(d->openList);
}

auto RecentInfo::save() const -> void
{
    QSettings set;
    set.beginGroup(u"recent-info"_q);
    d->openList.save(u"recent-open-list"_q, &set);
    d->lastList.save(u"last-playlist"_q, &set);
    set.setValue(u"last-mrl"_q, d->lastMrl.location());
    set.endGroup();
}

auto RecentInfo::load() -> void
{
    QSettings set;
    set.beginGroup(u"recent-info"_q);
    d->openList.load(u"recent-open-list"_q, &set);
    d->lastList.load(u"last-playlist"_q, &set);
    d->lastMrl = set.value(u"last-mrl"_q, QString()).toString();
    set.endGroup();
}

auto RecentInfo::setLastPlaylist(const Playlist &list) -> void
{
    d->lastList = list;
}

auto RecentInfo::lastPlaylist() const -> Playlist
{
    return d->lastList;
}

auto RecentInfo::setLastMrl(const Mrl &mrl) -> void
{
    d->lastMrl = mrl;
}

auto RecentInfo::lastMrl() const -> Mrl
{
    return d->lastMrl;
}
