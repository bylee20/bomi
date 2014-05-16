#include "historymodel.hpp"
#include "appstate.hpp"
#include "log.hpp"

DECLARE_LOG_CONTEXT(History)

auto reg_history_model() -> void {
    qmlRegisterType<HistoryModel>();
}

using namespace MrlStateHelpers;

struct RowCache {
    Mrl mrl;
    int row = -1;
};

struct HistoryModel::Data {
    HistoryModel *p = nullptr;
    QSqlQuery loader, finder;
    QSqlError error;
    QList<MrlField> fields = MrlField::list();
    QList<MrlField> restores;
    QString insertTemplate;
    MrlState cached;
    const QString stateTable = _L("state") % _N(MrlState::Version);
    const QString appTable   = _L("app")   % _N(MrlState::Version);
    bool rememberImage = false, reload = true, visible = false;
    bool check(const QSqlQuery &query) {
        if (!query.lastError().isValid())
            return true;
        _Error("Query Error: %% for %%"
               , query.lastError().text(), query.lastQuery());
        return false;
    }
    bool insertToApp(const MrlState *state) {
        const auto mrl = state->mrl;
        const_cast<MrlState*>(state)->mrl = Mrl();
        _InsertMrlState(finder, fields, state
                        , _MakeInsertQueryTemplate(appTable, fields));
        const_cast<MrlState*>(state)->mrl = mrl;
        return check(finder);
    }
    bool insert(const MrlState *state) {
        if (insertTemplate.isEmpty())
            insertTemplate = _MakeInsertQueryTemplate(stateTable, fields);
        if (state->mrl == cached.mrl)
            cached.mrl = Mrl();
        _InsertMrlState(finder, fields, state, insertTemplate);
        return check(finder);
    }
    int rows = 0;

    int idx_mrl, idx_last, idx_device;
    bool load() {
        const QString select = QString::fromLatin1("SELECT *, (SELECT COUNT(*)"
            " FROM %1) as total FROM %2 ORDER BY last_played_date_time DESC");
        if (!loader.exec(select.arg(stateTable).arg(stateTable)))
            return false;
        Q_ASSERT(!loader.isForwardOnly());
        p->beginResetModel();
        rows = 0;
        if (loader.next()) {
            rows = loader.value("total").toInt();
            auto record = loader.record();
            idx_mrl = record.indexOf("mrl");
            idx_last = record.indexOf("last_played_date_time");
            idx_device = record.indexOf("device");
            Q_ASSERT(idx_mrl >= 0 && idx_last >= 0 && idx_device >= 0);
            loader.seek(-1);
        }
        error = QSqlError();
        p->endResetModel();
        reload = false;
        rowCache = RowCache();
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

    bool findAndFill(MrlState *state, const QList<MrlField> &fields, const Mrl &mrl) {
        static const auto query = QString("SELECT %3 FROM %1 WHERE mrl = %2").arg(stateTable);
        finder.exec(query.arg(_ToSql(mrl.toString())).arg(_MrlFieldColumnListString(fields)));
        if (!finder.next())
            return false;
        _FillMrlStateFromRecord(state, fields, finder.record());
        return true;
    }
    Mrl getMrl() const {
        const auto id = loader.value(idx_mrl).toString();
        const auto dev = loader.value(idx_device).toString();
        return Mrl::fromUniqueId(id, dev);
    }
    RowCache rowCache;
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
        _Error("Error: %%. Couldn't create database.", d->db.lastError().text());
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
    } else {
        auto record = d->db.record(d->stateTable);
        QList<MrlField> lacks;
        for (const auto &field : d->fields) {
            if (!record.contains(_L(field.property().name())))
                lacks.append(field);
        }
        if (!lacks.isEmpty()) {
            d->db.transaction();
            for (auto &field : lacks) {
                const QString query = QString("ALTER TABLE %3 ADD COLUMN %1 %2")
                        .arg(field.property().name()).arg(field.type());
                d->finder.exec(query.arg(d->appTable));
                d->finder.exec(query.arg(d->stateTable));
                d->check(d->finder);
            }
            d->db.commit();
        }
    }
    d->load();
}

