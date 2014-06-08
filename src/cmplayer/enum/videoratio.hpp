#ifndef VIDEORATIO_HPP
#define VIDEORATIO_HPP

#include "enums.hpp"
#define VIDEORATIO_IS_FLAG 0

enum class VideoRatio : int {
    Source = (int)0,
    Window = (int)1,
    _4__3 = (int)2,
    _16__10 = (int)3,
    _16__9 = (int)4,
    _1_85__1 = (int)5,
    _2_35__1 = (int)6
};

Q_DECLARE_METATYPE(VideoRatio)

constexpr inline auto operator == (VideoRatio e, int i) -> bool { return (int)e == i; }
constexpr inline auto operator != (VideoRatio e, int i) -> bool { return (int)e != i; }
constexpr inline auto operator == (int i, VideoRatio e) -> bool { return (int)e == i; }
constexpr inline auto operator != (int i, VideoRatio e) -> bool { return (int)e != i; }
constexpr inline auto operator > (VideoRatio e, int i) -> bool { return (int)e > i; }
constexpr inline auto operator < (VideoRatio e, int i) -> bool { return (int)e < i; }
constexpr inline auto operator >= (VideoRatio e, int i) -> bool { return (int)e >= i; }
constexpr inline auto operator <= (VideoRatio e, int i) -> bool { return (int)e <= i; }
constexpr inline auto operator > (int i, VideoRatio e) -> bool { return i > (int)e; }
constexpr inline auto operator < (int i, VideoRatio e) -> bool { return i < (int)e; }
constexpr inline auto operator >= (int i, VideoRatio e) -> bool { return i >= (int)e; }
constexpr inline auto operator <= (int i, VideoRatio e) -> bool { return i <= (int)e; }
#if VIDEORATIO_IS_FLAG
#include "enumflags.hpp"
using  = EnumFlags<VideoRatio>;
constexpr inline auto operator | (VideoRatio e1, VideoRatio e2) -> 
{ return (::IntType(e1) | ::IntType(e2)); }
constexpr inline auto operator ~ (VideoRatio e) -> EnumNot<VideoRatio>
{ return EnumNot<VideoRatio>(e); }
constexpr inline auto operator & (VideoRatio lhs,  rhs) -> EnumAnd<VideoRatio>
{ return rhs & lhs; }
Q_DECLARE_METATYPE()
#endif

template<>
class EnumInfo<VideoRatio> {
    typedef VideoRatio Enum;
public:
    typedef VideoRatio type;
    using Data =  qreal;
    struct Item {
        Enum value;
        QString name, key;
        qreal data;
    };
    using ItemList = std::array<Item, 7>;
    static constexpr auto size() -> int
    { return 7; }
    static constexpr auto typeName() -> const char*
    { return "VideoRatio"; }
    static constexpr auto typeKey() -> const char*
    { return "size"; }
    static auto typeDescription() -> QString
    { return qApp->translate("EnumInfo", "Size"); }
    static auto item(Enum e) -> const Item*
    { return 0 <= e && e < size() ? &info[(int)e] : nullptr; }
    static auto name(Enum e) -> QString
    { auto i = item(e); return i ? i->name : QString(); }
    static auto key(Enum e) -> QString
    { auto i = item(e); return i ? i->key : QString(); }
    static auto data(Enum e) -> qreal
    { auto i = item(e); return i ? i->data : qreal(); }
    static auto description(int e) -> QString
    { return description((Enum)e); }
    static auto description(Enum e) -> QString
    {
        switch (e) {
        case Enum::Source: return qApp->translate("EnumInfo", "Same as Source");
        case Enum::Window: return qApp->translate("EnumInfo", "Same as Window");
        case Enum::_4__3: return qApp->translate("EnumInfo", "4:3 (TV)");
        case Enum::_16__10: return qApp->translate("EnumInfo", "16:10 (Wide Monitor)");
        case Enum::_16__9: return qApp->translate("EnumInfo", "16:9 (HDTV)");
        case Enum::_1_85__1: return qApp->translate("EnumInfo", "1.85:1 (Wide Vision)");
        case Enum::_2_35__1: return qApp->translate("EnumInfo", "2.35:1 (CinemaScope)");
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
    static auto fromData(const qreal &data,
                         Enum def = default_()) -> Enum
    {
        auto it = std::find_if(info.cbegin(), info.cend(),
                               [&data] (const Item &item)
                               { return item.data == data; });
        return it != info.cend() ? it->value : def;
    }
    static constexpr auto default_() -> Enum
    { return VideoRatio::Source; }
private:
    static const ItemList info;
};

using VideoRatioInfo = EnumInfo<VideoRatio>;

#endif
