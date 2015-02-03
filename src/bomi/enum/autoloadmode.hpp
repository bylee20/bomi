#ifndef AUTOLOADMODE_HPP
#define AUTOLOADMODE_HPP

#include "enums.hpp"
#define AUTOLOADMODE_IS_FLAG 0

enum class AutoloadMode : int {
    Matched = (int)0,
    Contain = (int)1,
    Folder = (int)2
};

Q_DECLARE_METATYPE(AutoloadMode)

constexpr inline auto operator == (AutoloadMode e, int i) -> bool { return (int)e == i; }
constexpr inline auto operator != (AutoloadMode e, int i) -> bool { return (int)e != i; }
constexpr inline auto operator == (int i, AutoloadMode e) -> bool { return (int)e == i; }
constexpr inline auto operator != (int i, AutoloadMode e) -> bool { return (int)e != i; }
constexpr inline auto operator > (AutoloadMode e, int i) -> bool { return (int)e > i; }
constexpr inline auto operator < (AutoloadMode e, int i) -> bool { return (int)e < i; }
constexpr inline auto operator >= (AutoloadMode e, int i) -> bool { return (int)e >= i; }
constexpr inline auto operator <= (AutoloadMode e, int i) -> bool { return (int)e <= i; }
constexpr inline auto operator > (int i, AutoloadMode e) -> bool { return i > (int)e; }
constexpr inline auto operator < (int i, AutoloadMode e) -> bool { return i < (int)e; }
constexpr inline auto operator >= (int i, AutoloadMode e) -> bool { return i >= (int)e; }
constexpr inline auto operator <= (int i, AutoloadMode e) -> bool { return i <= (int)e; }
#if AUTOLOADMODE_IS_FLAG
#include "enumflags.hpp"
using  = EnumFlags<AutoloadMode>;
constexpr inline auto operator | (AutoloadMode e1, AutoloadMode e2) -> 
{ return (::IntType(e1) | ::IntType(e2)); }
constexpr inline auto operator ~ (AutoloadMode e) -> EnumNot<AutoloadMode>
{ return EnumNot<AutoloadMode>(e); }
constexpr inline auto operator & (AutoloadMode lhs,  rhs) -> EnumAnd<AutoloadMode>
{ return rhs & lhs; }
Q_DECLARE_METATYPE()
#endif

template<>
class EnumInfo<AutoloadMode> {
    typedef AutoloadMode Enum;
public:
    typedef AutoloadMode type;
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
    { return "AutoloadMode"; }
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
        case Enum::Matched: return qApp->translate("EnumInfo", "Files of which names match playing file name");
        case Enum::Contain: return qApp->translate("EnumInfo", "Files of which names contain playing file name");
        case Enum::Folder: return qApp->translate("EnumInfo", "All files in the matched folders");
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
    { return AutoloadMode::Matched; }
private:
    static const ItemList info;
};

using AutoloadModeInfo = EnumInfo<AutoloadMode>;

#endif
