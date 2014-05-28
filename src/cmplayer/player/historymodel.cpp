#include "historymodel.hpp"
#include "mrlstatesqlfield.hpp"
#include "misc/log.hpp"

DECLARE_LOG_CONTEXT(History)

auto reg_history_model() -> void { qmlRegisterType<HistoryModel>(); }

struct RowCache { Mrl mrl; int row = -1; };

static constexpr auto currentVersion = MrlState::Version + 1;

struct HistoryModel::Data {
    HistoryModel *p = nullptr;
    QSqlDatabase db;
    RowCache rowCache;
    QSqlQuery loader, finder;
    QSqlError error;
    MrlStateSqlFieldList fields, restores;
    MrlState cached;
    const MrlState default_{};
    const QString table = _L("state") % _N(currentVersion);
    bool rememberImage = false, reload = true, visible = false;
    int idx_mrl, idx_last, idx_device, rows = 0;
    bool check(const QSqlQuery &query) {
        if (!query.lastError().isValid())
            return true;
        _Error("Query Error: %% for %%"
               , query.lastError().text(), query.lastQuery());
        return false;
    }
    bool insert(const MrlState *state) {
        if (state->mrl == cached.mrl)
            cached.mrl = Mrl();
        fields.insert(finder, state);
        return check(finder);
    }

    bool load() {
        const QString select = QString::fromLatin1(
            "SELECT mrl, last_played_date_time, device, "
            "(SELECT COUNT(*) FROM %1) as total "
            "FROM %2 ORDER BY last_played_date_time DESC"
        );
        if (!loader.exec(select.arg(table).arg(table)))
            return false;
        Q_ASSERT(!loader.isForwardOnly());
        p->beginResetModel();
        rows = 0;
        if (loader.next()) {
            rows = loader.value("total").toInt();
            idx_mrl = 0;
            idx_last = 1;
            idx_device = 2;
            loader.seek(-1);
        }
        error = QSqlError();
        p->endResetModel();
        reload = false;
        rowCache = RowCache();
        return true;
    }

    auto import(const QVector<MrlState*> &states) -> void
    {
        db.transaction();
        finder.exec(QString::fromLatin1("DROP TABLE IF EXISTS %1").arg(table));
        QString columns;
        for (auto &f : fields)
            columns += f.property().name() % _L(' ') % f.type() % _L(", ");
        columns.chop(2);

        finder.exec(QString::fromLatin1("CREATE TABLE %1 (%2)").arg(table).arg(columns));
        for (auto state : states) {
            insert(state);
            delete state;
        }
        db.commit();
    }


    Mrl getMrl() const {
        const auto id = loader.value(idx_mrl).toString();
        const auto dev = loader.value(idx_device).toString();
        return Mrl::fromUniqueId(id, dev);
    }
};

struct SqlField {
    QString name, type;
};

HistoryModel::HistoryModel(QObject *parent)
: QAbstractTableModel(parent), d(new Data) {
    d->p = this;

    auto &metaObject = MrlState::staticMetaObject;
    const int count = metaObject.propertyCount();
    const int offset = metaObject.propertyOffset();
    Q_ASSERT(offset == 1);
    d->fields.reserve(count - offset);
    for (int i=offset; i<count; ++i) {
        const auto property = metaObject.property(i);
        const auto def = property.read(&d->default_);
        d->fields.push_back(property, def);
    }
    d->fields.prepareInsert(d->table);
    d->fields.prepareSelect(d->table, d->fields.field("mrl"));

    d->db = QSqlDatabase::addDatabase("QSQLITE", "history-model");
    d->db.setDatabaseName(_WritablePath() % "/history.db");
    if (!d->db.open()) {
        _Error("Error: %%. Couldn't create database.",
               d->db.lastError().text());
        return;
    }

    d->loader = QSqlQuery(d->db);
    d->finder = QSqlQuery(d->db);
    d->finder.exec("PRAGMA user_version");
    int version = 0;
    if (d->finder.next())
        version = d->finder.value(0).toLongLong();
    if (version < currentVersion) {
        d->import(_ImportMrlStates(version, d->db));
        d->finder.exec("PRAGMA user_version = " % _N(currentVersion));
    } else {
        auto record = d->db.record(d->table);
        QList<MrlStateSqlField> lacks;
        for (const auto &field : d->fields) {
            if (!record.contains(_L(field.property().name())))
                lacks.append(field);
        }
        if (!lacks.isEmpty()) {
            d->db.transaction();
            for (auto &field : lacks) {
                const QString query = QString("ALTER TABLE %3 ADD COLUMN %1 %2")
                        .arg(field.property().name()).arg(field.type());
                d->finder.exec(query.arg(d->table));
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

auto HistoryModel::getState(MrlState *state) const -> bool
{
    if (!state->mrl.isUnique())
        return false;
    Q_ASSERT(d->restores.isSelectPrepared());
    if (d->cached.mrl != state->mrl)
        return d->restores.select(d->finder, state);
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
    Q_ASSERT(d->fields.isSelectPrepared());
    if (!d->fields.select(d->finder, &d->cached, mrl))
        return nullptr;
    d->cached.mrl = mrl;
    return &d->cached;
}

auto HistoryModel::play(int row) -> void
{
    if (_InRange(0, row, d->rows - 1) && d->loader.seek(row))
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
    case LatestPlayRole: {
        const auto msecs = d->loader.value(d->idx_last).toLongLong();
        return QDateTime::fromMSecsSinceEpoch(msecs).toString(Qt::ISODate);
    } case LocationRole:
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

auto HistoryModel::setPropertiesToRestore(const QVector<QMetaProperty> &properties) -> void
{
    d->restores.clear();
    d->restores.reserve(properties.size());
    for (auto &f : d->fields) {
        if (properties.contains(f.property()))
            d->restores.push_back(f);
    }
    d->restores.prepareSelect(d->table, d->fields.field("mrl"));
    d->restores.prepareInsert(d->table);
}

auto HistoryModel::clear() -> void
{
    d->db.transaction();
    d->loader.exec("DELETE FROM " % d->table);
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
