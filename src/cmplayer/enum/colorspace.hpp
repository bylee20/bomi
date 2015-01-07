#ifndef COLORSPACE_HPP
#define COLORSPACE_HPP

#include "enums.hpp"
#define COLORSPACE_IS_FLAG 0
#include "colorenumdata.hpp"

enum class ColorSpace : int {
    Auto = (int)0,
    SMPTE240M = (int)1,
    BT601 = (int)2,
    BT709 = (int)3,
    BT2020NCL = (int)4,
    BT2020CL = (int)5,
    RGB = (int)6,
    XYZ = (int)7,
    YCgCo = (int)8
};

Q_DECLARE_METATYPE(ColorSpace)

constexpr inline auto operator == (ColorSpace e, int i) -> bool { return (int)e == i; }
constexpr inline auto operator != (ColorSpace e, int i) -> bool { return (int)e != i; }
constexpr inline auto operator == (int i, ColorSpace e) -> bool { return (int)e == i; }
constexpr inline auto operator != (int i, ColorSpace e) -> bool { return (int)e != i; }
constexpr inline auto operator > (ColorSpace e, int i) -> bool { return (int)e > i; }
constexpr inline auto operator < (ColorSpace e, int i) -> bool { return (int)e < i; }
constexpr inline auto operator >= (ColorSpace e, int i) -> bool { return (int)e >= i; }
constexpr inline auto operator <= (ColorSpace e, int i) -> bool { return (int)e <= i; }
constexpr inline auto operator > (int i, ColorSpace e) -> bool { return i > (int)e; }
constexpr inline auto operator < (int i, ColorSpace e) -> bool { return i < (int)e; }
constexpr inline auto operator >= (int i, ColorSpace e) -> bool { return i >= (int)e; }
constexpr inline auto operator <= (int i, ColorSpace e) -> bool { return i <= (int)e; }
#if COLORSPACE_IS_FLAG
#include "enumflags.hpp"
using  = EnumFlags<ColorSpace>;
constexpr inline auto operator | (ColorSpace e1, ColorSpace e2) -> 
{ return (::IntType(e1) | ::IntType(e2)); }
constexpr inline auto operator ~ (ColorSpace e) -> EnumNot<ColorSpace>
{ return EnumNot<ColorSpace>(e); }
constexpr inline auto operator & (ColorSpace lhs,  rhs) -> EnumAnd<ColorSpace>
{ return rhs & lhs; }
Q_DECLARE_METATYPE()
#endif

template<>
class EnumInfo<ColorSpace> {
    typedef ColorSpace Enum;
public:
    typedef ColorSpace type;
    using Data =  ColorEnumData;
    struct Item {
        Enum value;
        QString name, key;
        ColorEnumData data;
    };
    using ItemList = std::array<Item, 9>;
    static constexpr auto size() -> int
    { return 9; }
    static constexpr auto typeName() -> const char*
    { return "ColorSpace"; }
    static constexpr auto typeKey() -> const char*
    { return "space"; }
    static auto typeDescription() -> QString
    { return qApp->translate("EnumInfo", "Color Space"); }
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
        case Enum::SMPTE240M: return qApp->translate("EnumInfo", "SMPTE-240M");
        case Enum::BT601: return qApp->translate("EnumInfo", "BT.601(SD)");
        case Enum::BT709: return qApp->translate("EnumInfo", "BT.709(HD)");
        case Enum::BT2020NCL: return qApp->translate("EnumInfo", "BT.2020-NCL(UHD)");
        case Enum::BT2020CL: return qApp->translate("EnumInfo", "BT.2020-CL(UHD)");
        case Enum::RGB: return qApp->translate("EnumInfo", "RGB");
        case Enum::XYZ: return qApp->translate("EnumInfo", "XYZ");
        case Enum::YCgCo: return qApp->translate("EnumInfo", "YCgCo");
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
    { return ColorSpace::Auto; }
private:
    static const ItemList info;
};

using ColorSpaceInfo = EnumInfo<ColorSpace>;

#endif
