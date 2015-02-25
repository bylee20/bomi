#ifndef DEINTMETHOD_HPP
#define DEINTMETHOD_HPP

#include "enums.hpp"
#define DEINTMETHOD_IS_FLAG 0

enum class DeintMethod : int {
    None = (int)0,
    Bob = (int)1,
    LinearBob = (int)2,
    CubicBob = (int)3,
    Yadif = (int)4
};

Q_DECLARE_METATYPE(DeintMethod)

constexpr inline auto operator == (DeintMethod e, int i) -> bool { return (int)e == i; }
constexpr inline auto operator != (DeintMethod e, int i) -> bool { return (int)e != i; }
constexpr inline auto operator == (int i, DeintMethod e) -> bool { return (int)e == i; }
constexpr inline auto operator != (int i, DeintMethod e) -> bool { return (int)e != i; }
constexpr inline auto operator > (DeintMethod e, int i) -> bool { return (int)e > i; }
constexpr inline auto operator < (DeintMethod e, int i) -> bool { return (int)e < i; }
constexpr inline auto operator >= (DeintMethod e, int i) -> bool { return (int)e >= i; }
constexpr inline auto operator <= (DeintMethod e, int i) -> bool { return (int)e <= i; }
constexpr inline auto operator > (int i, DeintMethod e) -> bool { return i > (int)e; }
constexpr inline auto operator < (int i, DeintMethod e) -> bool { return i < (int)e; }
constexpr inline auto operator >= (int i, DeintMethod e) -> bool { return i >= (int)e; }
constexpr inline auto operator <= (int i, DeintMethod e) -> bool { return i <= (int)e; }
#if DEINTMETHOD_IS_FLAG
#include "enumflags.hpp"
using  = EnumFlags<DeintMethod>;
constexpr inline auto operator | (DeintMethod e1, DeintMethod e2) -> 
{ return (::IntType(e1) | ::IntType(e2)); }
constexpr inline auto operator ~ (DeintMethod e) -> EnumNot<DeintMethod>
{ return EnumNot<DeintMethod>(e); }
constexpr inline auto operator & (DeintMethod lhs,  rhs) -> EnumAnd<DeintMethod>
{ return rhs & lhs; }
Q_DECLARE_METATYPE()
#endif

template<>
class EnumInfo<DeintMethod> {
    typedef DeintMethod Enum;
public:
    typedef DeintMethod type;
    using Data =  QVariant;
    struct Item {
        Enum value;
        QString name, key;
        QVariant data;
    };
    using ItemList = std::array<Item, 5>;
    static constexpr auto size() -> int
    { return 5; }
    static constexpr auto typeName() -> const char*
    { return "DeintMethod"; }
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
        case Enum::None: return qApp->translate("EnumInfo", "");
        case Enum::Bob: return qApp->translate("EnumInfo", "");
        case Enum::LinearBob: return qApp->translate("EnumInfo", "");
        case Enum::CubicBob: return qApp->translate("EnumInfo", "");
        case Enum::Yadif: return qApp->translate("EnumInfo", "");
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
    { return DeintMethod::None; }
private:
    static const ItemList info;
};

using DeintMethodInfo = EnumInfo<DeintMethod>;

#endif
