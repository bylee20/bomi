#ifndef SUBTITLEAUTOLOAD_HPP
#define SUBTITLEAUTOLOAD_HPP

#include "enums.hpp"

enum class SubtitleAutoload : int {
    Matched = (int)0,
    Contain = (int)1,
    Folder = (int)2
};

inline auto operator == (SubtitleAutoload e, int i) -> bool { return (int)e == i; }
inline auto operator != (SubtitleAutoload e, int i) -> bool { return (int)e != i; }
inline auto operator == (int i, SubtitleAutoload e) -> bool { return (int)e == i; }
inline auto operator != (int i, SubtitleAutoload e) -> bool { return (int)e != i; }
inline auto operator & (SubtitleAutoload e, int i) -> int { return (int)e & i; }
inline auto operator & (int i, SubtitleAutoload e) -> int { return (int)e & i; }
inline auto operator &= (int &i, SubtitleAutoload e) -> int& { return i &= (int)e; }
inline auto operator ~ (SubtitleAutoload e) -> int { return ~(int)e; }
inline auto operator | (SubtitleAutoload e, int i) -> int { return (int)e | i; }
inline auto operator | (int i, SubtitleAutoload e) -> int { return (int)e | i; }
constexpr inline auto operator | (SubtitleAutoload e1, SubtitleAutoload e2) -> int { return (int)e1 | (int)e2; }
inline auto operator |= (int &i, SubtitleAutoload e) -> int& { return i |= (int)e; }
inline auto operator > (SubtitleAutoload e, int i) -> bool { return (int)e > i; }
inline auto operator < (SubtitleAutoload e, int i) -> bool { return (int)e < i; }
inline auto operator >= (SubtitleAutoload e, int i) -> bool { return (int)e >= i; }
inline auto operator <= (SubtitleAutoload e, int i) -> bool { return (int)e <= i; }
inline auto operator > (int i, SubtitleAutoload e) -> bool { return i > (int)e; }
inline auto operator < (int i, SubtitleAutoload e) -> bool { return i < (int)e; }
inline auto operator >= (int i, SubtitleAutoload e) -> bool { return i >= (int)e; }
inline auto operator <= (int i, SubtitleAutoload e) -> bool { return i <= (int)e; }

Q_DECLARE_METATYPE(SubtitleAutoload)

template<>
class EnumInfo<SubtitleAutoload> {
    typedef SubtitleAutoload Enum;
public:
    typedef SubtitleAutoload type;
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
    { return "SubtitleAutoload"; }
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
        case Enum::Matched: return qApp->translate("EnumInfo", "Subtitles which have the same name as that of playing file");
        case Enum::Contain: return qApp->translate("EnumInfo", "Subtitles whose names contain the name of playing file");
        case Enum::Folder: return qApp->translate("EnumInfo", "All subtitles in the folder where the playing file is located");
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
    { return SubtitleAutoload::Matched; }
private:
    static const ItemList info;
};

using SubtitleAutoloadInfo = EnumInfo<SubtitleAutoload>;

#endif
