#ifndef JRCONNECTION_HPP
#define JRCONNECTION_HPP

#include "enums.hpp"
#define JRCONNECTION_IS_FLAG 0

enum class JrConnection : int {
    Tcp = (int)0,
    Local = (int)1
};

Q_DECLARE_METATYPE(JrConnection)

constexpr inline auto operator == (JrConnection e, int i) -> bool { return (int)e == i; }
constexpr inline auto operator != (JrConnection e, int i) -> bool { return (int)e != i; }
constexpr inline auto operator == (int i, JrConnection e) -> bool { return (int)e == i; }
constexpr inline auto operator != (int i, JrConnection e) -> bool { return (int)e != i; }
constexpr inline auto operator > (JrConnection e, int i) -> bool { return (int)e > i; }
constexpr inline auto operator < (JrConnection e, int i) -> bool { return (int)e < i; }
constexpr inline auto operator >= (JrConnection e, int i) -> bool { return (int)e >= i; }
constexpr inline auto operator <= (JrConnection e, int i) -> bool { return (int)e <= i; }
constexpr inline auto operator > (int i, JrConnection e) -> bool { return i > (int)e; }
constexpr inline auto operator < (int i, JrConnection e) -> bool { return i < (int)e; }
constexpr inline auto operator >= (int i, JrConnection e) -> bool { return i >= (int)e; }
constexpr inline auto operator <= (int i, JrConnection e) -> bool { return i <= (int)e; }
#if JRCONNECTION_IS_FLAG
#include "enumflags.hpp"
using  = EnumFlags<JrConnection>;
constexpr inline auto operator | (JrConnection e1, JrConnection e2) -> 
{ return (::IntType(e1) | ::IntType(e2)); }
constexpr inline auto operator ~ (JrConnection e) -> EnumNot<JrConnection>
{ return EnumNot<JrConnection>(e); }
constexpr inline auto operator & (JrConnection lhs,  rhs) -> EnumAnd<JrConnection>
{ return rhs & lhs; }
Q_DECLARE_METATYPE()
#endif

template<>
class EnumInfo<JrConnection> {
    typedef JrConnection Enum;
public:
    typedef JrConnection type;
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
    { return "JrConnection"; }
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
        case Enum::Tcp: return qApp->translate("EnumInfo", "TCP socket");
        case Enum::Local: return qApp->translate("EnumInfo", "Local socket");
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
    { return JrConnection::Tcp; }
private:
    static const ItemList info;
};

using JrConnectionInfo = EnumInfo<JrConnection>;

#endif
