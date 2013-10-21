#include "recentinfo.hpp"
#include "playlist.hpp"
#include "playengine.hpp"

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

QList<Mrl> RecentInfo::openList() const {
	return d->openList;
}

void RecentInfo::stack(const Mrl &mrl) {
	if (mrl.isEmpty())
		return;
	d->openList.removeAll(mrl);
	d->openList.prepend(mrl);
	while (d->openList.size() > d->max)
		d->openList.pop_back();
	emit openListChanged(d->openList);
}

void RecentInfo::clear() {
	d->openList.clear();
	emit openListChanged(d->openList);
}

void RecentInfo::save() const {
	QSettings set;
	set.beginGroup("recent-info");
	d->openList.save("recent-open-list", &set);
	d->lastList.save("last-playlist", &set);
	set.setValue("last-mrl", d->lastMrl.location());
	set.endGroup();
}

void RecentInfo::load() {
	QSettings set;
	set.beginGroup("recent-info");
	d->openList.load("recent-open-list", &set);
	d->lastList.load("last-playlist", &set);
	d->lastMrl = set.value("last-mrl", QString()).toString();
	set.endGroup();
}

void RecentInfo::setLastPlaylist(const Playlist &list) {
	d->lastList = list;
}

Playlist RecentInfo::lastPlaylist() const {
	return d->lastList;
}

void RecentInfo::setLastMrl(const Mrl &mrl) {
	d->lastMrl = mrl;
}

Mrl RecentInfo::lastMrl() const {
	return d->lastMrl;
}
