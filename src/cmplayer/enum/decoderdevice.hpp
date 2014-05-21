#ifndef DECODERDEVICE_HPP
#define DECODERDEVICE_HPP

#include "enums.hpp"
#define DECODERDEVICE_IS_FLAG 0

enum class DecoderDevice : int {
    None = (int)0,
    CPU = (int)1,
    GPU = (int)2
};

Q_DECLARE_METATYPE(DecoderDevice)

inline auto operator == (DecoderDevice e, int i) -> bool { return (int)e == i; }
inline auto operator != (DecoderDevice e, int i) -> bool { return (int)e != i; }
inline auto operator == (int i, DecoderDevice e) -> bool { return (int)e == i; }
inline auto operator != (int i, DecoderDevice e) -> bool { return (int)e != i; }
inline auto operator > (DecoderDevice e, int i) -> bool { return (int)e > i; }
inline auto operator < (DecoderDevice e, int i) -> bool { return (int)e < i; }
inline auto operator >= (DecoderDevice e, int i) -> bool { return (int)e >= i; }
inline auto operator <= (DecoderDevice e, int i) -> bool { return (int)e <= i; }
inline auto operator > (int i, DecoderDevice e) -> bool { return i > (int)e; }
inline auto operator < (int i, DecoderDevice e) -> bool { return i < (int)e; }
inline auto operator >= (int i, DecoderDevice e) -> bool { return i >= (int)e; }
inline auto operator <= (int i, DecoderDevice e) -> bool { return i <= (int)e; }
#if DECODERDEVICE_IS_FLAG
Q_DECLARE_FLAGS(, DecoderDevice)
Q_DECLARE_OPERATORS_FOR_FLAGS()
Q_DECLARE_METATYPE()
#else
inline auto operator & (DecoderDevice e, int i) -> int { return (int)e & i; }
inline auto operator & (int i, DecoderDevice e) -> int { return (int)e & i; }
inline auto operator &= (int &i, DecoderDevice e) -> int& { return i &= (int)e; }
inline auto operator ~ (DecoderDevice e) -> int { return ~(int)e; }
inline auto operator | (DecoderDevice e, int i) -> int { return (int)e | i; }
inline auto operator | (int i, DecoderDevice e) -> int { return (int)e | i; }
constexpr inline auto operator | (DecoderDevice e1, DecoderDevice e2) -> int { return (int)e1 | (int)e2; }
inline auto operator |= (int &i, DecoderDevice e) -> int& { return i |= (int)e; }
#endif

template<>
class EnumInfo<DecoderDevice> {
    typedef DecoderDevice Enum;
public:
    typedef DecoderDevice type;
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
    { return "DecoderDevice"; }
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
        case Enum::CPU: return qApp->translate("EnumInfo", "");
        case Enum::GPU: return qApp->translate("EnumInfo", "");
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
    { return DecoderDevice::None; }
private:
    static const ItemList info;
};

using DecoderDeviceInfo = EnumInfo<DecoderDevice>;

#endif
