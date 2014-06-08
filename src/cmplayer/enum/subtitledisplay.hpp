#ifndef SUBTITLEDISPLAY_HPP
#define SUBTITLEDISPLAY_HPP

#include "enums.hpp"
#define SUBTITLEDISPLAY_IS_FLAG 0

enum class SubtitleDisplay : int {
    OnLetterbox = (int)0,
    InVideo = (int)1
};

Q_DECLARE_METATYPE(SubtitleDisplay)

constexpr inline auto operator == (SubtitleDisplay e, int i) -> bool { return (int)e == i; }
constexpr inline auto operator != (SubtitleDisplay e, int i) -> bool { return (int)e != i; }
constexpr inline auto operator == (int i, SubtitleDisplay e) -> bool { return (int)e == i; }
constexpr inline auto operator != (int i, SubtitleDisplay e) -> bool { return (int)e != i; }
constexpr inline auto operator > (SubtitleDisplay e, int i) -> bool { return (int)e > i; }
constexpr inline auto operator < (SubtitleDisplay e, int i) -> bool { return (int)e < i; }
constexpr inline auto operator >= (SubtitleDisplay e, int i) -> bool { return (int)e >= i; }
constexpr inline auto operator <= (SubtitleDisplay e, int i) -> bool { return (int)e <= i; }
constexpr inline auto operator > (int i, SubtitleDisplay e) -> bool { return i > (int)e; }
constexpr inline auto operator < (int i, SubtitleDisplay e) -> bool { return i < (int)e; }
constexpr inline auto operator >= (int i, SubtitleDisplay e) -> bool { return i >= (int)e; }
constexpr inline auto operator <= (int i, SubtitleDisplay e) -> bool { return i <= (int)e; }
#if SUBTITLEDISPLAY_IS_FLAG
#include "enumflags.hpp"
using  = EnumFlags<SubtitleDisplay>;
constexpr inline auto operator | (SubtitleDisplay e1, SubtitleDisplay e2) -> 
{ return (::IntType(e1) | ::IntType(e2)); }
constexpr inline auto operator ~ (SubtitleDisplay e) -> EnumNot<SubtitleDisplay>
{ return EnumNot<SubtitleDisplay>(e); }
constexpr inline auto operator & (SubtitleDisplay lhs,  rhs) -> EnumAnd<SubtitleDisplay>
{ return rhs & lhs; }
Q_DECLARE_METATYPE()
#endif

template<>
class EnumInfo<SubtitleDisplay> {
    typedef SubtitleDisplay Enum;
public:
    typedef SubtitleDisplay type;
    using Data =  QVariant;
    struct Item {
        Enum value;
        QString name, key;
        QVariant data;
    };
    using ItemList = std::array<Item, 2>;
    static constexpr auto size() -> int
    { return 2; }
    static constexpr auto typeName() -> const char*
    { return "SubtitleDisplay"; }
    static constexpr auto typeKey() -> const char*
    { return "display"; }
    static auto typeDescription() -> QString
    { return qApp->translate("EnumInfo", "Subtitle Display"); }
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
        case Enum::OnLetterbox: return qApp->translate("EnumInfo", "Display on Letterbox");
        case Enum::InVideo: return qApp->translate("EnumInfo", "Display in Video");
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
    { return SubtitleDisplay::OnLetterbox; }
private:
    static const ItemList info;
};

using SubtitleDisplayInfo = EnumInfo<SubtitleDisplay>;

#endif
