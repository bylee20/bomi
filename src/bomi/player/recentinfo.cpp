#include "recentinfo.hpp"
#include "playlist.hpp"
#include "misc/objectstorage.hpp"

struct RecentInfo::Data {
    int max = 10;
    Playlist openList, lastList;
    Mrl lastMrl;
    ObjectStorage storage;
    Update update;
};

RecentInfo::RecentInfo()
: d(new Data) {
    d->storage.setObject(this, u"recent-info"_q);
    d->storage.add("last-mrl",
                   [=] () { return QVariant::fromValue(d->lastMrl.location()); },
                   [=] (auto &var) { return d->lastMrl = var.toString(); });
    d->storage.add("recent-open-list", &d->openList);
    d->storage.add("last-playlist", &d->lastList);
    d->storage.restore();
}

RecentInfo::~RecentInfo() {
    d->storage.save();
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
    if (d->update)
        d->update(d->openList);
}

auto RecentInfo::clear() -> void
{
    d->openList.clear();
    if (d->update)
        d->update(d->openList);
}

auto RecentInfo::save() const -> void
{
    d->storage.save();
}

auto RecentInfo::load() -> void
{
    d->storage.restore();
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

auto RecentInfo::setUpdateFunc(Update &&func) -> void
{
    d->update = std::move(func);
}
