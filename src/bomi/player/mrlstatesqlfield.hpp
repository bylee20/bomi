#ifndef MRLSTATESQLFIELD_HPP
#define MRLSTATESQLFIELD_HPP

#include <QMetaProperty>

struct MrlStateSqlField;

struct MrlStateSqlField {
    MrlStateSqlField() noexcept { }
    MrlStateSqlField(const QMetaProperty &property, const QVariant &def) noexcept;
    auto type() const -> QString { return m_sqlType; }
    const QMetaProperty &property() const { return m_property; }
    const QVariant &default_() const { return m_defaultValue; }
    template<class T>
    auto sqlData(T*) const -> void; // error
    auto sqlData(const QVariant &value) const noexcept -> QVariant
    { return m_v2d(value); }
    auto exportTo(QObject *state, const QVariant &sqlData) const -> bool
    {
        const auto var = m_d2v(sqlData, m_defaultValue.userType());
        return m_property.write(state, var.isValid() ? var : m_defaultValue);
    }
    auto isValid() const -> bool { return m_v2d && m_d2v; }
private:
    QMetaProperty m_property;
    QString m_sqlType;
    QVariant m_defaultValue;
    QVariant(*m_v2d)(const QVariant&) = nullptr;
    QVariant(*m_d2v)(const QVariant&,int) = nullptr;
    friend class MrlStateSqlFieldList;
};

class QSqlQuery;                        class QSqlDatabase;

class MrlStateSqlFieldList {
    using Field = MrlStateSqlField;
public:
    enum QueryType { Insert, Update, Select, MaxType };
    using iterator = QVector<Field>::iterator;
    using const_iterator =QVector<Field>::const_iterator;
    auto begin() const -> const_iterator { return m_fields.begin(); }
    auto begin() -> iterator { return m_fields.begin(); }
    auto end() const -> const_iterator { return m_fields.end(); }
    auto end() -> iterator { return m_fields.end(); }
    auto size() const -> int { return m_fields.size(); }
    auto isEmpty() const -> bool { return m_fields.isEmpty(); }
    auto clear() -> void;
    auto front() const -> const Field& { return m_fields.front(); }
    auto back() const -> const Field& { return m_fields.back(); }
    auto reserve(int size) -> void { m_fields.reserve(size); }
    auto push_back(const QMetaProperty &property, const QVariant &def) -> void
        { m_fields.push_back({ property, def }); }
    auto push_back(const Field &field) -> void { m_fields.push_back(field); }
    auto prepareInsert(const QString &table) -> QString;
    auto prepareUpdate(const QString &table, const Field &where) -> QString;
    auto prepareSelect(const QString &table, const Field &where) -> QString;
    auto field(const QString &name) const -> Field;
    auto insert(QSqlQuery &query, const QObject *object) -> bool;
    auto update(QSqlQuery &query, const QObject *object) -> bool;
    auto select(QSqlQuery &query, QObject *object) const -> bool
        { return select(query, object, m_where.property().read(object)); }
    auto select(QSqlQuery &q, QObject *o, const QVariant &where) const -> bool;
    template<class T>
    auto select(QSqlQuery &query, QObject *object, const T &t) const -> bool
        { return select(query, object, QVariant::fromValue<T>(t)); }
    auto isInsertPrepared() const -> bool { return isPrepared(Insert); }
    auto isUpdatePrepared() const -> bool { return isPrepared(Update); }
    auto isSelectPrepared() const -> bool { return isPrepared(Select); }
    auto isPrepared(QueryType type) const -> bool
        { return !m_queries[type].isEmpty(); }
    auto query(QueryType type) -> QString const { return m_queries[type]; }
private:
    QVector<QString> m_queries = QVector<QString>(MaxType);
    QVector<Field> m_fields;
    MrlStateSqlField m_where, m_updateWhere;
};

#endif // MRLSTATESQLFIELD_HPP
