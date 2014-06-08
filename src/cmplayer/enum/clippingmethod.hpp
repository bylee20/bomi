#ifndef CLIPPINGMETHOD_HPP
#define CLIPPINGMETHOD_HPP

#include "enums.hpp"
#define CLIPPINGMETHOD_IS_FLAG 0

enum class ClippingMethod : int {
    Auto = (int)0,
    Soft = (int)1,
    Hard = (int)2
};

Q_DECLARE_METATYPE(ClippingMethod)

constexpr inline auto operator == (ClippingMethod e, int i) -> bool { return (int)e == i; }
constexpr inline auto operator != (ClippingMethod e, int i) -> bool { return (int)e != i; }
constexpr inline auto operator == (int i, ClippingMethod e) -> bool { return (int)e == i; }
constexpr inline auto operator != (int i, ClippingMethod e) -> bool { return (int)e != i; }
constexpr inline auto operator > (ClippingMethod e, int i) -> bool { return (int)e > i; }
constexpr inline auto operator < (ClippingMethod e, int i) -> bool { return (int)e < i; }
constexpr inline auto operator >= (ClippingMethod e, int i) -> bool { return (int)e >= i; }
constexpr inline auto operator <= (ClippingMethod e, int i) -> bool { return (int)e <= i; }
constexpr inline auto operator > (int i, ClippingMethod e) -> bool { return i > (int)e; }
constexpr inline auto operator < (int i, ClippingMethod e) -> bool { return i < (int)e; }
constexpr inline auto operator >= (int i, ClippingMethod e) -> bool { return i >= (int)e; }
constexpr inline auto operator <= (int i, ClippingMethod e) -> bool { return i <= (int)e; }
#if CLIPPINGMETHOD_IS_FLAG
#include "enumflags.hpp"
using  = EnumFlags<ClippingMethod>;
constexpr inline auto operator | (ClippingMethod e1, ClippingMethod e2) -> 
{ return (::IntType(e1) | ::IntType(e2)); }
constexpr inline auto operator ~ (ClippingMethod e) -> EnumNot<ClippingMethod>
{ return EnumNot<ClippingMethod>(e); }
constexpr inline auto operator & (ClippingMethod lhs,  rhs) -> EnumAnd<ClippingMethod>
{ return rhs & lhs; }
Q_DECLARE_METATYPE()
#endif

template<>
class EnumInfo<ClippingMethod> {
    typedef ClippingMethod Enum;
public:
    typedef ClippingMethod type;
    using Data =  QVariant;
    struct Item {
        Enum value;
        QString name, key;
        QVariant data;
    };
    using ItemList = std::array<Item, 3>;
    static constexpr auto size() -> int
    { return 3; }
    static constexpr auto typeName() -> const char*
    { return "ClippingMethod"; }
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
        case Enum::Auto: return qApp->translate("EnumInfo", "Auto-clipping");
        case Enum::Soft: return qApp->translate("EnumInfo", "Soft-clipping");
        case Enum::Hard: return qApp->translate("EnumInfo", "Hard-clipping");
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
    { return ClippingMethod::Auto; }
private:
    static const ItemList info;
};

using ClippingMethodInfo = EnumInfo<ClippingMethod>;

#endif
