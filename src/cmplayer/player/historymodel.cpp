#include "historymodel.hpp"
#include "mrlstatesqlfield.hpp"
#include "misc/log.hpp"

DECLARE_LOG_CONTEXT(History)

class Transactor {
public:
    Transactor(QSqlDatabase *db, bool commit = true)
        : m_db(db), m_commit(commit) { start(); }
    ~Transactor() { done(); }
    auto start() -> bool
    {
        if (m_doing)
            return true;
        return m_doing = check(m_db->transaction(), "transaction()"_b);
    }
    auto done() -> void
    {
        if (!m_doing)
            return;
        if (!m_commit || !check(m_db->commit(), "commit()"_b))
            check(m_db->rollback(), "rollback()"_b);
        m_doing = false;
    }
private:
    auto check(bool ok, const char *at) const noexcept -> bool
    {
        if (!ok)
            _Error("Error on %%: %%", _L(at), m_db->lastError().text());
        return ok;
    }
    QSqlDatabase *m_db = nullptr;
    bool m_commit = true, m_doing = false;
};

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
    const QString table = "state"_a % _N(currentVersion);
    bool rememberImage = false, reload = true, visible = false;
    int idx_mrl, idx_last, idx_device, rows = 0;
    auto check(const QSqlQuery &query) -> bool
    {
        if (!query.lastError().isValid())
            return true;
        _Error("Error on query: %% for %%"
               , query.lastError().text(), query.lastQuery());
        return false;
    }
    auto insert(const MrlState *state) -> bool
    {
        if (state->mrl == cached.mrl)
            cached.mrl = Mrl();
        fields.insert(finder, state);
        return check(finder);
    }
    auto load() -> bool
    {
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
            rows = loader.value(u"total"_q).toInt();
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
        Transactor t(&db);
        finder.exec(u"DROP TABLE IF EXISTS %1"_q.arg(table));
        QString columns = _ToStringList(fields, [] (const MrlStateSqlField &f) {
            return QString(_L(f.property().name()) % ' '_q % f.type());
        }).join(u", "_q);

        finder.exec(u"CREATE TABLE %1 (%2)"_q.arg(table).arg(columns));
        for (auto state : states) {
            insert(state);
            delete state;
        }
    }
    auto getMrl() const -> Mrl
    {
        const auto id = loader.value(idx_mrl).toString();
        const auto dev = loader.value(idx_device).toString();
        return Mrl::fromUniqueId(id, dev);
    }
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
    d->fields.prepareSelect(d->table, d->fields.field(u"mrl"_q));

    d->db = QSqlDatabase::addDatabase(u"QSQLITE"_q, u"history-model"_q);
    d->db.setDatabaseName(_WritablePath() % "/history.db"_a);
    if (!d->db.open()) {
        _Error("Error: %%. Couldn't create database.",
               d->db.lastError().text());
        return;
    }

    d->loader = QSqlQuery(d->db);
    d->finder = QSqlQuery(d->db);

    d->finder.exec(u"PRAGMA journal_mode = WAL"_q);
    d->finder.exec(u"PRAGMA user_version"_q);
    int version = 0;
    if (d->finder.next())
        version = d->finder.value(0).toLongLong();
    if (version < currentVersion) {
        d->import(_ImportMrlStates(version, d->db));
        d->finder.exec("PRAGMA user_version = "_a % _N(currentVersion));
    } else {
        auto record = d->db.record(d->table);
        QVector<MrlStateSqlField> lacks;
        for (const auto &field : d->fields) {
            if (!record.contains(_L(field.property().name())))
                lacks.append(field);
        }
        if (!lacks.isEmpty()) {
            Transactor t(&d->db);
            for (auto &field : lacks) {
                const auto query = u"ALTER TABLE %3 ADD COLUMN %1 %2"_q
                        .arg(_L(field.property().name())).arg(field.type());
                d->finder.exec(query.arg(d->table));
                d->check(d->finder);
            }
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
    if (d->restores.isEmpty())
        return true;
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
    if (_InRange0(row, d->rows) && d->loader.seek(row))
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
    hash[NameRole] = "name"_b;
    hash[LatestPlayRole] = "latestplay"_b;
    hash[LocationRole] = "location"_b;
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
    Transactor t(&d->db);
    d->insert(state);
    t.done();
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
    d->restores.prepareSelect(d->table, d->fields.field(u"mrl"_q));
    d->restores.prepareInsert(d->table);
}

auto HistoryModel::clear() -> void
{
    Transactor t(&d->db);
    d->loader.exec("DELETE FROM "_a % d->table);
    t.done();
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
