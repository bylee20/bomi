#ifndef RECENTINFO_HPP
#define RECENTINFO_HPP

#include "stdafx.hpp"
#include "mrl.hpp"

class Playlist;		class QDateTime;
class PlayEngine;

class RecentInfo : public QObject {
	Q_OBJECT
public:
	RecentInfo(const RecentInfo&) = delete;
	RecentInfo &operator = (const RecentInfo&) = delete;
	~RecentInfo();
	QList<Mrl> openList() const;
	int stoppedTime(const Mrl &mrl) const;
	int askStartTime(const Mrl &mrl) const;
//	void play(PlayEngine *engine, const Mrl &mrl, bool ask = true);
	QDateTime stoppedDate(const Mrl &mrl)const ;
	static RecentInfo &get() {return *obj;}
	void stack(const Mrl &mrl);
	void setLastPlaylist(const Playlist &list);
	void setLastMrl(const Mrl &mrl);
	Mrl lastMrl() const;
	Playlist lastPlaylist() const;
// private:
	RecentInfo();
public slots:
	void clear();
signals:
	void openListChanged(const QList<Mrl> &list);
private:
	void save() const;
	void load();

	friend class HistoryView;
	void setStopped(const Mrl &mrl, int time, const QDateTime &date);
	void setFinished(const Mrl &mrl);

	struct Data;
	Data *d;
	static RecentInfo *obj;
	friend int main(int argc, char **argv);
};

#endif // RECENTINFO_HPP
