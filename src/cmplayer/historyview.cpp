#include "listmodel.hpp"
#include "historyview.hpp"
#include "playengine.hpp"
#include "recentinfo.hpp"

class HistoryView::Item : public ListModel::Item {
public:
	Item() {}
	Item(const Mrl &mrl, const QDateTime &date);
	~Item() {}
	int stoppedTime() const {return m_stopped;}
	void setStoppedTime(int time) {m_stopped = time;}
	bool isStopped() const {return m_stopped > 0;}
	const Mrl &mrl() const {return m_mrl;}
	QVariant data(int column, int role) const {
		if (role != Qt::DisplayRole)
			return QVariant();
		switch (column) {
		case Name:
			return m_mrl.displayName();
		case Latest:
			return m_date.toString(Qt::ISODate);
		case Location:
			return m_mrl.toString();
		default:
			return QVariant();
		}
	}
	void update(const QDateTime &date) {
		m_date = date;
		emitDataChanged(Latest);
	}
	void update() {update(QDateTime::currentDateTime());}
	void setMrl(const Mrl &mrl) {
		m_mrl = mrl;
		emitDataChanged();
	}
	const QDateTime &date() const {return m_date;}
private:
	Mrl m_mrl;
	QDateTime m_date;
	int m_stopped = 0;
};

HistoryView::Item::Item(const Mrl &mrl, const QDateTime &date)
: m_mrl(mrl), m_date(m_date) {
	setMrl(mrl);
	update(date);
	m_stopped = -1;
}

struct HistoryView::Data {
	ListModel model = {ColumnCount};
	PlayEngine *engine;
	QMenu *context;
	QTreeView *view;
};

HistoryView::HistoryView(PlayEngine *engine, QWidget *parent)
: ToggleDialog(parent), d(new Data) {
	d->engine = engine;

	d->model.setColumnTitle(Name, tr("Name"));
	d->model.setColumnTitle(Latest, tr("Latest Play"));
	d->model.setColumnTitle(Location, tr("Location"));

	d->view = new QTreeView(this);
	d->view->setSelectionMode(QTreeView::ExtendedSelection);
	d->view->setAlternatingRowColors(true);
	d->view->setRootIsDecorated(false);
	d->view->setModel(&d->model);

	d->context = new QMenu(this);

	connect(engine, SIGNAL(started(Mrl)), this, SLOT(onStarted(Mrl)));
	connect(engine,	SIGNAL(stopped(Mrl,int,int)), this, SLOT(onStopped(Mrl,int,int)));
	connect(engine, SIGNAL(finished(Mrl)), this, SLOT(onFinished(Mrl)));
	connect(d->view, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(play(QModelIndex)));
	connect(d->context->addAction(tr("Erase")), SIGNAL(triggered()), this, SLOT(erase()));
	connect(d->context->addAction(tr("Clear")), SIGNAL(triggered()), this, SLOT(clearAll()));
	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu()));

	load();

	setWindowTitle(tr("Play History"));
	setContextMenuPolicy(Qt::CustomContextMenu);

	auto vbox = new QVBoxLayout(this);
	vbox->setSpacing(0);
	vbox->setMargin(0);
	vbox->addWidget(d->view);
}

HistoryView::~HistoryView() {
	save();
	delete d;
}

void HistoryView::showContextMenu() {
	d->context->exec(QCursor::pos());
}

void HistoryView::erase() {
	d->model.removeItem(d->view->selectionModel()->selectedRows());
}

int HistoryView::findIndex(const Mrl &mrl) const {
	for (int i=0; i<d->model.size(); ++i) {
		Item *item = static_cast<Item*>(d->model.at(i));
		if (item->mrl() == mrl)
			return i;
	}
	return -1;
}

HistoryView::Item *HistoryView::item(int index) const {
	return static_cast<Item*>(d->model.item(index));
}

