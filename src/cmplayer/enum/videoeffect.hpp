#ifndef VIDEOEFFECT_HPP
#define VIDEOEFFECT_HPP

#include "enums.hpp"
#define VIDEOEFFECT_IS_FLAG 1

enum class VideoEffect : int {
    None = (int)0,
    FlipV = (int)(1 << 0),
    FlipH = (int)(1 << 1),
    Remap = (int)(1 << 4),
    Gray = (int)(1 << 2),
    Invert = (int)(1 << 3),
    Disable = (int)(1 << 8)
};

Q_DECLARE_METATYPE(VideoEffect)

constexpr inline auto operator == (VideoEffect e, int i) -> bool { return (int)e == i; }
constexpr inline auto operator != (VideoEffect e, int i) -> bool { return (int)e != i; }
constexpr inline auto operator == (int i, VideoEffect e) -> bool { return (int)e == i; }
constexpr inline auto operator != (int i, VideoEffect e) -> bool { return (int)e != i; }
constexpr inline auto operator > (VideoEffect e, int i) -> bool { return (int)e > i; }
constexpr inline auto operator < (VideoEffect e, int i) -> bool { return (int)e < i; }
constexpr inline auto operator >= (VideoEffect e, int i) -> bool { return (int)e >= i; }
constexpr inline auto operator <= (VideoEffect e, int i) -> bool { return (int)e <= i; }
constexpr inline auto operator > (int i, VideoEffect e) -> bool { return i > (int)e; }
constexpr inline auto operator < (int i, VideoEffect e) -> bool { return i < (int)e; }
constexpr inline auto operator >= (int i, VideoEffect e) -> bool { return i >= (int)e; }
constexpr inline auto operator <= (int i, VideoEffect e) -> bool { return i <= (int)e; }
#if VIDEOEFFECT_IS_FLAG
#include "enumflags.hpp"
using VideoEffects = EnumFlags<VideoEffect>;
constexpr inline auto operator | (VideoEffect e1, VideoEffect e2) -> VideoEffects
{ return VideoEffects(VideoEffects::IntType(e1) | VideoEffects::IntType(e2)); }
constexpr inline auto operator ~ (VideoEffect e) -> EnumNot<VideoEffect>
{ return EnumNot<VideoEffect>(e); }
constexpr inline auto operator & (VideoEffect lhs, VideoEffects rhs) -> EnumAnd<VideoEffect>
{ return rhs & lhs; }
Q_DECLARE_METATYPE(VideoEffects)
#endif

template<>
class EnumInfo<VideoEffect> {
    typedef VideoEffect Enum;
public:
    typedef VideoEffect type;
    using Data =  QVariant;
    struct Item {
        Enum value;
        QString name, key;
        QVariant data;
    };
    using ItemList = std::array<Item, 7>;
    static constexpr auto size() -> int
    { return 7; }
    static constexpr auto typeName() -> const char*
    { return "VideoEffect"; }
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
    static auto data(Enum e) -> QVariant
    { auto i = item(e); return i ? i->data : QVariant(); }
    static auto description(int e) -> QString
    { return description((Enum)e); }
    static auto description(Enum e) -> QString
    {
        switch (e) {
        case Enum::None: return qApp->translate("EnumInfo", "");
        case Enum::FlipV: return qApp->translate("EnumInfo", "");
        case Enum::FlipH: return qApp->translate("EnumInfo", "");
        case Enum::Remap: return qApp->translate("EnumInfo", "");
        case Enum::Gray: return qApp->translate("EnumInfo", "");
        case Enum::Invert: return qApp->translate("EnumInfo", "");
        case Enum::Disable: return qApp->translate("EnumInfo", "");
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
    { return VideoEffect::None; }
private:
    static const ItemList info;
};

using VideoEffectInfo = EnumInfo<VideoEffect>;

#endif
