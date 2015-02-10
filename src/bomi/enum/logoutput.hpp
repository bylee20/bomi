#ifndef LOGOUTPUT_HPP
#define LOGOUTPUT_HPP

#include "enums.hpp"
#define LOGOUTPUT_IS_FLAG 0

enum class LogOutput : int {
    Off = (int)0,
    StdOut = (int)1,
    StdErr = (int)2,
    File = (int)3,
    Journal = (int)4,
    Viewer = (int)5
};

Q_DECLARE_METATYPE(LogOutput)

constexpr inline auto operator == (LogOutput e, int i) -> bool { return (int)e == i; }
constexpr inline auto operator != (LogOutput e, int i) -> bool { return (int)e != i; }
constexpr inline auto operator == (int i, LogOutput e) -> bool { return (int)e == i; }
constexpr inline auto operator != (int i, LogOutput e) -> bool { return (int)e != i; }
constexpr inline auto operator > (LogOutput e, int i) -> bool { return (int)e > i; }
constexpr inline auto operator < (LogOutput e, int i) -> bool { return (int)e < i; }
constexpr inline auto operator >= (LogOutput e, int i) -> bool { return (int)e >= i; }
constexpr inline auto operator <= (LogOutput e, int i) -> bool { return (int)e <= i; }
constexpr inline auto operator > (int i, LogOutput e) -> bool { return i > (int)e; }
constexpr inline auto operator < (int i, LogOutput e) -> bool { return i < (int)e; }
constexpr inline auto operator >= (int i, LogOutput e) -> bool { return i >= (int)e; }
constexpr inline auto operator <= (int i, LogOutput e) -> bool { return i <= (int)e; }
#if LOGOUTPUT_IS_FLAG
#include "enumflags.hpp"
using  = EnumFlags<LogOutput>;
constexpr inline auto operator | (LogOutput e1, LogOutput e2) -> 
{ return (::IntType(e1) | ::IntType(e2)); }
constexpr inline auto operator ~ (LogOutput e) -> EnumNot<LogOutput>
{ return EnumNot<LogOutput>(e); }
constexpr inline auto operator & (LogOutput lhs,  rhs) -> EnumAnd<LogOutput>
{ return rhs & lhs; }
Q_DECLARE_METATYPE()
#endif

template<>
class EnumInfo<LogOutput> {
    typedef LogOutput Enum;
public:
    typedef LogOutput type;
    using Data =  QVariant;
    struct Item {
        Enum value;
        QString name, key;
        QVariant data;
    };
    using ItemList = std::array<Item, 6>;
    static constexpr auto size() -> int
    { return 6; }
    static constexpr auto typeName() -> const char*
    { return "LogOutput"; }
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
        case Enum::Off: return qApp->translate("EnumInfo", "No Output");
        case Enum::StdOut: return qApp->translate("EnumInfo", "stdout");
        case Enum::StdErr: return qApp->translate("EnumInfo", "stderr");
        case Enum::File: return qApp->translate("EnumInfo", "Text File");
        case Enum::Journal: return qApp->translate("EnumInfo", "journald");
        case Enum::Viewer: return qApp->translate("EnumInfo", "Viewer");
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
    { return LogOutput::Off; }
private:
    static const ItemList info;
};

using LogOutputInfo = EnumInfo<LogOutput>;

#endif
