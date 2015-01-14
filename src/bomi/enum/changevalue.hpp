#ifndef CHANGEVALUE_HPP
#define CHANGEVALUE_HPP

#include "enums.hpp"
#define CHANGEVALUE_IS_FLAG 0

enum class ChangeValue : int {
    Reset = (int)0,
    Increase = (int)1,
    Decrease = (int)2
};

Q_DECLARE_METATYPE(ChangeValue)

constexpr inline auto operator == (ChangeValue e, int i) -> bool { return (int)e == i; }
constexpr inline auto operator != (ChangeValue e, int i) -> bool { return (int)e != i; }
constexpr inline auto operator == (int i, ChangeValue e) -> bool { return (int)e == i; }
constexpr inline auto operator != (int i, ChangeValue e) -> bool { return (int)e != i; }
constexpr inline auto operator > (ChangeValue e, int i) -> bool { return (int)e > i; }
constexpr inline auto operator < (ChangeValue e, int i) -> bool { return (int)e < i; }
constexpr inline auto operator >= (ChangeValue e, int i) -> bool { return (int)e >= i; }
constexpr inline auto operator <= (ChangeValue e, int i) -> bool { return (int)e <= i; }
constexpr inline auto operator > (int i, ChangeValue e) -> bool { return i > (int)e; }
constexpr inline auto operator < (int i, ChangeValue e) -> bool { return i < (int)e; }
constexpr inline auto operator >= (int i, ChangeValue e) -> bool { return i >= (int)e; }
constexpr inline auto operator <= (int i, ChangeValue e) -> bool { return i <= (int)e; }
#if CHANGEVALUE_IS_FLAG
#include "enumflags.hpp"
using  = EnumFlags<ChangeValue>;
constexpr inline auto operator | (ChangeValue e1, ChangeValue e2) -> 
{ return (::IntType(e1) | ::IntType(e2)); }
constexpr inline auto operator ~ (ChangeValue e) -> EnumNot<ChangeValue>
{ return EnumNot<ChangeValue>(e); }
constexpr inline auto operator & (ChangeValue lhs,  rhs) -> EnumAnd<ChangeValue>
{ return rhs & lhs; }
Q_DECLARE_METATYPE()
#endif

template<>
class EnumInfo<ChangeValue> {
    typedef ChangeValue Enum;
public:
    typedef ChangeValue type;
    using Data =  int;
    struct Item {
        Enum value;
        QString name, key;
        int data;
    };
    using ItemList = std::array<Item, 3>;
    static constexpr auto size() -> int
    { return 3; }
    static constexpr auto typeName() -> const char*
    { return "ChangeValue"; }
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
    static auto data(Enum e) -> int
    { auto i = item(e); return i ? i->data : int(); }
    static auto description(int e) -> QString
    { return description((Enum)e); }
    static auto description(Enum e) -> QString
    {
        switch (e) {
        case Enum::Reset: return qApp->translate("EnumInfo", "Reset");
        case Enum::Increase: return qApp->translate("EnumInfo", "Increase %1");
        case Enum::Decrease: return qApp->translate("EnumInfo", "Decrease %1");
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
    static auto fromData(const int &data,
                         Enum def = default_()) -> Enum
    {
        auto it = std::find_if(info.cbegin(), info.cend(),
                               [&data] (const Item &item)
                               { return item.data == data; });
        return it != info.cend() ? it->value : def;
    }
    static constexpr auto default_() -> Enum
    { return ChangeValue::Reset; }
private:
    static const ItemList info;
};

using ChangeValueInfo = EnumInfo<ChangeValue>;

#endif
