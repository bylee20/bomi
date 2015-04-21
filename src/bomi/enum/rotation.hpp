#ifndef ROTATION_HPP
#define ROTATION_HPP

#include "enums.hpp"
#define ROTATION_IS_FLAG 0

enum class Rotation : int {
    D0 = (int)0,
    D90 = (int)1,
    D180 = (int)2,
    D270 = (int)3
};

Q_DECLARE_METATYPE(Rotation)

constexpr inline auto operator == (Rotation e, int i) -> bool { return (int)e == i; }
constexpr inline auto operator != (Rotation e, int i) -> bool { return (int)e != i; }
constexpr inline auto operator == (int i, Rotation e) -> bool { return (int)e == i; }
constexpr inline auto operator != (int i, Rotation e) -> bool { return (int)e != i; }
constexpr inline auto operator > (Rotation e, int i) -> bool { return (int)e > i; }
constexpr inline auto operator < (Rotation e, int i) -> bool { return (int)e < i; }
constexpr inline auto operator >= (Rotation e, int i) -> bool { return (int)e >= i; }
constexpr inline auto operator <= (Rotation e, int i) -> bool { return (int)e <= i; }
constexpr inline auto operator > (int i, Rotation e) -> bool { return i > (int)e; }
constexpr inline auto operator < (int i, Rotation e) -> bool { return i < (int)e; }
constexpr inline auto operator >= (int i, Rotation e) -> bool { return i >= (int)e; }
constexpr inline auto operator <= (int i, Rotation e) -> bool { return i <= (int)e; }
#if ROTATION_IS_FLAG
#include "enumflags.hpp"
using  = EnumFlags<Rotation>;
constexpr inline auto operator | (Rotation e1, Rotation e2) -> 
{ return (::IntType(e1) | ::IntType(e2)); }
constexpr inline auto operator ~ (Rotation e) -> EnumNot<Rotation>
{ return EnumNot<Rotation>(e); }
constexpr inline auto operator & (Rotation lhs,  rhs) -> EnumAnd<Rotation>
{ return rhs & lhs; }
Q_DECLARE_METATYPE()
#endif

template<>
class EnumInfo<Rotation> {
    typedef Rotation Enum;
public:
    typedef Rotation type;
    using Data =  int;
    struct Item {
        Enum value;
        QString name, key;
        int data;
    };
    using ItemList = std::array<Item, 4>;
    static constexpr auto size() -> int
    { return 4; }
    static constexpr auto typeName() -> const char*
    { return "Rotation"; }
    static constexpr auto typeKey() -> const char*
    { return "rotate"; }
    static auto typeDescription() -> QString
    { return qApp->translate("EnumInfo", "Rotate"); }
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
        case Enum::D0: return qApp->translate("EnumInfo", "0°");
        case Enum::D90: return qApp->translate("EnumInfo", "90°");
        case Enum::D180: return qApp->translate("EnumInfo", "180°");
        case Enum::D270: return qApp->translate("EnumInfo", "270°");
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
    { return Rotation::D0; }
private:
    static const ItemList info;
};

using RotationInfo = EnumInfo<Rotation>;

#endif
