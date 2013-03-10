#ifndef HISTORYMODEL_HPP
#define HISTORYMODEL_HPP

#include "stdafx.hpp"
#include "dialogs.hpp"
#include "global.hpp"
#include "mrl.hpp"
#include "listmodel.hpp"

class PlayEngine;		class QModelIndex;

class HistoryModel : public BaseListModel {
	Q_OBJECT
public:
	enum Column {Name = 0, LatestPlay, Location, ColumnCount};
	enum Role {NameRole = Qt::UserRole + 1, LatestPlayRole, LocationRole};
	HistoryModel(QObject *parent = nullptr);
	~HistoryModel();
	QList<Mrl> top(int count = 10) const;
	int stoppedTime(const Mrl &mrl) const { const int i = findIndex(mrl); return i < 0 ? -1 : m_items[i].stopped; }
	QDateTime stoppedDate(const Mrl &mrl) const { const int i = findIndex(mrl); return i < 0 ? QDateTime() : m_items[i].date; }
	int rowCount(const QModelIndex &parent = QModelIndex()) const { return parent.isValid() ? 0 : m_items.size(); }
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	Q_INVOKABLE void play(int row) {emit playRequested(m_items[row].mrl);}
	Q_INVOKABLE QString name(int row) const {return m_items[row].mrl.displayName();}
	Q_INVOKABLE QString latestPlay(int row) const {return m_items[row].date.toString(Qt::ISODate);}
	Q_INVOKABLE QString location(int row) const {return m_items[row].mrl.toString();}
	void setRememberImage(bool on) {m_rememberImage = on;}
public slots:
	void clear() {beginResetModel(); m_items.clear(); endResetModel();}
	void setStarted(Mrl mrl);
	void setStopped(Mrl mrl, int time, int duration);
	void setFinished(Mrl mrl);
signals:
	void playRequested(const Mrl &mrl);
private:
	static Role columnToRole(int column) {return static_cast<Role>(NameRole + column);}
	struct Item { Mrl mrl; QDateTime date; int stopped = 0; };
	void save() const;
	void load();
	RoleHash roleNames() const;
	int findIndex(const Mrl &mrl) const;
	QList<Item> m_items;
	bool m_rememberImage = false;
};

#endif // HISTORYMODEL_HPP
