#ifndef DITHERING_HPP
#define DITHERING_HPP

#include "enums.hpp"
#define DITHERING_IS_FLAG 0

enum class Dithering : int {
    None = (int)0,
    Fruit = (int)1,
    Ordered = (int)2
};

Q_DECLARE_METATYPE(Dithering)

constexpr inline auto operator == (Dithering e, int i) -> bool { return (int)e == i; }
constexpr inline auto operator != (Dithering e, int i) -> bool { return (int)e != i; }
constexpr inline auto operator == (int i, Dithering e) -> bool { return (int)e == i; }
constexpr inline auto operator != (int i, Dithering e) -> bool { return (int)e != i; }
constexpr inline auto operator > (Dithering e, int i) -> bool { return (int)e > i; }
constexpr inline auto operator < (Dithering e, int i) -> bool { return (int)e < i; }
constexpr inline auto operator >= (Dithering e, int i) -> bool { return (int)e >= i; }
constexpr inline auto operator <= (Dithering e, int i) -> bool { return (int)e <= i; }
constexpr inline auto operator > (int i, Dithering e) -> bool { return i > (int)e; }
constexpr inline auto operator < (int i, Dithering e) -> bool { return i < (int)e; }
constexpr inline auto operator >= (int i, Dithering e) -> bool { return i >= (int)e; }
constexpr inline auto operator <= (int i, Dithering e) -> bool { return i <= (int)e; }
#if DITHERING_IS_FLAG
#include "enumflags.hpp"
using  = EnumFlags<Dithering>;
constexpr inline auto operator | (Dithering e1, Dithering e2) -> 
{ return (::IntType(e1) | ::IntType(e2)); }
constexpr inline auto operator ~ (Dithering e) -> EnumNot<Dithering>
{ return EnumNot<Dithering>(e); }
constexpr inline auto operator & (Dithering lhs,  rhs) -> EnumAnd<Dithering>
{ return rhs & lhs; }
Q_DECLARE_METATYPE()
#endif

template<>
class EnumInfo<Dithering> {
    typedef Dithering Enum;
public:
    typedef Dithering type;
    using Data =  QByteArray;
    struct Item {
        Enum value;
        QString name, key;
        QByteArray data;
    };
    using ItemList = std::array<Item, 3>;
    static constexpr auto size() -> int
    { return 3; }
    static constexpr auto typeName() -> const char*
    { return "Dithering"; }
    static constexpr auto typeKey() -> const char*
    { return "dithering"; }
    static auto typeDescription() -> QString
    { return qApp->translate("EnumInfo", "Dithering"); }
    static auto item(Enum e) -> const Item*
    { return 0 <= e && e < size() ? &info[(int)e] : nullptr; }
    static auto name(Enum e) -> QString
    { auto i = item(e); return i ? i->name : QString(); }
    static auto key(Enum e) -> QString
    { auto i = item(e); return i ? i->key : QString(); }
    static auto data(Enum e) -> QByteArray
    { auto i = item(e); return i ? i->data : QByteArray(); }
    static auto description(int e) -> QString
    { return description((Enum)e); }
    static auto description(Enum e) -> QString
    {
        switch (e) {
        case Enum::None: return qApp->translate("EnumInfo", "Off");
        case Enum::Fruit: return qApp->translate("EnumInfo", "Random Dithering");
        case Enum::Ordered: return qApp->translate("EnumInfo", "Ordered Dithering");
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
    static auto fromData(const QByteArray &data,
                         Enum def = default_()) -> Enum
    {
        auto it = std::find_if(info.cbegin(), info.cend(),
                               [&data] (const Item &item)
                               { return item.data == data; });
        return it != info.cend() ? it->value : def;
    }
    static constexpr auto default_() -> Enum
    { return Dithering::None; }
private:
    static const ItemList info;
};

using DitheringInfo = EnumInfo<Dithering>;

#endif
