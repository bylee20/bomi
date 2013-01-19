#include "playlistmodel.hpp"

PlaylistModel::PlaylistModel(QObject *parent)
: BaseListModel(parent) {
	connect(this, &PlaylistModel::modelReset, [this] () {
		emit loadedChanged(m_loaded);
		emit countChanged(rowCount());
		emit contentWidthChanged();
	});
}

PlaylistModel::~PlaylistModel() {}

QHash<int, QByteArray> PlaylistModel::roleNames() const {
	QHash<int, QByteArray> names;
	names[NameRole] = "name";
	names[LocationRole] = "location";
	names[LoadedRole] = "isLoaded";
	return names;
}

QString PlaylistModel::number(int row) const {
	if (m_fill.isNull())
		return QString::number(row+1);
	int digits = 0, left = m_list.size();
	do {
		++digits;
		left = left/10;
	} while (left);
	return _N(row+1, 10, digits, m_fill);
}

void PlaylistModel::play(int row) {
	if (isValidRow(row))
		emit playRequested(row);
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const {
	const int row = index.row();
	if (!isValidRow(row))
		return QVariant();
	if (role == NameRole) {
		return name(row);
	} else if (role == LocationRole) {
		return location(row);
	} else if (role == LoadedRole)
		return m_loaded == row;
	return QVariant();
}

Qt::ItemFlags PlaylistModel::flags(const QModelIndex &index) const {
	if (!index.isValid())
		return Qt::ItemIsEnabled;
	return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

int PlaylistModel::rowCount(const QModelIndex &parent) const {
	return parent.isValid() ? 0 : m_list.size();
}

int PlaylistModel::append(const Mrl &mrl) {
	Playlist list;
	list.append(mrl);
	append(list);
	return m_list.size()-1;
}

void PlaylistModel::merge(const Playlist &playlist) {
	beginResetModel();
	if (m_list.isEmpty())
		m_list = playlist;
	else {
		for (int i=0; i<playlist.size(); ++i) {
			if (!m_list.contains(playlist[i]))
				m_list.append(playlist[i]);
		}
	}
	endResetModel();
}

void PlaylistModel::append(const Playlist &list) {
	if (list.isEmpty())
		return;
	beginInsertRows(QModelIndex(), m_list.size(), m_list.size() + list.size() - 1);
	m_list += list;
	endInsertRows();
	emit countChanged(rowCount());
}

void PlaylistModel::erase(int row) {
	if (isValidRow(row))
		erase(QModelIndexList() << index(row, 0));
}

void PlaylistModel::erase(const QModelIndexList &indexes) {
	if (indexes.isEmpty())
		return;
	beginResetModel();
	std::map<int, int> map;
	for (int i=0; i<indexes.size(); ++i)
		map[indexes[i].row()] = indexes[i].column();
	for (std::map<int, int>::const_reverse_iterator it = map.rbegin()
			; it != map.rend(); ++it) {
		m_list.removeAt(it->first);
	}
	if (map.count(m_loaded) > 0)
		m_loaded = -1;
	endResetModel();
}

void PlaylistModel::setLoaded(int row) {
	if (!isValidRow(row))
		row = -1;
	if (m_loaded == row)
		return;
	const int old = m_loaded;
	m_loaded = row;
	if (old != -1)
		emit rowChanged(old);
	if (m_loaded != -1)
		emit rowChanged(m_loaded);
	emit loadedChanged(m_loaded);
}

bool PlaylistModel::swap(int r1, int r2) {
	if (!isValidRow(r1) || !isValidRow(r2))
		return false;
	if (r1 == r2)
		return true;
	m_list.swap(r1, r2);
	if (r1 == m_loaded) {
		m_loaded = r2;
		emit loadedChanged(m_loaded);
	} else if (r2 == m_loaded) {
		m_loaded = r1;
		emit loadedChanged(m_loaded);
	}
	emit rowChanged(r1);
	emit rowChanged(r2);
	return true;
}
