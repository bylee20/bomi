#ifndef ADJUSTCOLOR_HPP
#define ADJUSTCOLOR_HPP

#include "enums.hpp"
#define ADJUSTCOLOR_IS_FLAG 0
#include "video/videocolor.hpp"

enum class AdjustColor : int {
    Reset = (int)0,
    BrightnessInc = (int)1,
    BrightnessDec = (int)2,
    ContrastInc = (int)3,
    ContrastDec = (int)4,
    SaturationInc = (int)5,
    SaturationDec = (int)6,
    HueInc = (int)7,
    HueDec = (int)8
};

Q_DECLARE_METATYPE(AdjustColor)

inline auto operator == (AdjustColor e, int i) -> bool { return (int)e == i; }
inline auto operator != (AdjustColor e, int i) -> bool { return (int)e != i; }
inline auto operator == (int i, AdjustColor e) -> bool { return (int)e == i; }
inline auto operator != (int i, AdjustColor e) -> bool { return (int)e != i; }
inline auto operator > (AdjustColor e, int i) -> bool { return (int)e > i; }
inline auto operator < (AdjustColor e, int i) -> bool { return (int)e < i; }
inline auto operator >= (AdjustColor e, int i) -> bool { return (int)e >= i; }
inline auto operator <= (AdjustColor e, int i) -> bool { return (int)e <= i; }
inline auto operator > (int i, AdjustColor e) -> bool { return i > (int)e; }
inline auto operator < (int i, AdjustColor e) -> bool { return i < (int)e; }
inline auto operator >= (int i, AdjustColor e) -> bool { return i >= (int)e; }
inline auto operator <= (int i, AdjustColor e) -> bool { return i <= (int)e; }
#if ADJUSTCOLOR_IS_FLAG
Q_DECLARE_FLAGS(, AdjustColor)
Q_DECLARE_OPERATORS_FOR_FLAGS()
Q_DECLARE_METATYPE()
#else
inline auto operator & (AdjustColor e, int i) -> int { return (int)e & i; }
inline auto operator & (int i, AdjustColor e) -> int { return (int)e & i; }
inline auto operator &= (int &i, AdjustColor e) -> int& { return i &= (int)e; }
inline auto operator ~ (AdjustColor e) -> int { return ~(int)e; }
inline auto operator | (AdjustColor e, int i) -> int { return (int)e | i; }
inline auto operator | (int i, AdjustColor e) -> int { return (int)e | i; }
constexpr inline auto operator | (AdjustColor e1, AdjustColor e2) -> int { return (int)e1 | (int)e2; }
inline auto operator |= (int &i, AdjustColor e) -> int& { return i |= (int)e; }
#endif

template<>
class EnumInfo<AdjustColor> {
    typedef AdjustColor Enum;
public:
    typedef AdjustColor type;
    using Data =  VideoColor;
    struct Item {
        Enum value;
        QString name, key;
        VideoColor data;
    };
    using ItemList = std::array<Item, 9>;
    static constexpr auto size() -> int
    { return 9; }
    static constexpr auto typeName() -> const char*
    { return "AdjustColor"; }
    static constexpr auto typeKey() -> const char*
    { return "color"; }
    static auto typeDescription() -> QString
    { return qApp->translate("EnumInfo", ""); }
    static auto item(Enum e) -> const Item*
    { return 0 <= e && e < size() ? &info[(int)e] : nullptr; }
    static auto name(Enum e) -> QString
    { auto i = item(e); return i ? i->name : QString(); }
    static auto key(Enum e) -> QString
    { auto i = item(e); return i ? i->key : QString(); }
    static auto data(Enum e) -> VideoColor
    { auto i = item(e); return i ? i->data : VideoColor(); }
    static auto description(int e) -> QString
    { return description((Enum)e); }
    static auto description(Enum e) -> QString
    {
        switch (e) {
        case Enum::Reset: return qApp->translate("EnumInfo", "");
        case Enum::BrightnessInc: return qApp->translate("EnumInfo", "");
        case Enum::BrightnessDec: return qApp->translate("EnumInfo", "");
        case Enum::ContrastInc: return qApp->translate("EnumInfo", "");
        case Enum::ContrastDec: return qApp->translate("EnumInfo", "");
        case Enum::SaturationInc: return qApp->translate("EnumInfo", "");
        case Enum::SaturationDec: return qApp->translate("EnumInfo", "");
        case Enum::HueInc: return qApp->translate("EnumInfo", "");
        case Enum::HueDec: return qApp->translate("EnumInfo", "");
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
    static auto fromData(const VideoColor &data,
                         Enum def = default_()) -> Enum
    {
        auto it = std::find_if(info.cbegin(), info.cend(),
                               [&data] (const Item &item)
                               { return item.data == data; });
        return it != info.cend() ? it->value : def;
    }
    static constexpr auto default_() -> Enum
    { return AdjustColor::Reset; }
private:
    static const ItemList info;
};

using AdjustColorInfo = EnumInfo<AdjustColor>;

#endif
