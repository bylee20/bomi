#ifndef AUDIODRIVER_HPP
#define AUDIODRIVER_HPP

#include "enums.hpp"
#define AUDIODRIVER_IS_FLAG 0

enum class AudioDriver : int {
    Auto = (int)0,
    CoreAudio = (int)1,
    PulseAudio = (int)2,
    OSS = (int)3,
    ALSA = (int)4,
    JACK = (int)5,
    PortAudio = (int)6,
    OpenAL = (int)7
};

Q_DECLARE_METATYPE(AudioDriver)

constexpr inline auto operator == (AudioDriver e, int i) -> bool { return (int)e == i; }
constexpr inline auto operator != (AudioDriver e, int i) -> bool { return (int)e != i; }
constexpr inline auto operator == (int i, AudioDriver e) -> bool { return (int)e == i; }
constexpr inline auto operator != (int i, AudioDriver e) -> bool { return (int)e != i; }
constexpr inline auto operator > (AudioDriver e, int i) -> bool { return (int)e > i; }
constexpr inline auto operator < (AudioDriver e, int i) -> bool { return (int)e < i; }
constexpr inline auto operator >= (AudioDriver e, int i) -> bool { return (int)e >= i; }
constexpr inline auto operator <= (AudioDriver e, int i) -> bool { return (int)e <= i; }
constexpr inline auto operator > (int i, AudioDriver e) -> bool { return i > (int)e; }
constexpr inline auto operator < (int i, AudioDriver e) -> bool { return i < (int)e; }
constexpr inline auto operator >= (int i, AudioDriver e) -> bool { return i >= (int)e; }
constexpr inline auto operator <= (int i, AudioDriver e) -> bool { return i <= (int)e; }
#if AUDIODRIVER_IS_FLAG
#include "enumflags.hpp"
using  = EnumFlags<AudioDriver>;
constexpr inline auto operator | (AudioDriver e1, AudioDriver e2) -> 
{ return (::IntType(e1) | ::IntType(e2)); }
constexpr inline auto operator ~ (AudioDriver e) -> EnumNot<AudioDriver>
{ return EnumNot<AudioDriver>(e); }
constexpr inline auto operator & (AudioDriver lhs,  rhs) -> EnumAnd<AudioDriver>
{ return rhs & lhs; }
Q_DECLARE_METATYPE()
#endif

template<>
class EnumInfo<AudioDriver> {
    typedef AudioDriver Enum;
public:
    typedef AudioDriver type;
    using Data =  QVariant;
    struct Item {
        Enum value;
        QString name, key;
        QVariant data;
    };
    using ItemList = std::array<Item, 8>;
    static constexpr auto size() -> int
    { return 8; }
    static constexpr auto typeName() -> const char*
    { return "AudioDriver"; }
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
        case Enum::Auto: return qApp->translate("EnumInfo", "");
        case Enum::CoreAudio: return qApp->translate("EnumInfo", "");
        case Enum::PulseAudio: return qApp->translate("EnumInfo", "");
        case Enum::OSS: return qApp->translate("EnumInfo", "");
        case Enum::ALSA: return qApp->translate("EnumInfo", "");
        case Enum::JACK: return qApp->translate("EnumInfo", "");
        case Enum::PortAudio: return qApp->translate("EnumInfo", "");
        case Enum::OpenAL: return qApp->translate("EnumInfo", "");
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
    { return AudioDriver::Auto; }
private:
    static const ItemList info;
};

using AudioDriverInfo = EnumInfo<AudioDriver>;

#endif
