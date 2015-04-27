#include "historymodel.hpp"
#include "mrlstatesqlfield.hpp"
#include "misc/log.hpp"
#include <QSqlDatabase>
#include <QSqlError>
#include <QQuickItem>
#include <QSqlQuery>
#include <QSqlRecord>

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
            _Error("Error on %%: %%", at, m_db->lastError().text());
        return ok;
    }
    QSqlDatabase *m_db = nullptr;
    bool m_commit = true, m_doing = false;
};

struct RowCache { Mrl mrl; int row = -1; };

static constexpr auto currentVersion = MrlState::Version;

struct HistoryModel::Data {
    HistoryModel *p = nullptr;
    QSqlDatabase db;
    RowCache rowCache;
    QSqlQuery loader, finder;
    QSqlError error;
    MrlStateSqlFieldList fields, restores, writes;
    MrlState cached;
    const MrlState default_{};
    const QString table = MrlState::table();
    bool rememberImage = false, reload = true, visible = false;
    bool mediaTitleLocal = false, mediaTitleUrl = false;
    int idx_mrl, idx_name, idx_last, idx_device, idx_star, rows = 0;
    QMutex mutex;
    auto check(const QSqlQuery &query) -> bool
    {
        if (!query.lastError().isValid())
            return true;
        _Error("Error on query: %% for %%"
               , query.lastError().text(), query.lastQuery());
        return false;
    }
    auto upsert(const MrlState *state) -> bool
    {
        if (state->mrl() == cached.mrl())
            cached.set_mrl(Mrl());
        if (!writes.update(finder, state))
            return check(finder);
        if (finder.numRowsAffected() > 0)
            return true;
        fields.insert(finder, state);
        return check(finder);
    }
    auto load() -> bool
    {
        const QString select = QString::fromLatin1(
            "SELECT mrl, name, last_played_date_time, device, star"
            ", (SELECT COUNT(*) FROM %1) as total FROM %2 ORDER BY "
            "CASE WHEN star = 1 THEN 1 ELSE 0 END DESC"
            ", last_played_date_time DESC"
        );
        if (!loader.exec(select.arg(table).arg(table))) {
            _Error("%%", loader.lastError().text());
            _Error("Query: %%", loader.lastQuery());
            return false;
        }
        Q_ASSERT(!loader.isForwardOnly());
        p->beginResetModel();
        rows = 0;
        if (loader.next()) {
            int c = 0;
            idx_mrl = c++;
            idx_name = c++;
            idx_last = c++;
            idx_device = c++;
            idx_star = c++;
            if (_Change(rows,loader.value(c).toInt()))
                emit p->lengthChanged(rows);
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
            upsert(state);
            delete state;
        }
    }
    auto getMrl() const -> Mrl
    {
        const auto id = loader.value(idx_mrl).toString();
        const auto dev = loader.value(idx_device).toString();
        const auto name = loader.value(idx_name).toString();
        return Mrl::fromUniqueId(id, dev, name);
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
    d->writes.reserve(d->fields.size() - 1);
    for (int i=offset; i<count; ++i) {
        const auto property = metaObject.property(i);
        const auto def = property.read(&d->default_);
        d->fields.push_back(property, def);
        if (qstrcmp(property.name(), "star"))
            d->writes.push_back(d->fields.back());
    }
    d->fields.prepareInsert(d->table);
    d->fields.prepareSelect(d->table, d->fields.field(u"mrl"_q));
    d->writes.prepareUpdate(d->table, d->fields.field(u"mrl"_q));
    setPropertiesToRestore(QStringList());

    d->db = QSqlDatabase::addDatabase(u"QSQLITE"_q, u"history-model"_q);
    d->db.setDatabaseName(_WritablePath(Location::Config) % "/history.db"_a);
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
    QMutexLocker locker(&d->mutex);
    if (!state->mrl().isUnique())
        return false;
    if (d->restores.isEmpty())
        return true;
    Q_ASSERT(d->restores.isSelectPrepared());
    if (d->cached.mrl() != state->mrl())
        return d->restores.select(d->finder, state);
    for (auto &f : d->restores)
        f.property().write(state, f.property().read(&d->cached));
    return true;
}

auto HistoryModel::find(const Mrl &mrl) const -> const MrlState*
{
    QMutexLocker locker(&d->mutex);
    if (!mrl.isUnique())
        return nullptr;
    if (d->cached.mrl() == mrl)
        return &d->cached;
    Q_ASSERT(d->fields.isSelectPrepared());
    if (!d->fields.select(d->finder, &d->cached, mrl))
        return nullptr;
    d->cached.set_mrl(mrl);
    return &d->cached;
}

auto HistoryModel::play(int row) -> void
{
    QMutexLocker locker(&d->mutex);
    if (_InRange0(row, d->rows) && d->loader.seek(row))
        emit playRequested(d->getMrl());
}

auto HistoryModel::setShowMediaTitleInName(bool local, bool url) -> void
{
    if (_Change(d->mediaTitleLocal, local) | _Change(d->mediaTitleUrl, url))
        d->load();
}

auto HistoryModel::getData(const int row, int role) const -> QVariant
{
    if (d->reload) {
        d->load();
        d->reload = false;
    }
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
    case NameRole: {
        fillMrl();
        if ((d->rowCache.mrl.isLocalFile() && d->mediaTitleLocal)
                || (d->rowCache.mrl.isRemoteUrl() && d->mediaTitleUrl)) {
            const auto name = d->loader.value(d->idx_name).toString();
            if (!name.isEmpty())
                return name;
        }
        return d->rowCache.mrl.displayName();
    } case LatestPlayRole: {
        const auto msecs = d->loader.value(d->idx_last).toLongLong();
        return QDateTime::fromMSecsSinceEpoch(msecs).toString(Qt::ISODate);
    } case LocationRole:
        return fillMrl().toString();
    case StarRole:
        return d->loader.value(d->idx_star);
    default:
        return QVariant();
    }
}

