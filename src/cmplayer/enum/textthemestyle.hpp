#ifndef TEXTTHEMESTYLE_HPP
#define TEXTTHEMESTYLE_HPP

#include "enums.hpp"
#define TEXTTHEMESTYLE_IS_FLAG 0

enum class TextThemeStyle : int {
    Normal = (int)0,
    Outline = (int)1,
    Raised = (int)2,
    Sunken = (int)3
};

Q_DECLARE_METATYPE(TextThemeStyle)

constexpr inline auto operator == (TextThemeStyle e, int i) -> bool { return (int)e == i; }
constexpr inline auto operator != (TextThemeStyle e, int i) -> bool { return (int)e != i; }
constexpr inline auto operator == (int i, TextThemeStyle e) -> bool { return (int)e == i; }
constexpr inline auto operator != (int i, TextThemeStyle e) -> bool { return (int)e != i; }
constexpr inline auto operator > (TextThemeStyle e, int i) -> bool { return (int)e > i; }
constexpr inline auto operator < (TextThemeStyle e, int i) -> bool { return (int)e < i; }
constexpr inline auto operator >= (TextThemeStyle e, int i) -> bool { return (int)e >= i; }
constexpr inline auto operator <= (TextThemeStyle e, int i) -> bool { return (int)e <= i; }
constexpr inline auto operator > (int i, TextThemeStyle e) -> bool { return i > (int)e; }
constexpr inline auto operator < (int i, TextThemeStyle e) -> bool { return i < (int)e; }
constexpr inline auto operator >= (int i, TextThemeStyle e) -> bool { return i >= (int)e; }
constexpr inline auto operator <= (int i, TextThemeStyle e) -> bool { return i <= (int)e; }
#if TEXTTHEMESTYLE_IS_FLAG
#include "enumflags.hpp"
using  = EnumFlags<TextThemeStyle>;
constexpr inline auto operator | (TextThemeStyle e1, TextThemeStyle e2) -> 
{ return (::IntType(e1) | ::IntType(e2)); }
constexpr inline auto operator ~ (TextThemeStyle e) -> EnumNot<TextThemeStyle>
{ return EnumNot<TextThemeStyle>(e); }
constexpr inline auto operator & (TextThemeStyle lhs,  rhs) -> EnumAnd<TextThemeStyle>
{ return rhs & lhs; }
Q_DECLARE_METATYPE()
#endif

template<>
class EnumInfo<TextThemeStyle> {
    typedef TextThemeStyle Enum;
public:
    typedef TextThemeStyle type;
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
    { return "TextThemeStyle"; }
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
        case Enum::Normal: return qApp->translate("EnumInfo", "Normal");
        case Enum::Outline: return qApp->translate("EnumInfo", "Outline");
        case Enum::Raised: return qApp->translate("EnumInfo", "Raised");
        case Enum::Sunken: return qApp->translate("EnumInfo", "Sunken");
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
    { return TextThemeStyle::Normal; }
private:
    static const ItemList info;
};

using TextThemeStyleInfo = EnumInfo<TextThemeStyle>;

#endif
