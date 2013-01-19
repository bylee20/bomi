#ifndef PLAYLISTMODEL_HPP
#define PLAYLISTMODEL_HPP

#include "stdafx.hpp"
#include "playlist.hpp"
#include "listmodel.hpp"

class PlaylistModel : public BaseListModel {
	Q_OBJECT
	Q_PROPERTY(int loaded READ loaded NOTIFY loadedChanged)
	Q_PROPERTY(int count READ count NOTIFY countChanged)
	Q_PROPERTY(QChar fillChar READ fillChar WRITE setFillChar)
	Q_ENUMS(Role)
public:
	enum Role {NameRole = Qt::UserRole + 1, LocationRole, LoadedRole};
	PlaylistModel(QObject *parent = 0);
	~PlaylistModel();
	Mrl operator[](int row) const {return m_list.value(row);}
	QVariant data(const QModelIndex &index, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	void setPlaylist(const Playlist &list) {beginResetModel(); m_list = list; m_loaded = -1; endResetModel();}
	const Playlist &playlist() const {return m_list;}
	int append(const Mrl &mrl);
	void append(const Playlist &list);
	void erase(int row);
	void erase(const QModelIndexList &indexes);
	inline bool isValidRow(int row) const {return 0 <= row && row < m_list.size();}
	int loaded() const {return m_loaded;}
	int count() const {return m_list.size();}
	int rowOf(const Mrl &mrl) const {return m_list.indexOf(mrl);}
	int next() const {return m_loaded+1;}
	int previous() const {return m_loaded-1;}
	Mrl nextMrl() const {return m_list.value(m_loaded + 1);}
	Mrl previousMrl() const {return m_list.value(m_loaded - 1);}
	bool swap(int r1, int r2);
	Mrl loadedMrl() const {return m_list.value(m_loaded);}
	void merge(const Playlist &playlist);
	QHash<int, QByteArray> roleNames() const override;
	Q_INVOKABLE void play(int row);
	Q_INVOKABLE QString name(int row) const {return m_list[row].displayName();}
	Q_INVOKABLE QString location(int row) const {return m_list[row].toString();}
	Q_INVOKABLE QString number(int row) const;
	QChar fillChar() const {return m_fill;}
	void setFillChar(QChar c) {if (_Change(m_fill, c)) emit fillCharChanged();}
public slots:
	void clear() {beginResetModel(); m_list.clear(); m_loaded = -1; endResetModel();}
	void playNext() {play(next());}
	void playPrevious() {play(previous());}
signals:
	void finished();
	void loadedChanged(int row);
	void countChanged(int count);
	void contentWidthChanged();
//	void playRequested(const Mrl &mrl);
	void playRequested(int row);
	void fillCharChanged();
private:
	friend class PlayEngine;
	void setLoaded(int row);
	Playlist m_list;
	int m_loaded = -1;
	QChar m_fill = QChar::Null;
};

#endif // PLAYLISTMODEL_HPP