auto HistoryModel::data(const QModelIndex &index, int role) const -> QVariant
{
    return getData(index.row(), role);
}

auto HistoryModel::roleNames() const -> QHash<int, QByteArray>
{
    QHash<int, QByteArray> hash;
    hash[NameRole] = "name"_b;
    hash[LatestPlayRole] = "latestplay"_b;
    hash[LocationRole] = "location"_b;
    hash[StarRole] = "star"_b;
    return hash;
}

auto HistoryModel::error() const -> QSqlError
{
    return d->error;
}

auto HistoryModel::isStarred(int row) const -> bool
{
    return getData(row, StarRole).toBool();
}

auto HistoryModel::setStarred(int row, bool star) -> void
{
    QMutexLocker locker(&d->mutex);
    if (!d->loader.seek(row)) {
        _Error("Cannot seek to %% row.", row);
        return;
    }
    const auto mrl = d->getMrl();
    const auto f = d->fields.field(u"star"_q), m = d->fields.field(u"mrl"_q);
    Transactor t(&d->db);
    d->finder.prepare("UPDATE "_a % d->table % " SET star=? WHERE mrl=?"_a);
    d->finder.bindValue(0, f.sqlData(QVariant::fromValue(star)));
    d->finder.bindValue(1, m.sqlData(QVariant::fromValue(mrl)));
    if (!d->finder.exec())
        qDebug() << d->finder.executedQuery();
    t.done();
    update();
}

auto HistoryModel::update() -> void
{
    d->load();
}

auto HistoryModel::update(const MrlState *state, const QString &column, bool reload) -> void
{
    Q_ASSERT(state);
    QMutexLocker locker(&d->mutex);
    if (!d->rememberImage && state->mrl().isImage())
        return;
    if (!state->mrl().isUnique())
        return;
    const auto f = d->fields.field(column), m = d->fields.field(u"mrl"_q);
    if (!f.isValid() || !m.isValid())
        return;
    Transactor t(&d->db);
    d->finder.prepare("UPDATE "_a % d->table % " SET "_a % column % "=? WHERE mrl=?"_a);
    d->finder.bindValue(0, f.sqlData(f.property().read(state)));
    d->finder.bindValue(1, m.sqlData(m.property().read(state)));
    d->finder.exec();
    t.done();
    if (reload)
        update();
}

auto HistoryModel::update(const MrlState *state, bool reload) -> void
{
    Q_ASSERT(state);
    QMutexLocker locker(&d->mutex);
    if (!d->rememberImage && state->mrl().isImage())
        return;
    if (!state->mrl().isUnique())
        return;
    Transactor t(&d->db);
    d->upsert(state);
    t.done();
    if (reload)
        update();
}

auto HistoryModel::setRememberImage(bool on) -> void
{
    d->rememberImage = on;
}

auto HistoryModel::isRestorable(const char *name) const -> bool
{
    QMutexLocker locker(&d->mutex);
    for (auto &f : d->fields) {
        if (!qstrcmp(f.property().name(), name))
            return true;
    }
    return false;
}

auto HistoryModel::setPropertiesToRestore(const QStringList &list) -> void
{
    QMutexLocker locker(&d->mutex);
    auto props = MrlState::defaultProperties();
    props += list;
    if (list.contains("sub_tracks"_a))
        props.push_back("sub_tracks_inclusive"_a);
    d->restores.clear();
    d->restores.reserve(props.size());
    for (auto &f : d->fields) {
        if (props.contains(_L(f.property().name())))
            d->restores.push_back(f);
    }
    d->restores.prepareSelect(d->table, d->fields.field(u"mrl"_q));
    d->restores.prepareInsert(d->table);
}

auto HistoryModel::clear() -> void
{
    QMutexLocker locker(&d->mutex);
    Transactor t(&d->db);
    d->loader.exec("DELETE FROM "_a % d->table % " WHERE star != 1 OR star IS NULL"_a);
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
