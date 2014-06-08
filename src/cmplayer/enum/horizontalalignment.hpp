#ifndef HORIZONTALALIGNMENT_HPP
#define HORIZONTALALIGNMENT_HPP

#include "enums.hpp"
#define HORIZONTALALIGNMENT_IS_FLAG 0

enum class HorizontalAlignment : int {
    Left = (int)0,
    Center = (int)1,
    Right = (int)2
};

Q_DECLARE_METATYPE(HorizontalAlignment)

constexpr inline auto operator == (HorizontalAlignment e, int i) -> bool { return (int)e == i; }
constexpr inline auto operator != (HorizontalAlignment e, int i) -> bool { return (int)e != i; }
constexpr inline auto operator == (int i, HorizontalAlignment e) -> bool { return (int)e == i; }
constexpr inline auto operator != (int i, HorizontalAlignment e) -> bool { return (int)e != i; }
constexpr inline auto operator > (HorizontalAlignment e, int i) -> bool { return (int)e > i; }
constexpr inline auto operator < (HorizontalAlignment e, int i) -> bool { return (int)e < i; }
constexpr inline auto operator >= (HorizontalAlignment e, int i) -> bool { return (int)e >= i; }
constexpr inline auto operator <= (HorizontalAlignment e, int i) -> bool { return (int)e <= i; }
constexpr inline auto operator > (int i, HorizontalAlignment e) -> bool { return i > (int)e; }
constexpr inline auto operator < (int i, HorizontalAlignment e) -> bool { return i < (int)e; }
constexpr inline auto operator >= (int i, HorizontalAlignment e) -> bool { return i >= (int)e; }
constexpr inline auto operator <= (int i, HorizontalAlignment e) -> bool { return i <= (int)e; }
#if HORIZONTALALIGNMENT_IS_FLAG
#include "enumflags.hpp"
using  = EnumFlags<HorizontalAlignment>;
constexpr inline auto operator | (HorizontalAlignment e1, HorizontalAlignment e2) -> 
{ return (::IntType(e1) | ::IntType(e2)); }
constexpr inline auto operator ~ (HorizontalAlignment e) -> EnumNot<HorizontalAlignment>
{ return EnumNot<HorizontalAlignment>(e); }
constexpr inline auto operator & (HorizontalAlignment lhs,  rhs) -> EnumAnd<HorizontalAlignment>
{ return rhs & lhs; }
Q_DECLARE_METATYPE()
#endif

template<>
class EnumInfo<HorizontalAlignment> {
    typedef HorizontalAlignment Enum;
public:
    typedef HorizontalAlignment type;
    using Data =  Qt::Alignment;
    struct Item {
        Enum value;
        QString name, key;
        Qt::Alignment data;
    };
    using ItemList = std::array<Item, 3>;
    static constexpr auto size() -> int
    { return 3; }
    static constexpr auto typeName() -> const char*
    { return "HorizontalAlignment"; }
    static constexpr auto typeKey() -> const char*
    { return "horizontal-alignment"; }
    static auto typeDescription() -> QString
    { return qApp->translate("EnumInfo", ""); }
    static auto item(Enum e) -> const Item*
    { return 0 <= e && e < size() ? &info[(int)e] : nullptr; }
    static auto name(Enum e) -> QString
    { auto i = item(e); return i ? i->name : QString(); }
    static auto key(Enum e) -> QString
    { auto i = item(e); return i ? i->key : QString(); }
    static auto data(Enum e) -> Qt::Alignment
    { auto i = item(e); return i ? i->data : Qt::Alignment(); }
    static auto description(int e) -> QString
    { return description((Enum)e); }
    static auto description(Enum e) -> QString
    {
        switch (e) {
        case Enum::Left: return qApp->translate("EnumInfo", "Left");
        case Enum::Center: return qApp->translate("EnumInfo", "Horizontal Center");
        case Enum::Right: return qApp->translate("EnumInfo", "Right");
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
    static auto fromData(const Qt::Alignment &data,
                         Enum def = default_()) -> Enum
    {
        auto it = std::find_if(info.cbegin(), info.cend(),
                               [&data] (const Item &item)
                               { return item.data == data; });
        return it != info.cend() ? it->value : def;
    }
    static constexpr auto default_() -> Enum
    { return HorizontalAlignment::Center; }
private:
    static const ItemList info;
};

using HorizontalAlignmentInfo = EnumInfo<HorizontalAlignment>;

#endif
