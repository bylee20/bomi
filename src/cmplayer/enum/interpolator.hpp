#ifndef INTERPOLATOR_HPP
#define INTERPOLATOR_HPP

#include "enums.hpp"
#define INTERPOLATOR_IS_FLAG 0

enum class Interpolator : int {
    Bilinear = (int)0,
    BicubicBS = (int)1,
    BicubicCR = (int)2,
    BicubicMN = (int)3,
    Spline16 = (int)4,
    Spline36 = (int)5,
    Spline64 = (int)6,
    Lanczos2 = (int)7,
    Lanczos3 = (int)8,
    Lanczos4 = (int)9,
    Sharpen3 = (int)10,
    Sharpen5 = (int)11
};

Q_DECLARE_METATYPE(Interpolator)

constexpr inline auto operator == (Interpolator e, int i) -> bool { return (int)e == i; }
constexpr inline auto operator != (Interpolator e, int i) -> bool { return (int)e != i; }
constexpr inline auto operator == (int i, Interpolator e) -> bool { return (int)e == i; }
constexpr inline auto operator != (int i, Interpolator e) -> bool { return (int)e != i; }
constexpr inline auto operator > (Interpolator e, int i) -> bool { return (int)e > i; }
constexpr inline auto operator < (Interpolator e, int i) -> bool { return (int)e < i; }
constexpr inline auto operator >= (Interpolator e, int i) -> bool { return (int)e >= i; }
constexpr inline auto operator <= (Interpolator e, int i) -> bool { return (int)e <= i; }
constexpr inline auto operator > (int i, Interpolator e) -> bool { return i > (int)e; }
constexpr inline auto operator < (int i, Interpolator e) -> bool { return i < (int)e; }
constexpr inline auto operator >= (int i, Interpolator e) -> bool { return i >= (int)e; }
constexpr inline auto operator <= (int i, Interpolator e) -> bool { return i <= (int)e; }
#if INTERPOLATOR_IS_FLAG
#include "enumflags.hpp"
using  = EnumFlags<Interpolator>;
constexpr inline auto operator | (Interpolator e1, Interpolator e2) -> 
{ return (::IntType(e1) | ::IntType(e2)); }
constexpr inline auto operator ~ (Interpolator e) -> EnumNot<Interpolator>
{ return EnumNot<Interpolator>(e); }
constexpr inline auto operator & (Interpolator lhs,  rhs) -> EnumAnd<Interpolator>
{ return rhs & lhs; }
Q_DECLARE_METATYPE()
#endif

template<>
class EnumInfo<Interpolator> {
    typedef Interpolator Enum;
public:
    typedef Interpolator type;
    using Data =  QByteArray;
    struct Item {
        Enum value;
        QString name, key;
        QByteArray data;
    };
    using ItemList = std::array<Item, 12>;
    static constexpr auto size() -> int
    { return 12; }
    static constexpr auto typeName() -> const char*
    { return "Interpolator"; }
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
    static auto data(Enum e) -> QByteArray
    { auto i = item(e); return i ? i->data : QByteArray(); }
    static auto description(int e) -> QString
    { return description((Enum)e); }
    static auto description(Enum e) -> QString
    {
        switch (e) {
        case Enum::Bilinear: return qApp->translate("EnumInfo", "Bilinear");
        case Enum::BicubicBS: return qApp->translate("EnumInfo", "B-Spline");
        case Enum::BicubicCR: return qApp->translate("EnumInfo", "Catmull-Rom");
        case Enum::BicubicMN: return qApp->translate("EnumInfo", "Mitchell-Netravali");
        case Enum::Spline16: return qApp->translate("EnumInfo", "Spline (Radius: 2)");
        case Enum::Spline36: return qApp->translate("EnumInfo", "Spline (Radius: 3)");
        case Enum::Spline64: return qApp->translate("EnumInfo", "Spline (Radius: 4)");
        case Enum::Lanczos2: return qApp->translate("EnumInfo", "Lanczos (Radius: 2)");
        case Enum::Lanczos3: return qApp->translate("EnumInfo", "Lanczos (Radius: 3)");
        case Enum::Lanczos4: return qApp->translate("EnumInfo", "Lanczos (Radius: 4)");
        case Enum::Sharpen3: return qApp->translate("EnumInfo", "Unsharp Masking (Radius: 3)");
        case Enum::Sharpen5: return qApp->translate("EnumInfo", "Unsharp Masking (Radius: 5)");
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
    static auto fromData(const QByteArray &data,
                         Enum def = default_()) -> Enum
    {
        auto it = std::find_if(info.cbegin(), info.cend(),
                               [&data] (const Item &item)
                               { return item.data == data; });
        return it != info.cend() ? it->value : def;
    }
    static constexpr auto default_() -> Enum
    { return Interpolator::Bilinear; }
private:
    static const ItemList info;
};

using InterpolatorInfo = EnumInfo<Interpolator>;

#endif
