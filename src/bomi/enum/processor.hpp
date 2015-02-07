#ifndef PROCESSOR_HPP
#define PROCESSOR_HPP

#include "enums.hpp"
#define PROCESSOR_IS_FLAG 1

enum class Processor : int {
    None = (int)0,
    CPU = (int)1,
    GPU = (int)2
};

Q_DECLARE_METATYPE(Processor)

constexpr inline auto operator == (Processor e, int i) -> bool { return (int)e == i; }
constexpr inline auto operator != (Processor e, int i) -> bool { return (int)e != i; }
constexpr inline auto operator == (int i, Processor e) -> bool { return (int)e == i; }
constexpr inline auto operator != (int i, Processor e) -> bool { return (int)e != i; }
constexpr inline auto operator > (Processor e, int i) -> bool { return (int)e > i; }
constexpr inline auto operator < (Processor e, int i) -> bool { return (int)e < i; }
constexpr inline auto operator >= (Processor e, int i) -> bool { return (int)e >= i; }
constexpr inline auto operator <= (Processor e, int i) -> bool { return (int)e <= i; }
constexpr inline auto operator > (int i, Processor e) -> bool { return i > (int)e; }
constexpr inline auto operator < (int i, Processor e) -> bool { return i < (int)e; }
constexpr inline auto operator >= (int i, Processor e) -> bool { return i >= (int)e; }
constexpr inline auto operator <= (int i, Processor e) -> bool { return i <= (int)e; }
#if PROCESSOR_IS_FLAG
#include "enumflags.hpp"
using Processors = EnumFlags<Processor>;
constexpr inline auto operator | (Processor e1, Processor e2) -> Processors
{ return Processors(Processors::IntType(e1) | Processors::IntType(e2)); }
constexpr inline auto operator ~ (Processor e) -> EnumNot<Processor>
{ return EnumNot<Processor>(e); }
constexpr inline auto operator & (Processor lhs, Processors rhs) -> EnumAnd<Processor>
{ return rhs & lhs; }
Q_DECLARE_METATYPE(Processors)
#endif

template<>
class EnumInfo<Processor> {
    typedef Processor Enum;
public:
    typedef Processor type;
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
    { return "Processor"; }
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
    { return Processor::None; }
private:
    static const ItemList info;
};

using ProcessorInfo = EnumInfo<Processor>;

#endif
