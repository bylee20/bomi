#ifndef QUICKSNAPSHOTSAVE_HPP
#define QUICKSNAPSHOTSAVE_HPP

#include "enums.hpp"
#define QUICKSNAPSHOTSAVE_IS_FLAG 0

enum class QuickSnapshotSave : int {
    Fixed = (int)0,
    Current = (int)1,
    Ask = (int)2
};

Q_DECLARE_METATYPE(QuickSnapshotSave)

constexpr inline auto operator == (QuickSnapshotSave e, int i) -> bool { return (int)e == i; }
constexpr inline auto operator != (QuickSnapshotSave e, int i) -> bool { return (int)e != i; }
constexpr inline auto operator == (int i, QuickSnapshotSave e) -> bool { return (int)e == i; }
constexpr inline auto operator != (int i, QuickSnapshotSave e) -> bool { return (int)e != i; }
constexpr inline auto operator > (QuickSnapshotSave e, int i) -> bool { return (int)e > i; }
constexpr inline auto operator < (QuickSnapshotSave e, int i) -> bool { return (int)e < i; }
constexpr inline auto operator >= (QuickSnapshotSave e, int i) -> bool { return (int)e >= i; }
constexpr inline auto operator <= (QuickSnapshotSave e, int i) -> bool { return (int)e <= i; }
constexpr inline auto operator > (int i, QuickSnapshotSave e) -> bool { return i > (int)e; }
constexpr inline auto operator < (int i, QuickSnapshotSave e) -> bool { return i < (int)e; }
constexpr inline auto operator >= (int i, QuickSnapshotSave e) -> bool { return i >= (int)e; }
constexpr inline auto operator <= (int i, QuickSnapshotSave e) -> bool { return i <= (int)e; }
#if QUICKSNAPSHOTSAVE_IS_FLAG
#include "enumflags.hpp"
using  = EnumFlags<QuickSnapshotSave>;
constexpr inline auto operator | (QuickSnapshotSave e1, QuickSnapshotSave e2) -> 
{ return (::IntType(e1) | ::IntType(e2)); }
constexpr inline auto operator ~ (QuickSnapshotSave e) -> EnumNot<QuickSnapshotSave>
{ return EnumNot<QuickSnapshotSave>(e); }
constexpr inline auto operator & (QuickSnapshotSave lhs,  rhs) -> EnumAnd<QuickSnapshotSave>
{ return rhs & lhs; }
Q_DECLARE_METATYPE()
#endif

template<>
class EnumInfo<QuickSnapshotSave> {
    typedef QuickSnapshotSave Enum;
public:
    typedef QuickSnapshotSave type;
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
    { return "QuickSnapshotSave"; }
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
        case Enum::Fixed: return qApp->translate("EnumInfo", "");
        case Enum::Current: return qApp->translate("EnumInfo", "");
        case Enum::Ask: return qApp->translate("EnumInfo", "");
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
    { return QuickSnapshotSave::Fixed; }
private:
    static const ItemList info;
};

using QuickSnapshotSaveInfo = EnumInfo<QuickSnapshotSave>;

#endif
