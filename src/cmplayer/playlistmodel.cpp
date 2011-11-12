#include "playlistmodel.hpp"
#include <QtCore/QDebug>
#include <QtGui/QStyleOptionViewItem>

PlaylistModel::PlaylistModel(QObject *parent)
: QAbstractItemModel(parent) {
	m_current = -1;
	m_defFont = QStyleOptionViewItem().font;
	m_curFont = m_defFont;
	m_curFont.setBold(true);
	m_curFont.setItalic(true);
}

PlaylistModel::~PlaylistModel() {}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const {
	if (!index.isValid())
		return QVariant();
	const int row = index.row();
	if (row < 0 || row >= m_list.size())
		return QVariant();
	if (role == Qt::DisplayRole) {
		if (index.column() == Name) {
			return QString::number(row+1) + " - " + m_list[row].displayName();
		} else if (index.column() == Location) {
			return m_list[row].toString();
		}
	} else if (role == Qt::FontRole) {
		return (row == m_current) ? m_curFont : m_defFont;
	}
	return QVariant();
}

Qt::ItemFlags PlaylistModel::flags(const QModelIndex &index) const {
	if (!index.isValid())
		return Qt::ItemIsEnabled;
	return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QVariant PlaylistModel::headerData(int section, Qt::Orientation o, int role) const {
	if (role != Qt::DisplayRole)
		return QVariant();
	if (o == Qt::Vertical)
		return section + 1;
	if (section == Name)
		return tr("Name");
	if (section == Location)
		return tr("Location");
	return QVariant();
}

QModelIndex PlaylistModel::index(int row, int column, const QModelIndex &parent) const {
	return parent.isValid() ? QModelIndex() : createIndex(row, column);
}

QModelIndex PlaylistModel::parent(const QModelIndex &/*index*/) const {
	return QModelIndex();
}

int PlaylistModel::rowCount(const QModelIndex &parent) const {
	return parent.isValid() ? 0 : m_list.size();
}

int PlaylistModel::columnCount(const QModelIndex &parent) const {
	return parent.isValid() ? 0 : ColumnCount;
}

void PlaylistModel::append(const Mrl &mrl) {
	Playlist list;
	list.append(mrl);
	append(list);
}

void PlaylistModel::merge(const Playlist &playlist) {
	if (m_list.isEmpty())
		m_list = playlist;
	else {
		for (int i=0; i<playlist.size(); ++i) {
			if (!m_list.contains(playlist[i]))
				m_list.append(playlist[i]);
		}
	}
	reset();
}

void PlaylistModel::append(const Playlist &list) {
	if (list.isEmpty())
		return;
	beginInsertRows(QModelIndex(), m_list.size(), m_list.size() + list.size() - 1);
	m_list += list;
	endInsertRows();
}

void PlaylistModel::erase(int row) {
	if (isValidRow(row))
		erase(QModelIndexList() << index(row, 0));
}

void PlaylistModel::erase(const QModelIndexList &indexes) {
	if (indexes.isEmpty())
		return;
	std::map<int, int> map;
	for (int i=0; i<indexes.size(); ++i)
		map[indexes[i].row()] = indexes[i].column();
	for (std::map<int, int>::const_reverse_iterator it = map.rbegin()
			; it != map.rend(); ++it) {
		m_list.removeAt(it->first);
	}
	if (map.count(m_current) > 0)
		m_current = -1;
	reset();
}

void PlaylistModel::setCurrentRow(int row) {
	if (!isValidRow(row))
		row = -1;
	if (m_current == row)
		return;
	const int old = m_current;
	m_current = row;
	if (old != -1)
		emitRowChanged(old);
	if (m_current != -1)
		emitRowChanged(m_current);
}

bool PlaylistModel::swap(int r1, int r2) {
	if (!isValidRow(r1) || !isValidRow(r2))
		return false;
	if (r1 == r2)
		return true;
	m_list.swap(r1, r2);
	if (r1 == m_current)
		m_current = r2;
	else if (r2 == m_current)
		m_current = r1;
	emitRowChanged(r1);
	emitRowChanged(r2);
	return true;
}

