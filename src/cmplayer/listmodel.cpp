#include "listmodel.hpp"
#include <set>
#include <QtCore/QDebug>

void ListModel::Item::emitDataChanged() {
	if (m_model)
		m_model->emitRowDataChanged(_row());
}

void ListModel::Item::emitDataChanged(int column) {
	if (m_model)
		m_model->emitDataChanged(_row(), column);
}

ListModel::ListModel(int columnCount, QObject *parent)
: QAbstractItemModel(parent), m_c_count(columnCount) {
	for (int i=0; i<m_c_count; ++i)
		m_c_title.push_back(QString());
}

ListModel::~ListModel() {
	qDeleteAll(m_item);
}

int ListModel::rowCount(const QModelIndex &parent) const {
	return parent.isValid() ? 0 : m_item.size();
}

int ListModel::columnCount(const QModelIndex &parent) const {
	return parent.isValid() ? 0 : m_c_count;
}

void ListModel::setColumnTitle(const QStringList &title) {
	for (int i=0; i<title.size() && i<m_c_count; ++i)
		m_c_title[i] = title[i];
}

void ListModel::setColumnTitle(int column, const QString &title) {
	if (isValidColumn(column))
		m_c_title[column] = title;
}

QVariant ListModel::headerData(int s, Qt::Orientation o, int role) const {
	if (role != Qt::DisplayRole)
		return QVariant();
	if (o == Qt::Horizontal && isValidColumn(s))
		return m_c_title[s];
	if (o == Qt::Vertical && isValidRow(s))
		return s+1;
	return QVariant();
}

QModelIndex ListModel::index(int row, int column, const QModelIndex &parent) const {
	if (parent.isValid() || !isValidRow(row) || !isValidColumn(column))
		return QModelIndex();
	return createIndex(row, column);
}

QVariant ListModel::data(const QModelIndex &index, int role) const {
	const int row = index.row();
	const int column = index.column();
	if (isValidRow(row) && isValidColumn(column))
		return m_item[row]->data(column, role);
	return QVariant();
}

void ListModel::emitDataChanged(int row, int column) {
	const QModelIndex idx = index(row, column);
	emit dataChanged(idx, idx);
}

void ListModel::emitRowDataChanged(int top, int bottom) {
	emit dataChanged(index(top, 0), index(bottom, m_c_count - 1));
}

void ListModel::emitRowDataChanged(int row) {
	emitDataChanged(row, row);
}

void ListModel::clear() {
	qDeleteAll(m_item);
	m_item.clear();
	reset();
}

void ListModel::setItems(const QList<Item*> &item) {
	qDeleteAll(m_item);
	m_item = item;
	for (int i=0; i<m_item.size(); ++i) {
		m_item[i]->m_app = i;
		m_item[i]->m_model = this;
	}
	reset();
}

void ListModel::insertItem(int row, Item *item) {
	if (row < 0)
		row = 0;
	else if (row > m_item.size())
		row = m_item.size();
	beginInsertRows(QModelIndex(), row, row);
	insert(row, item);
	endInsertRows();
}

void ListModel::append(Item *item) {
	item->m_app = m_item.isEmpty() ? 0 : m_item.last()->m_app + 1;
	item->m_model = this;
	m_item.append(item);
}

void ListModel::prepend(Item *item) {
	item->m_model = this;
	item->m_app = m_item.isEmpty() ? 0 : m_item.first()->m_app - 1;
	m_item.prepend(item);
}

void ListModel::insert(int row, Item *item) {
	if (row <= 0)
		prepend(item);
	else if (row >= size())
		append(item);
	else {
		item->m_model = this;
		m_item.insert(row, item);
		sync(row);
	}
}

ListModel::Item *ListModel::take(int row) {
	if (row < 0 || row >= m_item.size())
		return 0;
	Item *item = m_item.takeAt(row);
	m_item[row]->m_app = item->m_app;
	item->m_model = 0;
	if (row == 0 || row == m_item.size()-1)
		return item;
	sync(row);
	return item;
}

void ListModel::sync(int from, int to) {
	if (from <= to) {
		if (from <= 0)
			from = 1;
		int app = m_item[from-1]->m_app;
		for (; from <= to; ++from)
			m_item[from]->m_app = ++app;
	} else if (from > to) {
		if (from >= m_item.size()-1)
			from = m_item.size() - 2;
		int app = m_item[from+1]->m_app;
		for (; from >= to; --from)
			m_item[from]->m_app = --app;
	}
	for (int i=0; i<m_item.size()-1; ++i)
		Q_ASSERT(m_item[i]->m_app + 1 == m_item[i+1]->m_app);
}

bool ListModel::move(int from, int to) {
	if (!isValidRow(from) || !isValidRow(to) || from == to)
		return false;
	Item *item = m_item.takeAt(from);
	m_item.insert(from < to ? to - 1 : to, item);
	if (qAbs(to - from) > m_item.size()) {
		if (to < from)
			qSwap(to, from);
		sync(0, from);
		sync(to, m_item.size()-1);
	} else
		sync(from, to);
	return true;
}

void ListModel::moveItem(int from, int to) {
	if (move(from, to)) {
		if (from < to)
			emitRowDataChanged(from, to);
		else
			emitRowDataChanged(to, from);
	}
}

ListModel::Item *ListModel::takeItem(int row) {
	if (!isValidRow(row))
		return 0;
	beginRemoveRows(QModelIndex(), row, row);
	Item *item = take(row);
	endRemoveRows();
	return item;
}

void ListModel::removeItem(const QModelIndexList &indexes) {
	std::set<int> rows;
	for (int i=0; i<indexes.size(); ++i)
		rows.insert(indexes[i].row());
	if (rows.empty())
		return;
	const int min = *rows.begin() - 1;
	const int max = *rows.rbegin() - rows.size() + 1;
	std::set<int>::const_reverse_iterator it = rows.rbegin();
	for (; it != rows.rend(); ++it)
		delete m_item.takeAt(*it);
	if (max < m_item.size() - min)
		sync(max, 0);
	else
		sync(min, m_item.size()-1);
	reset();
}
