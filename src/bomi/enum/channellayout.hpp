#ifndef CHANNELLAYOUT_HPP
#define CHANNELLAYOUT_HPP

#include "enums.hpp"
#define CHANNELLAYOUT_IS_FLAG 0
#include "speakerid.hpp"

enum class ChannelLayout : int {
    Mono = (int)((int)SpeakerId::FrontCenter),
    _2_0 = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight),
    _2_1 = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::LowFrequency),
    _3_0 = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter),
    _3_0_Back = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::BackCenter),
    _3_1 = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::LowFrequency),
    _4_0 = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::BackLeft|(int)SpeakerId::BackRight),
    _4_0_Side = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::SideLeft|(int)SpeakerId::SideRight),
    _4_0_Diamond = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::BackCenter),
    _4_1 = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::BackLeft|(int)SpeakerId::BackRight|(int)SpeakerId::LowFrequency),
    _4_1_Diamond = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::BackCenter|(int)SpeakerId::LowFrequency),
    _5_0 = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::BackLeft|(int)SpeakerId::BackRight),
    _5_0_Side = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::SideLeft|(int)SpeakerId::SideRight),
    _5_1 = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::BackLeft|(int)SpeakerId::BackRight|(int)SpeakerId::LowFrequency),
    _5_1_Side = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::SideLeft|(int)SpeakerId::SideRight|(int)SpeakerId::LowFrequency),
    _6_0 = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::BackCenter|(int)SpeakerId::SideLeft|(int)SpeakerId::SideRight),
    _6_0_Front = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontLeftCenter|(int)SpeakerId::FrontRightCenter|(int)SpeakerId::SideLeft|(int)SpeakerId::SideRight),
    _6_0_Hex = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::BackCenter|(int)SpeakerId::BackLeft|(int)SpeakerId::BackRight),
    _6_1 = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::BackCenter|(int)SpeakerId::SideLeft|(int)SpeakerId::SideRight|(int)SpeakerId::LowFrequency),
    _6_1_Hex = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::BackCenter|(int)SpeakerId::BackLeft|(int)SpeakerId::BackRight|(int)SpeakerId::LowFrequency),
    _6_1_Front = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontLeftCenter|(int)SpeakerId::FrontRightCenter|(int)SpeakerId::SideLeft|(int)SpeakerId::SideRight|(int)SpeakerId::LowFrequency),
    _7_0 = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::BackLeft|(int)SpeakerId::BackRight|(int)SpeakerId::SideLeft|(int)SpeakerId::SideRight),
    _7_0_Front = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::FrontLeftCenter|(int)SpeakerId::FrontRightCenter|(int)SpeakerId::SideLeft|(int)SpeakerId::SideRight),
    _7_1 = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::BackLeft|(int)SpeakerId::BackRight|(int)SpeakerId::SideLeft|(int)SpeakerId::SideRight|(int)SpeakerId::LowFrequency),
    _7_1_Wide = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::FrontLeftCenter|(int)SpeakerId::FrontRightCenter|(int)SpeakerId::BackLeft|(int)SpeakerId::BackRight|(int)SpeakerId::LowFrequency),
    _7_1_Side = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::FrontLeftCenter|(int)SpeakerId::FrontRightCenter|(int)SpeakerId::SideLeft|(int)SpeakerId::SideRight|(int)SpeakerId::LowFrequency)
};

Q_DECLARE_METATYPE(ChannelLayout)

constexpr inline auto operator == (ChannelLayout e, int i) -> bool { return (int)e == i; }
constexpr inline auto operator != (ChannelLayout e, int i) -> bool { return (int)e != i; }
constexpr inline auto operator == (int i, ChannelLayout e) -> bool { return (int)e == i; }
constexpr inline auto operator != (int i, ChannelLayout e) -> bool { return (int)e != i; }
constexpr inline auto operator > (ChannelLayout e, int i) -> bool { return (int)e > i; }
constexpr inline auto operator < (ChannelLayout e, int i) -> bool { return (int)e < i; }
constexpr inline auto operator >= (ChannelLayout e, int i) -> bool { return (int)e >= i; }
constexpr inline auto operator <= (ChannelLayout e, int i) -> bool { return (int)e <= i; }
constexpr inline auto operator > (int i, ChannelLayout e) -> bool { return i > (int)e; }
constexpr inline auto operator < (int i, ChannelLayout e) -> bool { return i < (int)e; }
constexpr inline auto operator >= (int i, ChannelLayout e) -> bool { return i >= (int)e; }
constexpr inline auto operator <= (int i, ChannelLayout e) -> bool { return i <= (int)e; }
#if CHANNELLAYOUT_IS_FLAG
#include "enumflags.hpp"
using  = EnumFlags<ChannelLayout>;
constexpr inline auto operator | (ChannelLayout e1, ChannelLayout e2) -> 
{ return (::IntType(e1) | ::IntType(e2)); }
constexpr inline auto operator ~ (ChannelLayout e) -> EnumNot<ChannelLayout>
{ return EnumNot<ChannelLayout>(e); }
constexpr inline auto operator & (ChannelLayout lhs,  rhs) -> EnumAnd<ChannelLayout>
{ return rhs & lhs; }
Q_DECLARE_METATYPE()
#endif

