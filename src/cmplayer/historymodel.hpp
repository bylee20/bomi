#ifndef HISTORYMODEL_HPP
#define HISTORYMODEL_HPP

#include "stdafx.hpp"
#include "mrlstate.hpp"

class HistoryModel: public QAbstractTableModel {
	Q_OBJECT
public:
	enum Role {NameRole = Qt::UserRole + 1, LatestPlayRole, LocationRole};
	HistoryModel(QObject *parent = nullptr);
	~HistoryModel();
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	QSqlError error() const;
	QHash<int, QByteArray> roleNames() const;
	const MrlState *find(const Mrl &mrl) const;
	bool getState(MrlState *state) const;
	Q_INVOKABLE void play(int row);
	void update(const MrlState *state, bool reload);
	void setRememberImage(bool on);
	void getAppState(MrlState *appState);
	void setAppState(const MrlState *appState);
	void clear();
signals:
	void playRequested(const Mrl &mrl);
private:
	struct Data;
	Data *d;
};

#endif // HISTORYMODEL_HPP
