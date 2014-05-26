#ifndef DEINTDEVICE_HPP
#define DEINTDEVICE_HPP

#include "enums.hpp"
#define DEINTDEVICE_IS_FLAG 1

enum class DeintDevice : int {
    None = (int)0,
    CPU = (int)1,
    GPU = (int)2,
    OpenGL = (int)4
};

Q_DECLARE_METATYPE(DeintDevice)

constexpr inline auto operator == (DeintDevice e, int i) -> bool { return (int)e == i; }
constexpr inline auto operator != (DeintDevice e, int i) -> bool { return (int)e != i; }
constexpr inline auto operator == (int i, DeintDevice e) -> bool { return (int)e == i; }
constexpr inline auto operator != (int i, DeintDevice e) -> bool { return (int)e != i; }
constexpr inline auto operator > (DeintDevice e, int i) -> bool { return (int)e > i; }
constexpr inline auto operator < (DeintDevice e, int i) -> bool { return (int)e < i; }
constexpr inline auto operator >= (DeintDevice e, int i) -> bool { return (int)e >= i; }
constexpr inline auto operator <= (DeintDevice e, int i) -> bool { return (int)e <= i; }
constexpr inline auto operator > (int i, DeintDevice e) -> bool { return i > (int)e; }
constexpr inline auto operator < (int i, DeintDevice e) -> bool { return i < (int)e; }
constexpr inline auto operator >= (int i, DeintDevice e) -> bool { return i >= (int)e; }
constexpr inline auto operator <= (int i, DeintDevice e) -> bool { return i <= (int)e; }
#if DEINTDEVICE_IS_FLAG
#include "enumflags.hpp"
using DeintDevices = EnumFlags<DeintDevice>;
constexpr inline auto operator | (DeintDevice e1, DeintDevice e2) -> DeintDevices
{ return DeintDevices(DeintDevices::IntType(e1) | DeintDevices::IntType(e2)); }
constexpr inline auto operator ~ (DeintDevice e) -> EnumNot<DeintDevice>
{ return EnumNot<DeintDevice>(e); }
constexpr inline auto operator & (DeintDevice lhs, DeintDevices rhs) -> EnumAnd<DeintDevice>
{ return rhs & lhs; }
Q_DECLARE_METATYPE(DeintDevices)
#endif

template<>
class EnumInfo<DeintDevice> {
    typedef DeintDevice Enum;
public:
    typedef DeintDevice type;
    using Data =  QVariant;
    struct Item {
        Enum value;
        QString name, key;
        QVariant data;
    };
    using ItemList = std::array<Item, 4>;
    static constexpr auto size() -> int
    { return 4; }
    static constexpr auto typeName() -> const char*
    { return "DeintDevice"; }
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
        case Enum::OpenGL: return qApp->translate("EnumInfo", "");
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
    { return DeintDevice::None; }
private:
    static const ItemList info;
};

using DeintDeviceInfo = EnumInfo<DeintDevice>;

#endif
