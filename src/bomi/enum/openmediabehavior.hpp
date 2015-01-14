#ifndef OPENMEDIABEHAVIOR_HPP
#define OPENMEDIABEHAVIOR_HPP

#include "enums.hpp"
#define OPENMEDIABEHAVIOR_IS_FLAG 0

enum class OpenMediaBehavior : int {
    Append = (int)0,
    ClearAndAppend = (int)1,
    NewPlaylist = (int)2
};

Q_DECLARE_METATYPE(OpenMediaBehavior)

constexpr inline auto operator == (OpenMediaBehavior e, int i) -> bool { return (int)e == i; }
constexpr inline auto operator != (OpenMediaBehavior e, int i) -> bool { return (int)e != i; }
constexpr inline auto operator == (int i, OpenMediaBehavior e) -> bool { return (int)e == i; }
constexpr inline auto operator != (int i, OpenMediaBehavior e) -> bool { return (int)e != i; }
constexpr inline auto operator > (OpenMediaBehavior e, int i) -> bool { return (int)e > i; }
constexpr inline auto operator < (OpenMediaBehavior e, int i) -> bool { return (int)e < i; }
constexpr inline auto operator >= (OpenMediaBehavior e, int i) -> bool { return (int)e >= i; }
constexpr inline auto operator <= (OpenMediaBehavior e, int i) -> bool { return (int)e <= i; }
constexpr inline auto operator > (int i, OpenMediaBehavior e) -> bool { return i > (int)e; }
constexpr inline auto operator < (int i, OpenMediaBehavior e) -> bool { return i < (int)e; }
constexpr inline auto operator >= (int i, OpenMediaBehavior e) -> bool { return i >= (int)e; }
constexpr inline auto operator <= (int i, OpenMediaBehavior e) -> bool { return i <= (int)e; }
#if OPENMEDIABEHAVIOR_IS_FLAG
#include "enumflags.hpp"
using  = EnumFlags<OpenMediaBehavior>;
constexpr inline auto operator | (OpenMediaBehavior e1, OpenMediaBehavior e2) -> 
{ return (::IntType(e1) | ::IntType(e2)); }
constexpr inline auto operator ~ (OpenMediaBehavior e) -> EnumNot<OpenMediaBehavior>
{ return EnumNot<OpenMediaBehavior>(e); }
constexpr inline auto operator & (OpenMediaBehavior lhs,  rhs) -> EnumAnd<OpenMediaBehavior>
{ return rhs & lhs; }
Q_DECLARE_METATYPE()
#endif

template<>
class EnumInfo<OpenMediaBehavior> {
    typedef OpenMediaBehavior Enum;
public:
    typedef OpenMediaBehavior type;
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
    { return "OpenMediaBehavior"; }
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
        case Enum::Append: return qApp->translate("EnumInfo", "Append the open media to the playlist");
        case Enum::ClearAndAppend: return qApp->translate("EnumInfo", "Clear the playlist and append the open media to the playlist");
        case Enum::NewPlaylist: return qApp->translate("EnumInfo", "Clear the playlist and generate new playlist");
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
    { return OpenMediaBehavior::Append; }
private:
    static const ItemList info;
};

using OpenMediaBehaviorInfo = EnumInfo<OpenMediaBehavior>;

#endif
