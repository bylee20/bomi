#ifndef KEYMODIFIER_HPP
#define KEYMODIFIER_HPP

#include "enums.hpp"
#define KEYMODIFIER_IS_FLAG 0

enum class KeyModifier : int {
    None = (int)Qt::NoModifier,
    Ctrl = (int)Qt::ControlModifier,
    Shift = (int)Qt::ShiftModifier,
    Alt = (int)Qt::AltModifier
};

Q_DECLARE_METATYPE(KeyModifier)

constexpr inline auto operator == (KeyModifier e, int i) -> bool { return (int)e == i; }
constexpr inline auto operator != (KeyModifier e, int i) -> bool { return (int)e != i; }
constexpr inline auto operator == (int i, KeyModifier e) -> bool { return (int)e == i; }
constexpr inline auto operator != (int i, KeyModifier e) -> bool { return (int)e != i; }
constexpr inline auto operator > (KeyModifier e, int i) -> bool { return (int)e > i; }
constexpr inline auto operator < (KeyModifier e, int i) -> bool { return (int)e < i; }
constexpr inline auto operator >= (KeyModifier e, int i) -> bool { return (int)e >= i; }
constexpr inline auto operator <= (KeyModifier e, int i) -> bool { return (int)e <= i; }
constexpr inline auto operator > (int i, KeyModifier e) -> bool { return i > (int)e; }
constexpr inline auto operator < (int i, KeyModifier e) -> bool { return i < (int)e; }
constexpr inline auto operator >= (int i, KeyModifier e) -> bool { return i >= (int)e; }
constexpr inline auto operator <= (int i, KeyModifier e) -> bool { return i <= (int)e; }
#if KEYMODIFIER_IS_FLAG
#include "enumflags.hpp"
using  = EnumFlags<KeyModifier>;
constexpr inline auto operator | (KeyModifier e1, KeyModifier e2) -> 
{ return (::IntType(e1) | ::IntType(e2)); }
constexpr inline auto operator ~ (KeyModifier e) -> EnumNot<KeyModifier>
{ return EnumNot<KeyModifier>(e); }
constexpr inline auto operator & (KeyModifier lhs,  rhs) -> EnumAnd<KeyModifier>
{ return rhs & lhs; }
Q_DECLARE_METATYPE()
#endif

template<>
class EnumInfo<KeyModifier> {
    typedef KeyModifier Enum;
public:
    typedef KeyModifier type;
    using Data =  QVariant;
    struct Item {
        Enum value;
        QString name, key;
        QVariant data;
    };
    using ItemList = std::array<Item, 4>;
    static constexpr auto size() -> int
    { return 4; }
    static constexpr auto typeName() -> const char*
    { return "KeyModifier"; }
    static constexpr auto typeKey() -> const char*
    { return ""; }
    static auto typeDescription() -> QString
    { return qApp->translate("EnumInfo", ""); }
    static auto item(Enum e) -> const Item*
    { 
    auto it = std::find_if(info.cbegin(), info.cend(),
                            [e] (const Item &info)
                            { return info.value == e; });
    return it != info.cend() ? &(*it) : nullptr;
 }
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
        case Enum::Ctrl: return qApp->translate("EnumInfo", "");
        case Enum::Shift: return qApp->translate("EnumInfo", "");
        case Enum::Alt: return qApp->translate("EnumInfo", "");
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
    { return KeyModifier::None; }
private:
    static const ItemList info;
};

using KeyModifierInfo = EnumInfo<KeyModifier>;

#endif
