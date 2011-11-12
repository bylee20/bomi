#include "recentinfo.hpp"
#include "playlist.hpp"
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QDateTime>

RecentInfo *RecentInfo::obj = 0;

typedef QMap<Mrl, QPair<int, QDateTime> > StoppedMap;

struct RecentInfo::Data {
	int max;
	Playlist openList, lastList;
	StoppedMap stopped;
	Mrl lastMrl;
};

RecentInfo::RecentInfo()
: d(new Data) {
	Q_ASSERT(obj == 0);
	obj = this;
	d->max = 10;
	load();
}

RecentInfo::~RecentInfo() {
	Q_ASSERT(obj == this);
	save();
	delete d;
	obj = 0;
}

QList<Mrl> RecentInfo::openList() const {
	return d->openList;
}

void RecentInfo::setStopped(const Mrl &mrl, int time, const QDateTime &date) {
	if (time == -1)
		setFinished(mrl);
	else {
		StoppedMap::iterator it = d->stopped.find(mrl);
		if (it != d->stopped.end()) {
			it->first = time;
			it->second = date;
		} else
			d->stopped.insert(mrl, qMakePair(time, date));
	}
}

void RecentInfo::setFinished(const Mrl &mrl) {
	d->stopped.remove(mrl);
}

int RecentInfo::stoppedTime(const Mrl &mrl) const {
	if (mrl.isDVD())
		return 0;
	StoppedMap::const_iterator it = d->stopped.find(mrl);
	return it == d->stopped.end() ? 0 : it->first;
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
	set.setValue("last-mrl", d->lastMrl.toString());
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

QDateTime RecentInfo::stoppedDate(const Mrl &mrl) const {
	StoppedMap::const_iterator it = d->stopped.find(mrl);
	return it == d->stopped.end() ? QDateTime() : it->second;
}
