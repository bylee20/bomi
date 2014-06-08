#ifndef INTERPOLATORTYPE_HPP
#define INTERPOLATORTYPE_HPP

#include "enums.hpp"
#define INTERPOLATORTYPE_IS_FLAG 0

enum class InterpolatorType : int {
    Bilinear = (int)0,
    BicubicBS = (int)1,
    BicubicCR = (int)2,
    BicubicMN = (int)3,
    Spline16 = (int)4,
    Spline36 = (int)5,
    Spline64 = (int)6,
    LanczosFast = (int)7,
    Lanczos2 = (int)8,
    Lanczos3 = (int)9,
    Lanczos4 = (int)10
};

Q_DECLARE_METATYPE(InterpolatorType)

constexpr inline auto operator == (InterpolatorType e, int i) -> bool { return (int)e == i; }
constexpr inline auto operator != (InterpolatorType e, int i) -> bool { return (int)e != i; }
constexpr inline auto operator == (int i, InterpolatorType e) -> bool { return (int)e == i; }
constexpr inline auto operator != (int i, InterpolatorType e) -> bool { return (int)e != i; }
constexpr inline auto operator > (InterpolatorType e, int i) -> bool { return (int)e > i; }
constexpr inline auto operator < (InterpolatorType e, int i) -> bool { return (int)e < i; }
constexpr inline auto operator >= (InterpolatorType e, int i) -> bool { return (int)e >= i; }
constexpr inline auto operator <= (InterpolatorType e, int i) -> bool { return (int)e <= i; }
constexpr inline auto operator > (int i, InterpolatorType e) -> bool { return i > (int)e; }
constexpr inline auto operator < (int i, InterpolatorType e) -> bool { return i < (int)e; }
constexpr inline auto operator >= (int i, InterpolatorType e) -> bool { return i >= (int)e; }
constexpr inline auto operator <= (int i, InterpolatorType e) -> bool { return i <= (int)e; }
#if INTERPOLATORTYPE_IS_FLAG
#include "enumflags.hpp"
using  = EnumFlags<InterpolatorType>;
constexpr inline auto operator | (InterpolatorType e1, InterpolatorType e2) -> 
{ return (::IntType(e1) | ::IntType(e2)); }
constexpr inline auto operator ~ (InterpolatorType e) -> EnumNot<InterpolatorType>
{ return EnumNot<InterpolatorType>(e); }
constexpr inline auto operator & (InterpolatorType lhs,  rhs) -> EnumAnd<InterpolatorType>
{ return rhs & lhs; }
Q_DECLARE_METATYPE()
#endif

template<>
class EnumInfo<InterpolatorType> {
    typedef InterpolatorType Enum;
public:
    typedef InterpolatorType type;
    using Data =  QVariant;
    struct Item {
        Enum value;
        QString name, key;
        QVariant data;
    };
    using ItemList = std::array<Item, 11>;
    static constexpr auto size() -> int
    { return 11; }
    static constexpr auto typeName() -> const char*
    { return "InterpolatorType"; }
    static constexpr auto typeKey() -> const char*
    { return ""; }
    static auto typeDescription() -> QString
    { return qApp->translate("EnumInfo", ""); }
    static auto item(Enum e) -> const Item*
    { return 0 <= e && e < size() ? &info[(int)e] : nullptr; }
    static auto name(Enum e) -> QString
    { auto i = item(e); return i ? i->name : QString(); }
    static auto key(Enum e) -> QString
    { auto i = item(e); return i ? i->key : QString(); }
    static auto data(Enum e) -> QVariant
    { auto i = item(e); return i ? i->data : QVariant(); }
    static auto description(int e) -> QString
    { return description((Enum)e); }
    static auto description(Enum e) -> QString
    {
        switch (e) {
        case Enum::Bilinear: return qApp->translate("EnumInfo", "Bilinear");
        case Enum::BicubicBS: return qApp->translate("EnumInfo", "B-Spline");
        case Enum::BicubicCR: return qApp->translate("EnumInfo", "Catmull-Rom");
        case Enum::BicubicMN: return qApp->translate("EnumInfo", "Mitchell-Netravali");
        case Enum::Spline16: return qApp->translate("EnumInfo", "2-Lobed Spline");
        case Enum::Spline36: return qApp->translate("EnumInfo", "3-Lobed Spline");
        case Enum::Spline64: return qApp->translate("EnumInfo", "4-Lobed Spline");
        case Enum::LanczosFast: return qApp->translate("EnumInfo", "Fast Lanczos");
        case Enum::Lanczos2: return qApp->translate("EnumInfo", "2-Lobed Lanczos");
        case Enum::Lanczos3: return qApp->translate("EnumInfo", "3-Lobed Lanczos");
        case Enum::Lanczos4: return qApp->translate("EnumInfo", "4-Lobed Lanczos");
        default: return QString();
        }
    }
    static constexpr auto items() -> const ItemList&
    { return info; }
    static auto from(int id, Enum def = default_()) -> Enum
    {
        auto it = std::find_if(info.cbegin(), info.cend(),
                               [id] (const Item &item)
                               { return item.value == id; });
        return it != info.cend() ? it->value : def;
    }
    static auto from(const QString &name, Enum def = default_()) -> Enum
    {
        auto it = std::find_if(info.cbegin(), info.cend(),
                               [&name] (const Item &item)
                               { return !name.compare(item.name); });
        return it != info.cend() ? it->value : def;
    }
    static auto fromName(Enum &val, const QString &name) -> bool
    {
        auto it = std::find_if(info.cbegin(), info.cend(),
                               [&name] (const Item &item)
                               { return !name.compare(item.name); });
        if (it == info.cend())
            return false;
        val = it->value;
        return true;
    }
    static auto fromData(const QVariant &data,
                         Enum def = default_()) -> Enum
    {
        auto it = std::find_if(info.cbegin(), info.cend(),
                               [&data] (const Item &item)
                               { return item.data == data; });
        return it != info.cend() ? it->value : def;
    }
    static constexpr auto default_() -> Enum
    { return InterpolatorType::Bilinear; }
private:
    static const ItemList info;
};

using InterpolatorTypeInfo = EnumInfo<InterpolatorType>;

#endif
