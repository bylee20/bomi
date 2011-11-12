#ifndef PLAYLISTMODEL_HPP
#define PLAYLISTMODEL_HPP

#include <QtCore/QAbstractItemModel>
#include <QtGui/QFont>
#include "playlist.hpp"

class PlaylistModel : public QAbstractItemModel {
	Q_OBJECT
public:
	enum Column {Name = 0, Location, ColumnCount};
	PlaylistModel(QObject *parent = 0);
	~PlaylistModel();
	QVariant data(const QModelIndex &index, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	QVariant headerData(int section, Qt::Orientation o, int role = Qt::DisplayRole) const;
	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
	QModelIndex parent(const QModelIndex &index) const;
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	void setPlaylist(const Playlist &list) {m_list = list; m_current = -1; reset();}
	const Playlist &playlist() const {return m_list;}
	void append(const Mrl &mrl);
	void append(const Playlist &list);
	void erase(int row);
	void erase(const QModelIndexList &indexes);
	inline bool isValidRow(int row) const {return 0 <= row && row < m_list.size();}
	int currentRow() const {return m_current;}
	void setCurrentRow(int row);
	int rowOf(const Mrl &mrl) const {return m_list.indexOf(mrl);}
	Mrl next() const {return m_list.value(m_current + 1);}
	Mrl previous() const {return m_list.value(m_current - 1);}
	bool swap(int r1, int r2);
	Mrl mrl(int row) const {return m_list.value(row);}
	Mrl current() const {return m_list.value(m_current);}
	void merge(const Playlist &playlist);
public slots:
	void clear() {m_list.clear(); m_current = -1; reset();}
private:
	void emitRowChanged(int row) {
		emit dataChanged(index(row, 0), index(row, ColumnCount));
	}
	Playlist m_list;
	int m_current;
	QFont m_curFont, m_defFont;
};

#endif // PLAYLISTMODEL_HPP
