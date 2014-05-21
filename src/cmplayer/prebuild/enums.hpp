#ifndef ENUMS_HPP
#define ENUMS_HPP

#include <QCoreApplication>
#include <array>

#ifdef None
#undef None
#endif

template<class T>
class EnumInfo { static constexpr int size() { return 0; } double dummy; };

using EnumVariantToSqlFunc   = QString (*)(const QVariant &var);
using EnumVariantFromSqlFunc = QVariant(*)(const QVariant &var, const QVariant &def);

template<class T>
QString _EnumVariantToSql(const QVariant &var) {
    Q_ASSERT(var.userType() == qMetaTypeId<T>());
    return QLatin1Char('\'') % EnumInfo<T>::name(var.value<T>()) % QLatin1Char('\'');
}

template<class T>
QVariant _EnumVariantFromSql(const QVariant &name, const QVariant &def) {
    const auto enum_ = EnumInfo<T>::from(name.toString(), def.value<T>());
    return QVariant::fromValue<T>(enum_);
}

template<class Enum>
using EnumData = typename EnumInfo<Enum>::Data;

template<class Enum>
static inline auto _GetEnumData(Enum e) -> EnumData<Enum>
    { return EnumInfo<Enum>::data(e); }

bool _GetEnumFunctionsForSql(int varType, EnumVariantToSqlFunc &toSql, EnumVariantFromSqlFunc &fromSql);
bool _IsEnumTypeId(int userType);

#endif

