#ifndef COLORRANGE_HPP
#define COLORRANGE_HPP

#include "enums.hpp"
#define COLORRANGE_IS_FLAG 0
#include "colorenumdata.hpp"

enum class ColorRange : int {
    Auto = (int)0,
    Limited = (int)1,
    Full = (int)2
};

Q_DECLARE_METATYPE(ColorRange)

constexpr inline auto operator == (ColorRange e, int i) -> bool { return (int)e == i; }
constexpr inline auto operator != (ColorRange e, int i) -> bool { return (int)e != i; }
constexpr inline auto operator == (int i, ColorRange e) -> bool { return (int)e == i; }
constexpr inline auto operator != (int i, ColorRange e) -> bool { return (int)e != i; }
constexpr inline auto operator > (ColorRange e, int i) -> bool { return (int)e > i; }
constexpr inline auto operator < (ColorRange e, int i) -> bool { return (int)e < i; }
constexpr inline auto operator >= (ColorRange e, int i) -> bool { return (int)e >= i; }
constexpr inline auto operator <= (ColorRange e, int i) -> bool { return (int)e <= i; }
constexpr inline auto operator > (int i, ColorRange e) -> bool { return i > (int)e; }
constexpr inline auto operator < (int i, ColorRange e) -> bool { return i < (int)e; }
constexpr inline auto operator >= (int i, ColorRange e) -> bool { return i >= (int)e; }
constexpr inline auto operator <= (int i, ColorRange e) -> bool { return i <= (int)e; }
#if COLORRANGE_IS_FLAG
#include "enumflags.hpp"
using  = EnumFlags<ColorRange>;
constexpr inline auto operator | (ColorRange e1, ColorRange e2) -> 
{ return (::IntType(e1) | ::IntType(e2)); }
constexpr inline auto operator ~ (ColorRange e) -> EnumNot<ColorRange>
{ return EnumNot<ColorRange>(e); }
constexpr inline auto operator & (ColorRange lhs,  rhs) -> EnumAnd<ColorRange>
{ return rhs & lhs; }
Q_DECLARE_METATYPE()
#endif

template<>
class EnumInfo<ColorRange> {
    typedef ColorRange Enum;
public:
    typedef ColorRange type;
    using Data =  ColorEnumData;
    struct Item {
        Enum value;
        QString name, key;
        ColorEnumData data;
    };
    using ItemList = std::array<Item, 3>;
    static constexpr auto size() -> int
    { return 3; }
    static constexpr auto typeName() -> const char*
    { return "ColorRange"; }
    static constexpr auto typeKey() -> const char*
    { return "range"; }
    static auto typeDescription() -> QString
    { return qApp->translate("EnumInfo", "Color Range"); }
    static auto item(Enum e) -> const Item*
    { return 0 <= e && e < size() ? &info[(int)e] : nullptr; }
    static auto name(Enum e) -> QString
    { auto i = item(e); return i ? i->name : QString(); }
    static auto key(Enum e) -> QString
    { auto i = item(e); return i ? i->key : QString(); }
    static auto data(Enum e) -> ColorEnumData
    { auto i = item(e); return i ? i->data : ColorEnumData(); }
    static auto description(int e) -> QString
    { return description((Enum)e); }
    static auto description(Enum e) -> QString
    {
        switch (e) {
        case Enum::Auto: return qApp->translate("EnumInfo", "Auto");
        case Enum::Limited: return qApp->translate("EnumInfo", "Limited Range");
        case Enum::Full: return qApp->translate("EnumInfo", "Full Range");
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
    static auto fromData(const ColorEnumData &data,
                         Enum def = default_()) -> Enum
    {
        auto it = std::find_if(info.cbegin(), info.cend(),
                               [&data] (const Item &item)
                               { return item.data == data; });
        return it != info.cend() ? it->value : def;
    }
    static constexpr auto default_() -> Enum
    { return ColorRange::Auto; }
private:
    static const ItemList info;
};

using ColorRangeInfo = EnumInfo<ColorRange>;

#endif