template<>
class EnumInfo<ChannelLayout> {
    typedef ChannelLayout Enum;
public:
    typedef ChannelLayout type;
    using Data =  QByteArray;
    struct Item {
        Enum value;
        QString name, key;
        QByteArray data;
    };
    using ItemList = std::array<Item, 26>;
    static constexpr auto size() -> int
    { return 26; }
    static constexpr auto typeName() -> const char*
    { return "ChannelLayout"; }
    static constexpr auto typeKey() -> const char*
    { return "channel"; }
    static auto typeDescription() -> QString
    { return qApp->translate("EnumInfo", "Channel Layout"); }
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
    static auto data(Enum e) -> QByteArray
    { auto i = item(e); return i ? i->data : QByteArray(); }
    static auto description(int e) -> QString
    { return description((Enum)e); }
    static auto description(Enum e) -> QString
    {
        switch (e) {
        case Enum::Mono: return qApp->translate("EnumInfo", "Mono");
        case Enum::_2_0: return qApp->translate("EnumInfo", "Stereo");
        case Enum::_2_1: return qApp->translate("EnumInfo", "2.1ch");
        case Enum::_3_0: return qApp->translate("EnumInfo", "3.0ch");
        case Enum::_3_0_Back: return qApp->translate("EnumInfo", "3.0ch(Back)");
        case Enum::_3_1: return qApp->translate("EnumInfo", "3.1ch");
        case Enum::_4_0: return qApp->translate("EnumInfo", "4.0ch");
        case Enum::_4_0_Side: return qApp->translate("EnumInfo", "4.0ch(Side)");
        case Enum::_4_0_Diamond: return qApp->translate("EnumInfo", "4.0ch(Diamond)");
        case Enum::_4_1: return qApp->translate("EnumInfo", "4.1ch");
        case Enum::_4_1_Diamond: return qApp->translate("EnumInfo", "4.1ch(Diamond)");
        case Enum::_5_0: return qApp->translate("EnumInfo", "5.0ch");
        case Enum::_5_0_Side: return qApp->translate("EnumInfo", "5.0ch(Side)");
        case Enum::_5_1: return qApp->translate("EnumInfo", "5.1ch");
        case Enum::_5_1_Side: return qApp->translate("EnumInfo", "5.1ch(Side)");
        case Enum::_6_0: return qApp->translate("EnumInfo", "6.0ch");
        case Enum::_6_0_Front: return qApp->translate("EnumInfo", "6.0ch(Front)");
        case Enum::_6_0_Hex: return qApp->translate("EnumInfo", "6.0ch(Hexagonal)");
        case Enum::_6_1: return qApp->translate("EnumInfo", "6.1ch");
        case Enum::_6_1_Hex: return qApp->translate("EnumInfo", "6.1ch(Back)");
        case Enum::_6_1_Front: return qApp->translate("EnumInfo", "6.1ch(Front)");
        case Enum::_7_0: return qApp->translate("EnumInfo", "7.0ch");
        case Enum::_7_0_Front: return qApp->translate("EnumInfo", "7.0ch(Front)");
        case Enum::_7_1: return qApp->translate("EnumInfo", "7.1ch");
        case Enum::_7_1_Wide: return qApp->translate("EnumInfo", "7.1ch(Wide)");
        case Enum::_7_1_Side: return qApp->translate("EnumInfo", "7.1ch(Side)");
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
    { return ChannelLayout::_2_0; }
private:
    static const ItemList info;
};

using ChannelLayoutInfo = EnumInfo<ChannelLayout>;

#include "enumflags.hpp"
constexpr inline auto operator & (SpeakerId lhs, ChannelLayout rhs) -> EnumAnd<SpeakerId>
{ return SpeakerId((int)lhs & (int)rhs); }
constexpr inline auto operator & (ChannelLayout rhs, SpeakerId lhs) -> EnumAnd<SpeakerId>
{ return SpeakerId((int)lhs & (int)rhs); }
constexpr inline auto operator & (EnumFlags<SpeakerId> lhs, ChannelLayout rhs) -> EnumFlags<SpeakerId>
{ return lhs & EnumFlags<SpeakerId>((SpeakerId)rhs); }
constexpr inline auto operator & (ChannelLayout lhs, EnumFlags<SpeakerId> rhs) -> EnumFlags<SpeakerId>
{ return rhs & EnumFlags<SpeakerId>((SpeakerId)lhs); }
constexpr inline auto operator | (SpeakerId lhs, SpeakerId rhs) -> EnumFlags<SpeakerId>
{ return EnumFlags<SpeakerId>((int)lhs | (int)rhs); }

#endif
