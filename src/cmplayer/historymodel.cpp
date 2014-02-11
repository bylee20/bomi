#include "historymodel.hpp"
#include "appstate.hpp"

using namespace MrlStateHelpers;

struct HistoryModel::Data {
	HistoryModel *p = nullptr;
	QSqlQuery loader, finder;
	QSqlError error;
	QList<MrlField> fields = MrlField::list();
	QList<MrlField> restores;
	QString insertTemplate;
	MrlState cached;
	const QString stateTable = _L("state") % _N(MrlState::Version), appTable = _L("app") % _N(MrlState::Version);
	bool rememberImage = false, reload = true;
	bool insertToApp(const MrlState *state) {
		if (_InsertMrlState(finder, fields, state, _MakeInsertQueryTemplate(appTable, fields)))
			return true;
		qDebug() << finder.lastError().text() << "in" << finder.lastQuery();
		return false;
	}
	bool insert(const MrlState *state) {
		if (insertTemplate.isEmpty())
			insertTemplate = _MakeInsertQueryTemplate(stateTable, fields);
		if (state->mrl == cached.mrl)
			cached.mrl = Mrl();
		if (_InsertMrlState(finder, fields, state, insertTemplate))
			return true;
		qDebug() << finder.lastError().text() << "in" << finder.lastQuery();
		return false;
	}
	int rows = 0;
	bool load() {
		const QString select = QString::fromLatin1("SELECT *, (SELECT COUNT(*) FROM %1) as total FROM %2 ORDER BY last_played_date_time DESC");
		if (!loader.exec(select.arg(stateTable).arg(stateTable)))
			return false;
		Q_ASSERT(!loader.isForwardOnly());
		p->beginResetModel();
		rows = 0;
		if (loader.next()) {
			rows = loader.value("total").toInt();
			loader.seek(-1);
		}
		error = QSqlError();
		p->endResetModel();
		reload = false;
		return true;
	}

	void import(const std::tuple<MrlState*, QList<MrlState*>> &tuple) {
		db.transaction();
		finder.exec(QString::fromLatin1("DROP TABLE IF EXISTS %1").arg(stateTable));
		finder.exec(QString::fromLatin1("DROP TABLE IF EXISTS %1").arg(appTable));


		QString columns;
		for (auto &f : fields)
			columns += f.property().name() % _L(' ') % f.type() % _L(", ");
		columns.chop(2);
		finder.exec(QString::fromLatin1("CREATE TABLE %1 (%2)").arg(stateTable).arg(columns));
		finder.exec(QString::fromLatin1("CREATE TABLE %1 (%2)").arg(appTable).arg(columns));
		auto app = std::get<0>(tuple);
		insertToApp(app);
		delete app;
		for (auto state : std::get<1>(tuple)) {
			insert(state);
			delete state;
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

	d->loader = QSqlQuery(d->db);
	d->finder = QSqlQuery(d->db);
	d->finder.exec("PRAGMA user_version");
	int version = 0;
	if (d->finder.next())
		version = d->finder.value(0).toLongLong();
	if (version != MrlState::Version) {
		d->import(_ImportMrlStatesFromPreviousVersion(version, d->db));
		d->finder.exec("PRAGMA user_version = " % _N(MrlState::Version));
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
	d->finder.exec(QString::fromLatin1("SELECT * FROM %1 LIMIT 1").arg(d->appTable));
	if (!d->finder.next()) {
		qDebug() << "no previous app state!";
		return;
	}
	_FillMrlStateFromQuery(appState, d->fields, d->finder);
}

void HistoryModel::setAppState(const MrlState *state) {
	const auto mrl = state->mrl;
	const_cast<MrlState*>(state)->mrl = Mrl();
	d->db.transaction();
	d->insertToApp(state);
	d->db.commit();
	const_cast<MrlState*>(state)->mrl = mrl;
}

bool HistoryModel::getState(MrlState *state) const {
	if (d->cached.mrl == state->mrl) {
		for (auto &f : d->restores)
			f.property().write(state, f.property().read(&d->cached));
		return true;
	}
	d->finder.exec(QString::fromLatin1("SELECT * FROM %1 WHERE mrl = %2 ").arg(d->stateTable).arg(_ToSql(state->mrl.toString())));
	if (!d->finder.next())
		return false;
	_FillMrlStateFromQuery(state, d->restores, d->finder);
	return true;
}

const MrlState *HistoryModel::find(const Mrl &mrl) const {
	if (d->cached.mrl == mrl)
		return &d->cached;
	d->finder.exec(QString::fromLatin1("SELECT * FROM %1 WHERE mrl = %2").arg(d->stateTable).arg(_ToSql(mrl.toString())));
	if (!d->finder.next())
		return nullptr;
	d->cached.mrl = mrl;
	_FillMrlStateFromQuery(&d->cached, d->fields, d->finder);
	return &d->cached;
}

void HistoryModel::play(int row) {
	if (0 <= row && row < d->rows && d->loader.seek(row))
		emit playRequested(Mrl::fromString(d->loader.value("mrl").toString()));
}

QVariant HistoryModel::data(const QModelIndex &index, int role) const {
	if (d->reload) {
		d->load();
		d->reload = false;
	}
	const int row = index.row();
	if (!(0 <= row && row < d->rows))
		return QVariant();
	if (!d->loader.seek(row))
		return QVariant();
	switch (role) {
	case NameRole:
		return Mrl::fromString(d->loader.value("mrl").toString()).displayName();
	case LatestPlayRole:
		return _DateTimeFromSql(d->loader.value("last_played_date_time").toLongLong());
	case LocationRole:
		return d->loader.value("mrl");
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

void HistoryModel::setPropertiesToRestore(const QList<QMetaProperty> &properties) {
	d->restores.clear();
	d->restores.reserve(properties.size());
	for (auto &f : d->fields) {
		if (properties.contains(f.property()))
			d->restores.push_back(f);
	}
}

void HistoryModel::clear() {
	d->db.transaction();
	d->loader.exec("DELETE FROM " % d->stateTable);
	d->db.commit();
	d->load();
}
