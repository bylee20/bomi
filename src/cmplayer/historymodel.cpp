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
		if (mrl.isEmpty())
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

//class HistoryView::Item : public ListModel::Item {
//public:
//	Item() {}
//	Item(const Mrl &mrl, const QDateTime &date);
//	~Item() {}
//	int stoppedTime() const {return m_stopped;}
//	void setStoppedTime(int time) {m_stopped = time;}
//	bool isStopped() const {return m_stopped > 0;}
//	const Mrl &mrl() const {return m_mrl;}
//	QVariant data(int column, int role) const {
//		if (role != Qt::DisplayRole)
//			return QVariant();
//		switch (column) {
//		case Name:
//			return m_mrl.displayName();
//		case Latest:
//			return m_date.toString(Qt::ISODate);
//		case Location:
//			return m_mrl.toString();
//		default:
//			return QVariant();
//		}
//	}
//	void update(const QDateTime &date) {
//		m_date = date;
//		emitDataChanged(Latest);
//	}
//	void update() {update(QDateTime::currentDateTime());}
//	void setMrl(const Mrl &mrl) {
//		m_mrl = mrl;
//		emitDataChanged();
//	}
//	const QDateTime &date() const {return m_date;}
//private:
//	Mrl m_mrl;
//	QDateTime m_date;
//	int m_stopped = 0;
//};

//HistoryView::Item::Item(const Mrl &mrl, const QDateTime &date)
//: m_mrl(mrl), m_date(date) {
//	setMrl(mrl);
//	update(date);
//	m_stopped = -1;
//}

//struct HistoryView::Data {
//	ListModel model = {ColumnCount};
//	QMenu *context;
//	QTreeView *view;
//	typedef QMap<Mrl, QPair<int, QDateTime> > StoppedMap;
//	StoppedMap stopped;
//	int time(const Mrl &mrl) const {
//		if (mrl.isDvd()) return 0;
//		auto it = stopped.find(mrl); return it == stopped.end() ? 0 : it->first;
//	}
//	QDateTime date(const Mrl &mrl) const {
//		auto  it = stopped.find(mrl); return it == stopped.end() ? QDateTime() : it->second;
//	}

//	void push(const Mrl &mrl, int time, const QDateTime &date) {
//		if (time < 0)
//			pop(mrl);
//		else {
//			auto it = stopped.find(mrl);
//			if (it != stopped.end()) {
//				it->first = time;
//				it->second = date;
//			} else
//				stopped.insert(mrl, qMakePair(time, date));
//		}
//	}
//	void pop(const Mrl &mrl) {stopped.remove(mrl);}
//};

//HistoryView::HistoryView(QWidget *parent)
//: ToggleDialog(parent), d(new Data) {
//	d->model.setColumnTitle(Name, tr("Name"));
//	d->model.setColumnTitle(Latest, tr("Latest Play"));
//	d->model.setColumnTitle(Location, tr("Location"));

//	d->view = new QTreeView(this);
//	d->view->setSelectionMode(QTreeView::ExtendedSelection);
//	d->view->setAlternatingRowColors(true);
//	d->view->setRootIsDecorated(false);
//	d->view->setModel(&d->model);

//	d->context = new QMenu(this);

//	connect(d->view, &QTreeView::doubleClicked, [this] (const QModelIndex &index) {
//		if (const auto *item = static_cast<Item*>(this->item(index.row())))
//			emit playRequested(item->mrl());
//	});
//	connect(d->context->addAction(tr("Erase")), &QAction::triggered, [this] () {
//			d->model.removeItem(d->view->selectionModel()->selectedRows());
//	});
//	connect(d->context->addAction(tr("Clear")), &QAction::triggered, [this] () {d->model.clear();});
//	connect(this, &QWidget::customContextMenuRequested, [this] () {d->context->exec(QCursor::pos());});

//	load();

//	setWindowTitle(tr("Play History"));
//	setContextMenuPolicy(Qt::CustomContextMenu);

//	auto vbox = new QVBoxLayout(this);
//	vbox->setSpacing(0);
//	vbox->setMargin(0);
//	vbox->addWidget(d->view);
//}

//HistoryView::~HistoryView() {
//	save();
//	delete d;
//}

//int HistoryView::findIndex(const Mrl &mrl) const {
//	for (int i=0; i<d->model.size(); ++i) {
//		Item *item = static_cast<Item*>(d->model.at(i));
//		if (item->mrl() == mrl)
//			return i;
//	}
//	return -1;
//}

//HistoryView::Item *HistoryView::item(int index) const {
//	return static_cast<Item*>(d->model.item(index));
//}

//void HistoryView::setStarted(Mrl mrl) {
//	int idx = findIndex(mrl);
//	Item *item = nullptr;
//	if (idx < 0) {
//		item = new Item;
//		item->setMrl(mrl);
//		d->model.prependItem(item);
//		while (d->model.size() > 999)
//			d->model.removeItem(d->model.size()-1);
//	} else {
//		item = this->item(idx);
//		if (idx > 0)
//			d->model.moveItem(idx, 0);
//	}
//	item->update();
//}

//void HistoryView::setStopped(Mrl mrl, int time, int duration) {
//	if (mrl.isDvd() || duration < 500 || duration - time < 500)
//		return;
//	if (Item *item = findItem(mrl)) {
//		item->update();
//		item->setStoppedTime(time);
//		d->push(mrl, time, item->date());
//	}
//}

//void HistoryView::setFinished(Mrl mrl) {
//	if (Item *item = findItem(mrl)) {
//		item->update();
//		item->setStoppedTime(-1);
//		d->pop(mrl);
//	}
//}

//QList<Mrl> HistoryView::top(int count) const {
//	QList<Mrl> list;
//	for (int i=0; i<count && i<d->model.size(); ++i)
//		list.push_back(item(i)->mrl());
//	return list;
//}

//int HistoryView::stoppedTime(const Mrl &mrl) const {
//	const Item *item = findItem(mrl);
//	return item ? item->stoppedTime() : -1;
//}

//QDateTime HistoryView::stoppedDate(const Mrl &mrl) const {
//	const auto item = findItem(mrl);
//	return item ? item->date() : QDateTime();
//}

//void HistoryView::save() const {
//	QSettings set;
//	set.beginGroup("history");
//	const int size = d->model.size();
//	set.beginWriteArray("list", size);
//	for (int i=0; i<size; ++i) {
//		const Item *item = this->item(i);
//		set.setArrayIndex(i);
//		set.setValue("mrl", item->mrl().toString());
//		set.setValue("date", item->date());
//		set.setValue("stopped-position", item->stoppedTime());
//	}
//	set.endArray();
//	set.endGroup();
//}

//void HistoryView::load() {
//	QSettings set;
//	set.beginGroup("history");
//	const int size = set.beginReadArray("list");
//	for (int i=0; i<size; ++i) {
//		set.setArrayIndex(i);
//		const Mrl mrl = set.value("mrl", QString()).toString();
//		if (mrl.isEmpty())
//			continue;
//		const QDateTime date = set.value("date", QDateTime()).toDateTime();
//		int stopped = set.value("stopped-position", -1).toInt();
//		Item *item = new Item;
//		item->setMrl(mrl);
//		item->update(date);
//		item->setStoppedTime(stopped);
//		d->model.append(item);
//		if (stopped > 0)
//			d->push(mrl, stopped, date);
//	}
//}
