#ifndef GENERATEPLAYLIST_HPP
#define GENERATEPLAYLIST_HPP

#include "enums.hpp"
#define GENERATEPLAYLIST_IS_FLAG 0

enum class GeneratePlaylist : int {
    Similar = (int)0,
    Folder = (int)1
};

Q_DECLARE_METATYPE(GeneratePlaylist)

constexpr inline auto operator == (GeneratePlaylist e, int i) -> bool { return (int)e == i; }
constexpr inline auto operator != (GeneratePlaylist e, int i) -> bool { return (int)e != i; }
constexpr inline auto operator == (int i, GeneratePlaylist e) -> bool { return (int)e == i; }
constexpr inline auto operator != (int i, GeneratePlaylist e) -> bool { return (int)e != i; }
constexpr inline auto operator > (GeneratePlaylist e, int i) -> bool { return (int)e > i; }
constexpr inline auto operator < (GeneratePlaylist e, int i) -> bool { return (int)e < i; }
constexpr inline auto operator >= (GeneratePlaylist e, int i) -> bool { return (int)e >= i; }
constexpr inline auto operator <= (GeneratePlaylist e, int i) -> bool { return (int)e <= i; }
constexpr inline auto operator > (int i, GeneratePlaylist e) -> bool { return i > (int)e; }
constexpr inline auto operator < (int i, GeneratePlaylist e) -> bool { return i < (int)e; }
constexpr inline auto operator >= (int i, GeneratePlaylist e) -> bool { return i >= (int)e; }
constexpr inline auto operator <= (int i, GeneratePlaylist e) -> bool { return i <= (int)e; }
#if GENERATEPLAYLIST_IS_FLAG
#include "enumflags.hpp"
using  = EnumFlags<GeneratePlaylist>;
constexpr inline auto operator | (GeneratePlaylist e1, GeneratePlaylist e2) -> 
{ return (::IntType(e1) | ::IntType(e2)); }
constexpr inline auto operator ~ (GeneratePlaylist e) -> EnumNot<GeneratePlaylist>
{ return EnumNot<GeneratePlaylist>(e); }
constexpr inline auto operator & (GeneratePlaylist lhs,  rhs) -> EnumAnd<GeneratePlaylist>
{ return rhs & lhs; }
Q_DECLARE_METATYPE()
#endif

template<>
class EnumInfo<GeneratePlaylist> {
    typedef GeneratePlaylist Enum;
public:
    typedef GeneratePlaylist type;
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
    { return "GeneratePlaylist"; }
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
        case Enum::Similar: return qApp->translate("EnumInfo", "Add files which have similar names");
        case Enum::Folder: return qApp->translate("EnumInfo", "Add all files in the same folder");
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
    { return GeneratePlaylist::Similar; }
private:
    static const ItemList info;
};

using GeneratePlaylistInfo = EnumInfo<GeneratePlaylist>;

#endif
