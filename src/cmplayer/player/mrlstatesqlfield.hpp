#ifndef MRLSTATESQLFIELD_HPP
#define MRLSTATESQLFIELD_HPP

#include "stdafx.hpp"
#include <functional>

struct MrlStateSqlField;

struct MrlStateSqlField {
    using List = QList<MrlStateSqlField>;
    MrlStateSqlField() noexcept { }
    MrlStateSqlField(const QMetaProperty &property,
                     const QVariant &def = QVariant()) noexcept;
    auto type() const -> QString { return m_sqlType; }
    const QMetaProperty &property() const { return m_property; }
    const QVariant &default_() const { return m_defaultValue; }
    auto sqlData(const QObject *state) const noexcept -> QVariant
        { return sqlData(m_property.read(state)); }
    auto sqlData(const QVariant &value) const noexcept -> QVariant
        { return m_v2d(value); }
    auto exportTo(QObject *state, const QVariant &sqlData) const -> bool
    {
        const auto var = m_d2v(sqlData);
        return m_property.write(state, var.isValid() ? var : m_defaultValue);
    }
    auto isValid() const -> bool { return m_v2d && m_d2v; }
private:
    using ConvertVariant = std::function<QVariant(QVariant)>;
    QMetaProperty m_property;
    QString m_sqlType;
    QVariant m_defaultValue;
    ConvertVariant m_v2d = nullptr, m_d2v = nullptr;
};

class MrlStateSqlFieldList {
    using Field = MrlStateSqlField;
public:
    enum QueryType { Insert, Select, MaxType };
    using iterator = QVector<Field>::iterator;
    using const_iterator =QVector<Field>::const_iterator;
    auto begin() const -> const_iterator { return m_fields.begin(); }
    auto begin() -> iterator { return m_fields.begin(); }
    auto end() const -> const_iterator { return m_fields.end(); }
    auto end() -> iterator { return m_fields.end(); }
    auto size() const -> int { return m_fields.size(); }
    auto clear() -> void;
    auto reserve(int size) -> void { m_fields.reserve(size); }
    auto push_back(const QMetaProperty &property, const QVariant &def) -> void
        { m_fields.push_back({ property, def }); }
    auto push_back(const Field &field) -> void { m_fields.push_back(field); }
    auto prepareInsert(const QString &table) -> QString;
    auto prepareSelect(const QString &table, const Field &where) -> QString;
    auto field(const QString &name) const -> Field;
    auto insert(QSqlQuery &query, const QObject *object) -> bool;
    auto select(QSqlQuery &query, QObject *object) const -> bool
        { return select(query, object, m_where.sqlData(object)); }
    auto select(QSqlQuery &q, QObject *o, const QVariant &where) const -> bool;
    template<class T>
    auto select(QSqlQuery &query, QObject *object, const T &t) const -> bool
        { return select(query, object, QVariant::fromValue<T>(t)); }
    auto isInsertPrepared() const -> bool { return isPrepared(Insert); }
    auto isSelectPrepared() const -> bool { return isPrepared(Select); }
    auto isPrepared(QueryType type) const -> bool
        { return !m_queries[type].isEmpty(); }
private:
    QVector<QString> m_queries = QVector<QString>(MaxType);
    QVector<Field> m_fields;
    MrlStateSqlField m_where;
};

#endif // MRLSTATESQLFIELD_HPP
