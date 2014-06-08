#ifndef VERTICALALIGNMENT_HPP
#define VERTICALALIGNMENT_HPP

#include "enums.hpp"
#define VERTICALALIGNMENT_IS_FLAG 0

enum class VerticalAlignment : int {
    Top = (int)0,
    Center = (int)1,
    Bottom = (int)2
};

Q_DECLARE_METATYPE(VerticalAlignment)

constexpr inline auto operator == (VerticalAlignment e, int i) -> bool { return (int)e == i; }
constexpr inline auto operator != (VerticalAlignment e, int i) -> bool { return (int)e != i; }
constexpr inline auto operator == (int i, VerticalAlignment e) -> bool { return (int)e == i; }
constexpr inline auto operator != (int i, VerticalAlignment e) -> bool { return (int)e != i; }
constexpr inline auto operator > (VerticalAlignment e, int i) -> bool { return (int)e > i; }
constexpr inline auto operator < (VerticalAlignment e, int i) -> bool { return (int)e < i; }
constexpr inline auto operator >= (VerticalAlignment e, int i) -> bool { return (int)e >= i; }
constexpr inline auto operator <= (VerticalAlignment e, int i) -> bool { return (int)e <= i; }
constexpr inline auto operator > (int i, VerticalAlignment e) -> bool { return i > (int)e; }
constexpr inline auto operator < (int i, VerticalAlignment e) -> bool { return i < (int)e; }
constexpr inline auto operator >= (int i, VerticalAlignment e) -> bool { return i >= (int)e; }
constexpr inline auto operator <= (int i, VerticalAlignment e) -> bool { return i <= (int)e; }
#if VERTICALALIGNMENT_IS_FLAG
#include "enumflags.hpp"
using  = EnumFlags<VerticalAlignment>;
constexpr inline auto operator | (VerticalAlignment e1, VerticalAlignment e2) -> 
{ return (::IntType(e1) | ::IntType(e2)); }
constexpr inline auto operator ~ (VerticalAlignment e) -> EnumNot<VerticalAlignment>
{ return EnumNot<VerticalAlignment>(e); }
constexpr inline auto operator & (VerticalAlignment lhs,  rhs) -> EnumAnd<VerticalAlignment>
{ return rhs & lhs; }
Q_DECLARE_METATYPE()
#endif

template<>
class EnumInfo<VerticalAlignment> {
    typedef VerticalAlignment Enum;
public:
    typedef VerticalAlignment type;
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
    { return "VerticalAlignment"; }
    static constexpr auto typeKey() -> const char*
    { return "vertical-alignment"; }
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
        case Enum::Top: return qApp->translate("EnumInfo", "Top");
        case Enum::Center: return qApp->translate("EnumInfo", "Vertical Center");
        case Enum::Bottom: return qApp->translate("EnumInfo", "Bottom");
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
    { return VerticalAlignment::Center; }
private:
    static const ItemList info;
};

using VerticalAlignmentInfo = EnumInfo<VerticalAlignment>;

#endif
