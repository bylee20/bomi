#include "mrlstatesqlfield.hpp"
#include "mrl.hpp"
#include "misc/jsonstorage.hpp"
#include <QSqlQuery>
#include <QSqlRecord>

template<class T>
SIA _Is(int type) -> bool { return qMetaTypeId<T>() == type; }

MrlStateSqlField::MrlStateSqlField(const QMetaProperty &property,
                                   const QVariant &def) noexcept
    : m_property(property)
    , m_sqlType(_L("INTEGER"))
    , m_defaultValue(def)
{
    m_v2d = [] (const QVariant &var) { return var; };
    m_d2v = [] (const QVariant &var, int) { return var; };
    const int userType = m_property.userType();
    switch (userType) {
    case QMetaType::Int:         case QMetaType::UInt:
    case QMetaType::LongLong:    case QMetaType::ULongLong:
        break;
    case QMetaType::Double:
        m_sqlType = u"REAL"_q;
        break;
    case QMetaType::QString:
        m_sqlType = u"TEXT"_q;
        break;
    case QMetaType::Bool:
        m_v2d = [] (const QVariant &value)
            { return QVariant((int)value.toBool()); };
        m_d2v = [] (const QVariant &sqlData,int)
            { return QVariant((bool)sqlData.toInt()); };
        break;
    case QMetaType::QDateTime:
        m_v2d = [] (const QVariant &value)
            { return QVariant(value.toDateTime().toMSecsSinceEpoch()); };
        m_d2v = [] (const QVariant &sqlData,int) -> QVariant
            { return QDateTime::fromMSecsSinceEpoch(sqlData.toLongLong()); };
        break;
    default: {
        if (_Is<Mrl>(userType)) {
            m_sqlType = u"TEXT PRIMARY KEY NOT NULL"_q;
            m_v2d = [] (const QVariant &value)
                { return QVariant(value.value<Mrl>().toString()); };
            m_d2v = [] (const QVariant &sqlData,int type) {
                if (sqlData.isNull() || !sqlData.isValid() || !_Is<Mrl>(type))
                    return QVariant();
                return QVariant::fromValue(Mrl::fromString(sqlData.toString()));
            };
            break;
        }
        m_sqlType = u"TEXT"_q;
        switch (_JsonType(userType)) {
        case QJsonValue::String:
            m_v2d = [] (const QVariant &value) -> QVariant
                { return _JsonFromQVariant(value).toString(); };
            m_d2v = [] (const QVariant &data, int type) -> QVariant {
                if (data.userType() != QMetaType::QString) return QVariant();
                return _JsonToQVariant(data.toString(), type);
            };
            break;
        case QJsonValue::Array:
            m_v2d = [] (const QVariant &value) -> QVariant
                { return QString::fromUtf8(QJsonDocument(_JsonFromQVariant(value).toArray()).toJson()); };
            m_d2v = [] (const QVariant &data, int type) -> QVariant {
                QJsonParseError e;
                const auto doc = QJsonDocument::fromJson(data.toString().toUtf8(), &e);
                if (e.error || !doc.isArray())
                    return QVariant();
                return _JsonToQVariant(doc.array(), type);
            };
            break;
        case QJsonValue::Object:
            m_v2d = [] (const QVariant &value) -> QVariant
                { return QString::fromUtf8(QJsonDocument(_JsonFromQVariant(value).toObject()).toJson()); };
            m_d2v = [] (const QVariant &data, int type) -> QVariant {
                QJsonParseError e;
                const auto doc = QJsonDocument::fromJson(data.toString().toUtf8(), &e);
                if (e.error || !doc.isObject())
                    return QVariant();
                return _JsonToQVariant(doc.object(), type);
            };
            break;
        default:
            Q_ASSERT(false);
        }
    }}
}

/******************************************************************************/

auto MrlStateSqlFieldList::clear() -> void
{
    for (auto &q : m_queries)
        q.clear();
    m_fields.clear();
    m_where = MrlStateSqlField();
}

auto MrlStateSqlFieldList::prepareInsert(const QString &table) -> QString
{
    auto &insert = m_queries[Insert];
    insert.clear();
    if (m_fields.isEmpty())
        return QString();
    const auto phs = _ToStringList(m_fields, [&] (const MrlStateSqlField &) {
        return QString('?'_q);
    }).join(','_q);
    const auto cols = _ToStringList(m_fields, [&] (const MrlStateSqlField &f) {
        return QString::fromLatin1(f.property().name());
    }).join(','_q);
    insert = u"INSERT OR REPLACE INTO %1 (%2) VALUES (%3)"_q
            .arg(table).arg(cols).arg(phs);
    return insert;
}

auto MrlStateSqlFieldList::field(const QString &name) const -> Field
{
    for (auto &field : m_fields) {
        if (!name.compare(_L(field.property().name())))
            return field;
    }
    return MrlStateSqlField();
}

auto MrlStateSqlFieldList::prepareSelect(const QString &table,
                                         const Field &where) -> QString
{
    auto &select = m_queries[Select];
    select.clear();
    if (m_fields.isEmpty() || !where.isValid())
        return QString();
    m_where = where;
    const auto columns = _ToStringList(m_fields, [&] (const auto &field) {
        return QString::fromLatin1(field.property().name());
    }).join(','_q);
    select = u"SELECT %1 FROM %2 WHERE %3 = ?"_q
            .arg(columns).arg(table).arg(_L(m_where.property().name()));
    return select;
}

auto MrlStateSqlFieldList::prepareUpdate(const QString &table, const Field &where) -> QString
{
    auto &update = m_queries[Update];
    update.clear();
    if (m_fields.isEmpty())
        return QString();
    m_updateWhere = where;
    const auto sets = _ToStringList(m_fields, [&] (const auto &field) {
        return QString(_L(field.property().name()) % "=?"_a);
    }).join(','_q);
    update = u"UPDATE %1 SET %2 WHERE %3 = ?"_q
            .arg(table, sets, _L(m_updateWhere.property().name()));
    return update;
}

auto MrlStateSqlFieldList::update(QSqlQuery &query, const QObject *o) -> bool
{
    auto &update = m_queries[Update];
    if (update.isEmpty())
        return false;
    if (!query.prepare(update))
        return false;
    for (int i = 0; i < m_fields.size(); ++i) {
        auto &f = m_fields[i];
        query.bindValue(i, f.sqlData(f.property().read(o)));
    }
    const auto where = m_updateWhere.sqlData(m_updateWhere.property().read(o));
    query.bindValue(m_fields.size(), where);
    return query.exec();
}

auto MrlStateSqlFieldList::select(QSqlQuery &query, QObject *object,
                                  const QVariant &where) const -> bool
{
    auto &select = m_queries[Select];
    if (select.isEmpty())
        return false;
    if (!query.prepare(select))
        return false;
    query.addBindValue(m_where.sqlData(where));
    if (!query.exec() || !query.next())
        return false;
    const auto record = query.record();
    for (int i = 0; i < m_fields.size(); ++i) {
        Q_ASSERT(_L(m_fields[i].property().name()) == record.fieldName(i));
        m_fields[i].exportTo(object, record.value(i));
    }
    return true;
}

auto MrlStateSqlFieldList::insert(QSqlQuery &query, const QObject *o) -> bool
{
    if (!isInsertPrepared())
        return false;
    if (!query.prepare(m_queries[Insert]))
        return false;
    for (int i=0; i<m_fields.size(); ++i) {
        auto &f = m_fields[i];
        query.bindValue(i, f.sqlData(f.property().read(o)));
    }
    return query.exec();
}
