#include "listmodel.hpp"
#include "historymodel.hpp"
#include "playengine.hpp"
#include "recentinfo.hpp"

HistoryModel::HistoryModel(QObject *parent)
: BaseListModel(parent, ColumnCount) {
	load();
}

HistoryModel::~HistoryModel(){
	save();
}

QList<Mrl> HistoryModel::top(int count) const {
	QList<Mrl> list;
	for (int i=0; i<count && i<m_items.size(); ++i)
		list.push_back(m_items[i].mrl);
	return list;
}

QVariant HistoryModel::data(const QModelIndex &index, int role) const {
	const int row = index.row();
	if (0 <= row && row < m_items.size()) {
		if (role == Qt::DisplayRole)
			role = columnToRole(index.column());
		switch (role) {
		case NameRole:
			return m_items[row].mrl.displayName();
		case LatestPlayRole:
			return m_items[row].date.toString(Qt::ISODate);
		case LocationRole:
			return m_items[row].mrl.toString();
		default:
			return QVariant();
		}
	}
	return QVariant();
}

HistoryModel::RoleHash HistoryModel::roleNames() const {
	RoleHash hash;
	hash[NameRole] = "name";
	hash[LatestPlayRole] = "latestplay";
	hash[LocationRole] = "location";
	return hash;
}

void HistoryModel::setStarted(Mrl mrl) {
	if (!m_rememberImage && mrl.isImage())
		return;
	int index = findIndex(mrl);
	Item item;
	if (index < 0) {
		item.mrl = mrl;
	} else {
		beginRemoveRows(QModelIndex(), index, index);
		item = m_items.takeAt(index);
		endRemoveRows();
	}
	item.date = QDateTime::currentDateTime();
	beginInsertRows(QModelIndex(), 0, 0);
	m_items.prepend(item);
	endInsertRows();
	if (m_items.size() > 999) {
		beginRemoveRows(QModelIndex(), 999, m_items.size()-1);
		while (m_items.size() > 999)
			m_items.removeLast();
		endRemoveRows();
	}
}

void HistoryModel::setStopped(Mrl mrl, int time, int duration) {
	if (!m_rememberImage && mrl.isImage())
		return;
	if (!mrl.isDvd() && duration > 500 && duration - time > 500) {
		const int row = findIndex(mrl);
		if (row != -1) {
			m_items[row].date = QDateTime::currentDateTime();
			m_items[row].stopped = time;
			emit rowChanged(row);
		}
	}
}

void HistoryModel::setFinished(Mrl mrl) {
	if (!m_rememberImage && mrl.isImage())
		return;
	const int row = findIndex(mrl);
	if (row != -1) {
		m_items[row].date = QDateTime::currentDateTime();
		m_items[row].stopped = -1;
		emit rowChanged(row);
	}
}

void HistoryModel::save() const {
	QSettings set;
	set.beginGroup("history");
	const int size = m_items.size();
	set.beginWriteArray("list", size);
	for (int i=0; i<size; ++i) {
		const Item &item = m_items[i];
		set.setArrayIndex(i);
		set.setValue("mrl", item.mrl.toString());
		set.setValue("date", item.date);
		set.setValue("stopped-position", item.stopped);
	}
	set.endArray();
	set.endGroup();
}

void HistoryModel::load() {
	QSettings set;
	set.beginGroup("history");
	const int size = set.beginReadArray("list");
	for (int i=0; i<size; ++i) {
		set.setArrayIndex(i);
		const Mrl mrl = set.value("mrl", QString()).toString();
		if (mrl.isEmpty() || mrl.isImage())
			continue;
		Item item;
		item.mrl = mrl;
		item.date = set.value("date", QDateTime()).toDateTime();
		item.stopped = set.value("stopped-position", -1).toInt();
		m_items.append(item);
	}
}


int HistoryModel::findIndex(const Mrl &mrl) const {
	for (int i=0; i<m_items.size(); ++i) {
		if (m_items[i].mrl == mrl)
			return i;
	}
	return -1;
}
