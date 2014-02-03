#include "historymodel.hpp"
#include "appstate.hpp"

using namespace MrlStateHelpers;

struct HistoryModel::Data {
	HistoryModel *p = nullptr;
	QSqlQuery query, finder;
	QSqlError error;
	QList<Field> fields = MrlStateHelpers::fields<MrlState>();
	QString insertTemplate, columns;
	MrlState cached;
	bool rememberImage = false, reload = true;
	bool insertApp(const MrlState *state) {
		QString q = _L("INSERT OR REPLACE INTO app (id, ") % columns % _L(") VALUES (0, %1)");
		QString values;
		for (auto &f : fields)
			values += f.toSql(f.property().read(state)) % _L(", ");
		values.chop(2);
		return query.exec(q.arg(values));
	}

	bool insert(const MrlState *state) {
		if (state->mrl == cached.mrl)
			cached.mrl = Mrl();
		QString values = toSql(state->mrl.toString()) % _L(", ")
						% toSql(state->mrl.displayName()) % _L(", ");
		for (auto &f : fields)
			values += f.toSql(f.property().read(state)) % _L(", ");
		values.chop(2);
		return finder.exec(insertTemplate.arg(values));
	}
	int rows = 0;
	bool load() {
		if (!query.exec("SELECT mrl, name, last_played_date_time, (SELECT COUNT(*) FROM state) as total FROM state ORDER BY last_played_date_time DESC"))
			return false;
		Q_ASSERT(!query.isForwardOnly());
		p->beginResetModel();
		rows = 0;
		if (query.next()) {
			rows = query.value("total").toInt();
			query.seek(-1);
		}
		error = QSqlError();
		p->endResetModel();
		reload = false;
		return true;
	}

	void import(const QList<MrlState*> &states) {
		db.transaction();
		query.exec("DROP TABLE IF EXISTS state");
		query.exec("DROP TABLE IF EXISTS app");
		QString columns;
		for (auto &f : this->fields)
			columns += f.property().name() % _L(' ') % f.type() % _L(", ");
		columns.chop(2);

		query.exec(_L("CREATE TABLE state (mrl TEXT UNIQUE, name TEXT, ") % columns % _L(')'));
		query.exec(_L("CREATE TABLE app (id INTEGER UNIQUE, ") % columns % _L(')'));
		if (!states.isEmpty()) {
			insertApp(states[0]);
			delete states[0];
			for (int i=1; i<states.size(); ++i) {
				insert(states[i]);
				delete states[i];
			}
		}
		db.commit();
	}
	QSqlDatabase db;
};

struct SqlField {
	QString name, type;
};

HistoryModel::HistoryModel(QObject *parent)
: QAbstractTableModel(parent), d(new Data) {
	d->p = this;
	d->db = QSqlDatabase::addDatabase("QSQLITE", "history-model");
	auto open = [this] () {
		auto path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
		if (!QDir().mkpath(path))
			return false;
		path += "/history.db";
		d->db.setDatabaseName(path);
		if (!d->db.open())
			return false;
		return true;
	};
	if (!open()) {
		qDebug() << "Cannot create database!" << d->db.lastError().text();
		return;
	}

	for (auto &f : d->fields)
		d->columns += f.property().name() % _L(", ");
	d->columns.chop(2);
	d->insertTemplate = _L("INSERT OR REPLACE INTO state (mrl, name, ") % d->columns % _L(") VALUES (%1)");
	d->query = QSqlQuery(d->db);
	d->finder = QSqlQuery(d->db);
	d->query.exec("PRAGMA user_version");
	int version = 0;
	if (d->query.next())
		version = d->query.value(0).toLongLong();
	if (version != MrlState::Version) {
		d->import(_ImportMrlStatesFromPreviousVersion(version, d->db));
		d->query.exec("PRAGMA user_version = " % _N(MrlState::Version));
	}
	d->load();
}

HistoryModel::~HistoryModel() {
	delete d;
}

int HistoryModel::rowCount(const QModelIndex &index) const {
	return index.isValid() ? 0 : d->rows;
}

int HistoryModel::columnCount(const QModelIndex &index) const {
	return index.isValid() ? 0 : 3;
}

void HistoryModel::getAppState(MrlState *appState) {
	d->finder.exec(_L("SELECT * FROM app LIMIT 1"));
	if (!d->finder.next()) {
		qDebug() << "no previous app state!";
		return;
	}
	for (auto &f : d->fields)
		f.property().write(appState, f.fromSql(d->finder.value(f.property().name())));
}

void HistoryModel::setAppState(const MrlState *state) {
	d->db.transaction();
	d->insertApp(state);
	d->db.commit();
}

bool HistoryModel::getState(MrlState *state, const QList<QMetaProperty> &restores) const {
	if (d->cached.mrl == state->mrl) {
		for (auto &f : d->fields) {
			if (restores.contains(f.property()))
				f.property().write(state, f.property().read(&d->cached));
		}
		return true;
	}
	d->finder.exec(_L("SELECT * FROM state WHERE mrl = ") % toSql(state->mrl.toString()));
	if (!d->finder.next())
		return false;
	for (auto &f : d->fields) {
		if (restores.contains(f.property()))
			f.property().write(state, f.fromSql(d->finder.value(f.property().name())));
	}
	return true;
}

const MrlState *HistoryModel::find(const Mrl &mrl) const {
	if (d->cached.mrl == mrl)
		return &d->cached;
	d->finder.exec(_L("SELECT * FROM state WHERE mrl = ") % toSql(mrl.toString()));
	if (!d->finder.next())
		return nullptr;
	d->cached.mrl = mrl;
	for (auto &f : d->fields)
		f.property().write(&d->cached, f.fromSql(d->finder.value(f.property().name())));
	return &d->cached;
}

void HistoryModel::play(int row) {
	if (0 <= row && row < d->rows && d->query.seek(row))
		emit playRequested(Mrl::fromString(d->query.value("mrl").toString()));
}

QVariant HistoryModel::data(const QModelIndex &index, int role) const {
	if (d->reload) {
		d->load();
		d->reload = false;
	}
	const int row = index.row();
	if (!(0 <= row && row < d->rows))
		return QVariant();
	if (!d->query.seek(row))
		return QVariant();
	switch (role) {
	case NameRole:
		return Mrl::fromString(d->query.value("mrl").toString()).displayName();
	case LatestPlayRole:
		return dateTimeFromSql(d->query.value("last_played_date_time").toLongLong());
	case LocationRole:
		return d->query.value("mrl");
	default:
		return QVariant();
	}
}

QHash<int, QByteArray> HistoryModel::roleNames() const {
	QHash<int, QByteArray> hash;
	hash[NameRole] = "name";
	hash[LatestPlayRole] = "latestplay";
	hash[LocationRole] = "location";
	return hash;
}

QSqlError HistoryModel::error() const {
	return d->error;
}


void HistoryModel::update(const MrlState *state, bool reload) {
	if (!d->rememberImage && state->mrl.isImage())
		return;
	d->db.transaction();
	d->insert(state);
	if (!d->db.commit())
		d->db.rollback();
	if (reload)
		d->load();
}

void HistoryModel::setRememberImage(bool on) {
	d->rememberImage = on;
}

void HistoryModel::clear() {
	d->db.transaction();
	d->query.exec("DELETE FROM state");
	d->db.commit();
	d->load();
}
