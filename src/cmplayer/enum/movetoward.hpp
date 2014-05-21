#ifndef MOVETOWARD_HPP
#define MOVETOWARD_HPP

#include "enums.hpp"

enum class MoveToward : int {
    Reset = (int)0,
    Upward = (int)1,
    Downward = (int)2,
    Leftward = (int)3,
    Rightward = (int)4
};

inline auto operator == (MoveToward e, int i) -> bool { return (int)e == i; }
inline auto operator != (MoveToward e, int i) -> bool { return (int)e != i; }
inline auto operator == (int i, MoveToward e) -> bool { return (int)e == i; }
inline auto operator != (int i, MoveToward e) -> bool { return (int)e != i; }
inline auto operator & (MoveToward e, int i) -> int { return (int)e & i; }
inline auto operator & (int i, MoveToward e) -> int { return (int)e & i; }
inline auto operator &= (int &i, MoveToward e) -> int& { return i &= (int)e; }
inline auto operator ~ (MoveToward e) -> int { return ~(int)e; }
inline auto operator | (MoveToward e, int i) -> int { return (int)e | i; }
inline auto operator | (int i, MoveToward e) -> int { return (int)e | i; }
constexpr inline auto operator | (MoveToward e1, MoveToward e2) -> int { return (int)e1 | (int)e2; }
inline auto operator |= (int &i, MoveToward e) -> int& { return i |= (int)e; }
inline auto operator > (MoveToward e, int i) -> bool { return (int)e > i; }
inline auto operator < (MoveToward e, int i) -> bool { return (int)e < i; }
inline auto operator >= (MoveToward e, int i) -> bool { return (int)e >= i; }
inline auto operator <= (MoveToward e, int i) -> bool { return (int)e <= i; }
inline auto operator > (int i, MoveToward e) -> bool { return i > (int)e; }
inline auto operator < (int i, MoveToward e) -> bool { return i < (int)e; }
inline auto operator >= (int i, MoveToward e) -> bool { return i >= (int)e; }
inline auto operator <= (int i, MoveToward e) -> bool { return i <= (int)e; }

Q_DECLARE_METATYPE(MoveToward)

template<>
class EnumInfo<MoveToward> {
    typedef MoveToward Enum;
public:
    typedef MoveToward type;
    using Data =  QPoint;
    struct Item {
        Enum value;
        QString name, key;
        QPoint data;
    };
    using ItemList = std::array<Item, 5>;
    static constexpr auto size() -> int
    { return 5; }
    static constexpr auto typeName() -> const char*
    { return "MoveToward"; }
    static constexpr auto typeKey() -> const char*
    { return "move"; }
    static auto typeDescription() -> QString
    { return qApp->translate("EnumInfo", ""); }
    static auto item(Enum e) -> const Item*
    { return 0 <= e && e < size() ? &info[(int)e] : nullptr; }
    static auto name(Enum e) -> QString
    { auto i = item(e); return i ? i->name : QString(); }
    static auto key(Enum e) -> QString
    { auto i = item(e); return i ? i->key : QString(); }
    static auto data(Enum e) -> QPoint
    { auto i = item(e); return i ? i->data : QPoint(); }
    static auto description(int e) -> QString
    { return description((Enum)e); }
    static auto description(Enum e) -> QString
    {
        switch (e) {
        case Enum::Reset: return qApp->translate("EnumInfo", "Reset");
        case Enum::Upward: return qApp->translate("EnumInfo", "Upward");
        case Enum::Downward: return qApp->translate("EnumInfo", "Downward");
        case Enum::Leftward: return qApp->translate("EnumInfo", "Leftward");
        case Enum::Rightward: return qApp->translate("EnumInfo", "Rightward");
        default: return "";
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
    static auto fromData(const QPoint &data,
                         Enum def = default_()) -> Enum
    {
        auto it = std::find_if(info.cbegin(), info.cend(),
                               [&data] (const Item &item)
                               { return item.data == data; });
        return it != info.cend() ? it->value : def;
    }
    static constexpr auto default_() -> Enum
    { return MoveToward::Reset; }
private:
    static const ItemList info;
};

using MoveTowardInfo = EnumInfo<MoveToward>;

#endif