HistoryModel::~HistoryModel() {
    delete d;
}

auto HistoryModel::rowCount(const QModelIndex &index) const -> int
{
    return index.isValid() ? 0 : d->rows;
}

auto HistoryModel::columnCount(const QModelIndex &index) const -> int
{
    return index.isValid() ? 0 : 3;
}

auto HistoryModel::getAppState(MrlState *appState) -> void
{
    d->finder.exec(QString::fromLatin1("SELECT %1 FROM %2 LIMIT 1")
                   .arg(_MrlFieldColumnListString(d->fields)).arg(d->appTable));
    if (!d->finder.next()) {
        _Debug("No previous app state exists");
        return;
    }
    _FillMrlStateFromRecord(appState, d->fields, d->finder.record());
}

auto HistoryModel::setAppState(const MrlState *state) -> void
{
    d->db.transaction();
    d->insertToApp(state);
    d->db.commit();
}

auto HistoryModel::getState(MrlState *state) const -> bool
{
    if (!state->mrl.isUnique())
        return false;
    if (d->cached.mrl != state->mrl)
        return d->findAndFill(state, d->restores, state->mrl);
    for (auto &f : d->restores)
        f.property().write(state, f.property().read(&d->cached));
    return true;
}

auto HistoryModel::find(const Mrl &mrl) const -> const MrlState*
{
    if (!mrl.isUnique())
        return nullptr;
    if (d->cached.mrl == mrl)
        return &d->cached;
    if (!d->findAndFill(&d->cached, d->fields, mrl))
        return nullptr;
    d->cached.mrl = mrl;
    return &d->cached;
}

auto HistoryModel::play(int row) -> void
{
    if (0 <= row && row < d->rows && d->loader.seek(row))
        emit playRequested(d->getMrl());
}

auto HistoryModel::data(const QModelIndex &index, int role) const -> QVariant
{
    if (d->reload) {
        d->load();
        d->reload = false;
    }
    const int row = index.row();
    if (!(0 <= row && row < d->rows))
        return QVariant();
    if (!d->loader.seek(row))
        return QVariant();
    if (d->rowCache.row != row) {
        d->rowCache.row = row;
        d->rowCache.mrl = Mrl();
    }
    auto fillMrl = [this] () -> const Mrl& {
        if (d->rowCache.mrl.isEmpty())
            d->rowCache.mrl = d->getMrl();
        return d->rowCache.mrl;
    };
    switch (role) {
    case NameRole:
        return fillMrl().displayName();
    case LatestPlayRole:
        return _DateTimeFromSql(d->loader.value(d->idx_last).toLongLong());
    case LocationRole:
        return fillMrl().toString();
    default:
        return QVariant();
    }
}

auto HistoryModel::roleNames() const -> QHash<int, QByteArray>
{
    QHash<int, QByteArray> hash;
    hash[NameRole] = "name";
    hash[LatestPlayRole] = "latestplay";
    hash[LocationRole] = "location";
    return hash;
}

auto HistoryModel::error() const -> QSqlError
{
    return d->error;
}


auto HistoryModel::update(const MrlState *state, bool reload) -> void
{
    if (!d->rememberImage && state->mrl.isImage())
        return;
    if (!state->mrl.isUnique())
        return;
    d->db.transaction();
    d->insert(state);
    if (!d->db.commit())
        d->db.rollback();
    if (reload)
        d->load();
}

auto HistoryModel::setRememberImage(bool on) -> void
{
    d->rememberImage = on;
}

auto HistoryModel::setPropertiesToRestore(const QList<QMetaProperty> &properties) -> void
{
    d->restores.clear();
    d->restores.reserve(properties.size());
    for (auto &f : d->fields) {
        if (properties.contains(f.property()))
            d->restores.push_back(f);
    }
}

auto HistoryModel::clear() -> void
{
    d->db.transaction();
    d->loader.exec("DELETE FROM " % d->stateTable);
    d->db.commit();
    d->load();
}

auto HistoryModel::isVisible() const -> bool
{
    return d->visible;
}

auto HistoryModel::setVisible(bool visible) -> void
{
    if (_Change(d->visible, visible))
        emit visibleChanged(d->visible);
}
