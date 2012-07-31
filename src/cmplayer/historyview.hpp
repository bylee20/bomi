#ifndef HISTORYVIEW_HPP
#define HISTORYVIEW_HPP

#include "dialogs.hpp"
#include "global.hpp"
#include "mrl.hpp"

class PlayEngine;		class QModelIndex;

class HistoryView : public ToggleDialog {
	Q_OBJECT
public:
	enum Column {Name = 0, Latest = 1, Location = 2, ColumnCount = 3};
	HistoryView(PlayEngine *engine, QWidget *parent);
	~HistoryView();
	QList<Mrl> top(int count = 10) const;
	int stoppedTime(const Mrl &mrl) const;
	void save() const;
	void setStopped(const Mrl &mrl, int pos, int duration) {onStopped(mrl, pos, duration);}
signals:
	void historyChanged();
	void playRequested(const Mrl &mrl);
private slots:
	void onStarted(Mrl mrl);
	void onStopped(Mrl mrl, int time, int duration);
	void onFinished(Mrl mrl);
//	void onStateChanged(State state, State old);
	void play(const QModelIndex &index);
	void clearAll();
	void showContextMenu();
	void erase();
private:
	void load();
	int findIndex(const Mrl &mrl) const;
	class Item;
	Item *item(int index) const;
	Item *findItem(const Mrl &mrl) const {return item(findIndex(mrl));}
	struct Data;
	Data *d;
};

#endif // HISTORYVIEW_HPP
