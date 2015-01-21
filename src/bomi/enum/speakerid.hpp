#ifndef SPEAKERID_HPP
#define SPEAKERID_HPP

#include "enums.hpp"
#define SPEAKERID_IS_FLAG 0
extern "C" {
#include <audio/chmap.h>
}
#ifdef bool
#undef bool
#endif

enum class SpeakerId : int {
    FrontLeft = (int)(1 << 0),
    FrontRight = (int)(1 << 1),
    FrontCenter = (int)(1 << 2),
    LowFrequency = (int)(1 << 3),
    BackLeft = (int)(1 << 4),
    BackRight = (int)(1 << 5),
    FrontLeftCenter = (int)(1 << 6),
    FrontRightCenter = (int)(1 << 7),
    BackCenter = (int)(1 << 8),
    SideLeft = (int)(1 << 9),
    SideRight = (int)(1 << 10)
};

Q_DECLARE_METATYPE(SpeakerId)

constexpr inline auto operator == (SpeakerId e, int i) -> bool { return (int)e == i; }
constexpr inline auto operator != (SpeakerId e, int i) -> bool { return (int)e != i; }
constexpr inline auto operator == (int i, SpeakerId e) -> bool { return (int)e == i; }
constexpr inline auto operator != (int i, SpeakerId e) -> bool { return (int)e != i; }
constexpr inline auto operator > (SpeakerId e, int i) -> bool { return (int)e > i; }
constexpr inline auto operator < (SpeakerId e, int i) -> bool { return (int)e < i; }
constexpr inline auto operator >= (SpeakerId e, int i) -> bool { return (int)e >= i; }
constexpr inline auto operator <= (SpeakerId e, int i) -> bool { return (int)e <= i; }
constexpr inline auto operator > (int i, SpeakerId e) -> bool { return i > (int)e; }
constexpr inline auto operator < (int i, SpeakerId e) -> bool { return i < (int)e; }
constexpr inline auto operator >= (int i, SpeakerId e) -> bool { return i >= (int)e; }
constexpr inline auto operator <= (int i, SpeakerId e) -> bool { return i <= (int)e; }
#if SPEAKERID_IS_FLAG
#include "enumflags.hpp"
using  = EnumFlags<SpeakerId>;
constexpr inline auto operator | (SpeakerId e1, SpeakerId e2) -> 
{ return (::IntType(e1) | ::IntType(e2)); }
constexpr inline auto operator ~ (SpeakerId e) -> EnumNot<SpeakerId>
{ return EnumNot<SpeakerId>(e); }
constexpr inline auto operator & (SpeakerId lhs,  rhs) -> EnumAnd<SpeakerId>
{ return rhs & lhs; }
Q_DECLARE_METATYPE()
#endif

template<>
class EnumInfo<SpeakerId> {
    typedef SpeakerId Enum;
public:
    typedef SpeakerId type;
    using Data =  mp_speaker_id;
    struct Item {
        Enum value;
        QString name, key;
        mp_speaker_id data;
    };
    using ItemList = std::array<Item, 11>;
    static constexpr auto size() -> int
    { return 11; }
    static constexpr auto typeName() -> const char*
    { return "SpeakerId"; }
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
    static auto data(Enum e) -> mp_speaker_id
    { auto i = item(e); return i ? i->data : mp_speaker_id(); }
    static auto description(int e) -> QString
    { return description((Enum)e); }
    static auto description(Enum e) -> QString
    {
        switch (e) {
        case Enum::FrontLeft: return qApp->translate("EnumInfo", "");
        case Enum::FrontRight: return qApp->translate("EnumInfo", "");
        case Enum::FrontCenter: return qApp->translate("EnumInfo", "");
        case Enum::LowFrequency: return qApp->translate("EnumInfo", "");
        case Enum::BackLeft: return qApp->translate("EnumInfo", "");
        case Enum::BackRight: return qApp->translate("EnumInfo", "");
        case Enum::FrontLeftCenter: return qApp->translate("EnumInfo", "");
        case Enum::FrontRightCenter: return qApp->translate("EnumInfo", "");
        case Enum::BackCenter: return qApp->translate("EnumInfo", "");
        case Enum::SideLeft: return qApp->translate("EnumInfo", "");
        case Enum::SideRight: return qApp->translate("EnumInfo", "");
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
    static auto fromData(const mp_speaker_id &data,
                         Enum def = default_()) -> Enum
    {
        auto it = std::find_if(info.cbegin(), info.cend(),
                               [&data] (const Item &item)
                               { return item.data == data; });
        return it != info.cend() ? it->value : def;
    }
    static constexpr auto default_() -> Enum
    { return SpeakerId::FrontLeft; }
private:
    static const ItemList info;
};

using SpeakerIdInfo = EnumInfo<SpeakerId>;

#endif
