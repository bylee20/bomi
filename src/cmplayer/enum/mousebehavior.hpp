#ifndef MOUSEBEHAVIOR_HPP
#define MOUSEBEHAVIOR_HPP

#include "enums.hpp"
#define MOUSEBEHAVIOR_IS_FLAG 0

enum class MouseBehavior : int {
    DoubleClick = (int)0,
    MiddleClick = (int)1,
    ScrollUp = (int)2,
    ScrollDown = (int)3,
    Extra1Click = (int)4,
    Extra2Click = (int)5,
    Extra3Click = (int)6,
    Extra4Click = (int)7,
    Extra5Click = (int)8,
    Extra6Click = (int)9,
    Extra7Click = (int)10,
    Extra8Click = (int)11,
    Extra9Click = (int)12
};

Q_DECLARE_METATYPE(MouseBehavior)

constexpr inline auto operator == (MouseBehavior e, int i) -> bool { return (int)e == i; }
constexpr inline auto operator != (MouseBehavior e, int i) -> bool { return (int)e != i; }
constexpr inline auto operator == (int i, MouseBehavior e) -> bool { return (int)e == i; }
constexpr inline auto operator != (int i, MouseBehavior e) -> bool { return (int)e != i; }
constexpr inline auto operator > (MouseBehavior e, int i) -> bool { return (int)e > i; }
constexpr inline auto operator < (MouseBehavior e, int i) -> bool { return (int)e < i; }
constexpr inline auto operator >= (MouseBehavior e, int i) -> bool { return (int)e >= i; }
constexpr inline auto operator <= (MouseBehavior e, int i) -> bool { return (int)e <= i; }
constexpr inline auto operator > (int i, MouseBehavior e) -> bool { return i > (int)e; }
constexpr inline auto operator < (int i, MouseBehavior e) -> bool { return i < (int)e; }
constexpr inline auto operator >= (int i, MouseBehavior e) -> bool { return i >= (int)e; }
constexpr inline auto operator <= (int i, MouseBehavior e) -> bool { return i <= (int)e; }
#if MOUSEBEHAVIOR_IS_FLAG
#include "enumflags.hpp"
using  = EnumFlags<MouseBehavior>;
constexpr inline auto operator | (MouseBehavior e1, MouseBehavior e2) -> 
{ return (::IntType(e1) | ::IntType(e2)); }
constexpr inline auto operator ~ (MouseBehavior e) -> EnumNot<MouseBehavior>
{ return EnumNot<MouseBehavior>(e); }
constexpr inline auto operator & (MouseBehavior lhs,  rhs) -> EnumAnd<MouseBehavior>
{ return rhs & lhs; }
Q_DECLARE_METATYPE()
#endif

template<>
class EnumInfo<MouseBehavior> {
    typedef MouseBehavior Enum;
public:
    typedef MouseBehavior type;
    using Data =  QVariant;
    struct Item {
        Enum value;
        QString name, key;
        QVariant data;
    };
    using ItemList = std::array<Item, 13>;
    static constexpr auto size() -> int
    { return 13; }
    static constexpr auto typeName() -> const char*
    { return "MouseBehavior"; }
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
        case Enum::DoubleClick:
            return qApp->translate("EnumInfo", "Double click");
        case Enum::MiddleClick:
            return qApp->translate("EnumInfo", "Middle button");
        case Enum::ScrollUp:
            return qApp->translate("EnumInfo", "Scroll up");
        case Enum::ScrollDown:
            return qApp->translate("EnumInfo", "Scroll down");
        default:
            break;
        }
        auto s = qApp->translate("EnumInfo", "Extra button %1");
        switch (e) {
            case Enum::Extra1Click: return s.arg(1);
            case Enum::Extra2Click: return s.arg(2);
            case Enum::Extra3Click: return s.arg(3);
            case Enum::Extra4Click: return s.arg(4);
            case Enum::Extra5Click: return s.arg(5);
            case Enum::Extra6Click: return s.arg(6);
            case Enum::Extra7Click: return s.arg(7);
            case Enum::Extra8Click: return s.arg(8);
            case Enum::Extra9Click: return s.arg(9);
            default:                return QString();
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
    { return MouseBehavior::DoubleClick; }
private:
    static const ItemList info;
};

using MouseBehaviorInfo = EnumInfo<MouseBehavior>;

#endif
