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
static inline auto _EnumData(Enum e) -> EnumData<Enum>
    { return EnumInfo<Enum>::data(e); }

template<class T>
static inline auto _EnumName(T t) -> QString { return EnumInfo<T>::name(t); }
template<class T>
static inline auto _EnumFrom(const QString &name,
                             T def = EnumInfo<T>::default_()) -> T
    { return EnumInfo<T>::from(name, def); }

template<class T>
auto _EnumVariantToEnumName(const QVariant &var) -> QString
{
    Q_ASSERT(var.userType() == qMetaTypeId<T>());
    return _EnumName<T>(var.value<T>());
}

template<class T>
auto _EnumNameToEnumVariant(const QString &name) -> QVariant
{
    return QVariant::fromValue<T>(_EnumFrom<T>(name));
}

struct EnumNameVariantConverter
{
    auto (*variantToName)(const QVariant&) -> QString = nullptr;
    auto (*nameToVariant)(const QString&) -> QVariant = nullptr;
    auto isNull() const -> bool { return !variantToName || !nameToVariant; }
};

auto _EnumNameVariantConverter(int metaType) -> EnumNameVariantConverter;

bool _GetEnumFunctionsForSql(int varType, EnumVariantToSqlFunc &toSql, EnumVariantFromSqlFunc &fromSql);
bool _IsEnumTypeId(int userType);

#endif