void HistoryView::onStarted(Mrl mrl) {
	int idx = findIndex(mrl);
	Item *item = nullptr;
	if (idx < 0) {
		item = new Item;
		item->setMrl(mrl);
		d->model.prependItem(item);
		while (d->model.size() > 999)
			d->model.removeItem(d->model.size()-1);
	} else {
		item = this->item(idx);
		if (idx > 0)
			d->model.moveItem(idx, 0);
	}
	item->update();
	emit historyChanged();
}

void HistoryView::onStopped(Mrl mrl, int time, int duration) {
	if (mrl.isDvd() || duration < 500 || duration - time < 500)
		return;
	Item *item = findItem(mrl);
	if (item) {
		item->update();
		item->setStoppedTime(time);
		RecentInfo::get().setStopped(mrl, time, item->date());
	}
}

void HistoryView::onFinished(Mrl mrl) {
	Item *item = findItem(mrl);
	if (item) {
		item->update();
		item->setStoppedTime(-1);
		RecentInfo::get().setFinished(mrl);
	}
}

//void HistoryView::onStateChanged(State state, State old) {
//	qDebug() << "onStateChanged" << (int)old << "-->" << (int)state;
//	qDebug() << "onStateChanged" << (old != State::Stopped && old != State::Finished) << (state != State::Paused && state != State::Playing);
//	if (old != State::Stopped && old != State::Finished)
//		return;
//	if (state != State::Paused && state != State::Playing)
//		return;
//	const Mrl mrl = d->engine->mrl();
//	if (mrl.isEmpty())
//		return;
//	int idx = findIndex(mrl);
//	qDebug() << idx;
//	if (idx == -1) {
//		const QDateTime date = QDateTime::currentDateTime();
//		Item *item = new Item();
//		item->setMrl(mrl);
//		item->update(date);
//		d->model.prependItem(item);
//		while (d->model.size() > 999)
//			d->model.removeItem(d->model.size()-1);
//	} else {
//		Item *item = this->item(idx);
//		if (idx > 0)
//			d->model.moveItem(idx, 0);
//		item->update();
//		item->setStoppedTime(-1);
//	}
//	emit historyChanged();
//}

QList<Mrl> HistoryView::top(int count) const {
	QList<Mrl> list;
	for (int i=0; i<count && i<d->model.size(); ++i)
		list.push_back(item(i)->mrl());
	return list;
}

int HistoryView::stoppedTime(const Mrl &mrl) const {
	const Item *item = findItem(mrl);
	return item ? item->stoppedTime() : -1;
}

void HistoryView::play(const QModelIndex &index) {
	const Item *item = static_cast<Item*>(this->item(index.row()));
	if (item)
		emit playRequested(item->mrl());
}

void HistoryView::clearAll() {
	d->model.clear();
}

void HistoryView::save() const {
	QSettings set;
	set.beginGroup("history");
	const int size = d->model.size();
	set.beginWriteArray("list", size);
	for (int i=0; i<size; ++i) {
		const Item *item = this->item(i);
		set.setArrayIndex(i);
		set.setValue("mrl", item->mrl().toString());
		set.setValue("date", item->date());
		set.setValue("stopped-position", item->stoppedTime());
	}
	set.endArray();
	set.endGroup();
}

void HistoryView::load() {
	QSettings set;
	set.beginGroup("history");
	const int size = set.beginReadArray("list");
	for (int i=0; i<size; ++i) {
		set.setArrayIndex(i);
		const Mrl mrl = set.value("mrl", QString()).toString();
		if (mrl.isEmpty())
			continue;
		const QDateTime date = set.value("date", QDateTime()).toDateTime();
		int stopped = set.value("stopped-position", -1).toInt();
		Item *item = new Item;
		item->setMrl(mrl);
		item->update(date);
		item->setStoppedTime(stopped);
		d->model.append(item);
		if (stopped > 0)
			RecentInfo::get().setStopped(mrl, stopped, date);
	}
}
