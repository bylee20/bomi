#ifndef PLAYLISTBEHAVIORWHENOPENMEDIA_HPP
#define PLAYLISTBEHAVIORWHENOPENMEDIA_HPP

#include "enums.hpp"

enum class PlaylistBehaviorWhenOpenMedia : int {
    AppendToPlaylist = (int)0,
    ClearAndAppendToPlaylist = (int)1,
    ClearAndGenerateNewPlaylist = (int)2
};

inline auto operator == (PlaylistBehaviorWhenOpenMedia e, int i) -> bool { return (int)e == i; }
inline auto operator != (PlaylistBehaviorWhenOpenMedia e, int i) -> bool { return (int)e != i; }
inline auto operator == (int i, PlaylistBehaviorWhenOpenMedia e) -> bool { return (int)e == i; }
inline auto operator != (int i, PlaylistBehaviorWhenOpenMedia e) -> bool { return (int)e != i; }
inline auto operator & (PlaylistBehaviorWhenOpenMedia e, int i) -> int { return (int)e & i; }
inline auto operator & (int i, PlaylistBehaviorWhenOpenMedia e) -> int { return (int)e & i; }
inline auto operator &= (int &i, PlaylistBehaviorWhenOpenMedia e) -> int& { return i &= (int)e; }
inline auto operator ~ (PlaylistBehaviorWhenOpenMedia e) -> int { return ~(int)e; }
inline auto operator | (PlaylistBehaviorWhenOpenMedia e, int i) -> int { return (int)e | i; }
inline auto operator | (int i, PlaylistBehaviorWhenOpenMedia e) -> int { return (int)e | i; }
constexpr inline auto operator | (PlaylistBehaviorWhenOpenMedia e1, PlaylistBehaviorWhenOpenMedia e2) -> int { return (int)e1 | (int)e2; }
inline auto operator |= (int &i, PlaylistBehaviorWhenOpenMedia e) -> int& { return i |= (int)e; }
inline auto operator > (PlaylistBehaviorWhenOpenMedia e, int i) -> bool { return (int)e > i; }
inline auto operator < (PlaylistBehaviorWhenOpenMedia e, int i) -> bool { return (int)e < i; }
inline auto operator >= (PlaylistBehaviorWhenOpenMedia e, int i) -> bool { return (int)e >= i; }
inline auto operator <= (PlaylistBehaviorWhenOpenMedia e, int i) -> bool { return (int)e <= i; }
inline auto operator > (int i, PlaylistBehaviorWhenOpenMedia e) -> bool { return i > (int)e; }
inline auto operator < (int i, PlaylistBehaviorWhenOpenMedia e) -> bool { return i < (int)e; }
inline auto operator >= (int i, PlaylistBehaviorWhenOpenMedia e) -> bool { return i >= (int)e; }
inline auto operator <= (int i, PlaylistBehaviorWhenOpenMedia e) -> bool { return i <= (int)e; }

Q_DECLARE_METATYPE(PlaylistBehaviorWhenOpenMedia)

template<>
class EnumInfo<PlaylistBehaviorWhenOpenMedia> {
    typedef PlaylistBehaviorWhenOpenMedia Enum;
public:
    typedef PlaylistBehaviorWhenOpenMedia type;
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
    { return "PlaylistBehaviorWhenOpenMedia"; }
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
        case Enum::AppendToPlaylist: return qApp->translate("EnumInfo", "Append the open media to the playlist");
        case Enum::ClearAndAppendToPlaylist: return qApp->translate("EnumInfo", "Clear the playlist and append the open media to the playlist");
        case Enum::ClearAndGenerateNewPlaylist: return qApp->translate("EnumInfo", "Clear the playlist and generate new playlist");
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
    { return PlaylistBehaviorWhenOpenMedia::AppendToPlaylist; }
private:
    static const ItemList info;
};

using PlaylistBehaviorWhenOpenMediaInfo = EnumInfo<PlaylistBehaviorWhenOpenMedia>;

#endif
