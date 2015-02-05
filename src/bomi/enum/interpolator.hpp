#ifndef INTERPOLATOR_HPP
#define INTERPOLATOR_HPP

#include "enums.hpp"
#define INTERPOLATOR_IS_FLAG 0

enum class Interpolator : int {
    Bilinear = (int)0,
    Bicubic = (int)1,
    Spline = (int)2,
    Lanczos = (int)3,
    EwaLanczos = (int)4,
    Sharpen = (int)5
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
    using ItemList = std::array<Item, 6>;
    static constexpr auto size() -> int
    { return 6; }
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
        case Enum::Bicubic: return qApp->translate("EnumInfo", "Bicubic");
        case Enum::Spline: return qApp->translate("EnumInfo", "Spline");
        case Enum::Lanczos: return qApp->translate("EnumInfo", "Lanczos");
        case Enum::EwaLanczos: return qApp->translate("EnumInfo", "EWA Lanczos");
        case Enum::Sharpen: return qApp->translate("EnumInfo", "Unsharp Masking");
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
