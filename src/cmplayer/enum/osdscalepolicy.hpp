#ifndef OSDSCALEPOLICY_HPP
#define OSDSCALEPOLICY_HPP

#include "enums.hpp"
#define OSDSCALEPOLICY_IS_FLAG 0

enum class OsdScalePolicy : int {
    Width = (int)0,
    Height = (int)1,
    Diagonal = (int)2
};

Q_DECLARE_METATYPE(OsdScalePolicy)

inline auto operator == (OsdScalePolicy e, int i) -> bool { return (int)e == i; }
inline auto operator != (OsdScalePolicy e, int i) -> bool { return (int)e != i; }
inline auto operator == (int i, OsdScalePolicy e) -> bool { return (int)e == i; }
inline auto operator != (int i, OsdScalePolicy e) -> bool { return (int)e != i; }
inline auto operator > (OsdScalePolicy e, int i) -> bool { return (int)e > i; }
inline auto operator < (OsdScalePolicy e, int i) -> bool { return (int)e < i; }
inline auto operator >= (OsdScalePolicy e, int i) -> bool { return (int)e >= i; }
inline auto operator <= (OsdScalePolicy e, int i) -> bool { return (int)e <= i; }
inline auto operator > (int i, OsdScalePolicy e) -> bool { return i > (int)e; }
inline auto operator < (int i, OsdScalePolicy e) -> bool { return i < (int)e; }
inline auto operator >= (int i, OsdScalePolicy e) -> bool { return i >= (int)e; }
inline auto operator <= (int i, OsdScalePolicy e) -> bool { return i <= (int)e; }
#if OSDSCALEPOLICY_IS_FLAG
Q_DECLARE_FLAGS(, OsdScalePolicy)
Q_DECLARE_OPERATORS_FOR_FLAGS()
Q_DECLARE_METATYPE()
#else
inline auto operator & (OsdScalePolicy e, int i) -> int { return (int)e & i; }
inline auto operator & (int i, OsdScalePolicy e) -> int { return (int)e & i; }
inline auto operator &= (int &i, OsdScalePolicy e) -> int& { return i &= (int)e; }
inline auto operator ~ (OsdScalePolicy e) -> int { return ~(int)e; }
inline auto operator | (OsdScalePolicy e, int i) -> int { return (int)e | i; }
inline auto operator | (int i, OsdScalePolicy e) -> int { return (int)e | i; }
constexpr inline auto operator | (OsdScalePolicy e1, OsdScalePolicy e2) -> int { return (int)e1 | (int)e2; }
inline auto operator |= (int &i, OsdScalePolicy e) -> int& { return i |= (int)e; }
#endif

template<>
class EnumInfo<OsdScalePolicy> {
    typedef OsdScalePolicy Enum;
public:
    typedef OsdScalePolicy type;
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
    { return "OsdScalePolicy"; }
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
        case Enum::Width: return qApp->translate("EnumInfo", "Fit to width of video");
        case Enum::Height: return qApp->translate("EnumInfo", "Fit to height of video");
        case Enum::Diagonal: return qApp->translate("EnumInfo", "Fit to diagonal of video");
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
    static auto fromData(const QVariant &data,
                         Enum def = default_()) -> Enum
    {
        auto it = std::find_if(info.cbegin(), info.cend(),
                               [&data] (const Item &item)
                               { return item.data == data; });
        return it != info.cend() ? it->value : def;
    }
    static constexpr auto default_() -> Enum
    { return OsdScalePolicy::Width; }
private:
    static const ItemList info;
};

using OsdScalePolicyInfo = EnumInfo<OsdScalePolicy>;

#endif
