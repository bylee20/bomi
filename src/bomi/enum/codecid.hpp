#ifndef CODECID_HPP
#define CODECID_HPP

#include "enums.hpp"
#define CODECID_IS_FLAG 0

enum class CodecId : int {
    Invalid = (int)0,
    Mpeg1 = (int)1,
    Mpeg2 = (int)2,
    Mpeg4 = (int)3,
    H264 = (int)4,
    Vc1 = (int)5,
    Wmv3 = (int)6,
    Hevc = (int)7
};

Q_DECLARE_METATYPE(CodecId)

constexpr inline auto operator == (CodecId e, int i) -> bool { return (int)e == i; }
constexpr inline auto operator != (CodecId e, int i) -> bool { return (int)e != i; }
constexpr inline auto operator == (int i, CodecId e) -> bool { return (int)e == i; }
constexpr inline auto operator != (int i, CodecId e) -> bool { return (int)e != i; }
constexpr inline auto operator > (CodecId e, int i) -> bool { return (int)e > i; }
constexpr inline auto operator < (CodecId e, int i) -> bool { return (int)e < i; }
constexpr inline auto operator >= (CodecId e, int i) -> bool { return (int)e >= i; }
constexpr inline auto operator <= (CodecId e, int i) -> bool { return (int)e <= i; }
constexpr inline auto operator > (int i, CodecId e) -> bool { return i > (int)e; }
constexpr inline auto operator < (int i, CodecId e) -> bool { return i < (int)e; }
constexpr inline auto operator >= (int i, CodecId e) -> bool { return i >= (int)e; }
constexpr inline auto operator <= (int i, CodecId e) -> bool { return i <= (int)e; }
#if CODECID_IS_FLAG
#include "enumflags.hpp"
using  = EnumFlags<CodecId>;
constexpr inline auto operator | (CodecId e1, CodecId e2) -> 
{ return (::IntType(e1) | ::IntType(e2)); }
constexpr inline auto operator ~ (CodecId e) -> EnumNot<CodecId>
{ return EnumNot<CodecId>(e); }
constexpr inline auto operator & (CodecId lhs,  rhs) -> EnumAnd<CodecId>
{ return rhs & lhs; }
Q_DECLARE_METATYPE()
#endif

template<>
class EnumInfo<CodecId> {
    typedef CodecId Enum;
public:
    typedef CodecId type;
    using Data =  QString;
    struct Item {
        Enum value;
        QString name, key;
        QString data;
    };
    using ItemList = std::array<Item, 8>;
    static constexpr auto size() -> int
    { return 8; }
    static constexpr auto typeName() -> const char*
    { return "CodecId"; }
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
    static auto data(Enum e) -> QString
    { auto i = item(e); return i ? i->data : QString(); }
    static auto description(int e) -> QString
    { return description((Enum)e); }
    static auto description(Enum e) -> QString
    {
        switch (e) {
        case Enum::Invalid: return qApp->translate("EnumInfo", "Not available");
        case Enum::Mpeg1: return qApp->translate("EnumInfo", "MPEG-1 video");
        case Enum::Mpeg2: return qApp->translate("EnumInfo", "MPEG-2 video");
        case Enum::Mpeg4: return qApp->translate("EnumInfo", "MPEG-4 part 2");
        case Enum::H264: return qApp->translate("EnumInfo", "H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10");
        case Enum::Vc1: return qApp->translate("EnumInfo", "SMPTE VC-1");
        case Enum::Wmv3: return qApp->translate("EnumInfo", "Windows Media Video 9");
        case Enum::Hevc: return qApp->translate("EnumInfo", "H.265 / HEVC (High Efficiency Video Coding)");
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
    static auto fromData(const QString &data,
                         Enum def = default_()) -> Enum
    {
        auto it = std::find_if(info.cbegin(), info.cend(),
                               [&data] (const Item &item)
                               { return item.data == data; });
        return it != info.cend() ? it->value : def;
    }
    static constexpr auto default_() -> Enum
    { return CodecId::Invalid; }
private:
    static const ItemList info;
};

using CodecIdInfo = EnumInfo<CodecId>;

#endif
