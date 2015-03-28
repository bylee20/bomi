#ifndef FRAMEBUFFEROBJECTFORMAT_HPP
#define FRAMEBUFFEROBJECTFORMAT_HPP

#include "enums.hpp"
#define FRAMEBUFFEROBJECTFORMAT_IS_FLAG 0
#include "opengl/openglmisc.hpp"

enum class FramebufferObjectFormat : int {
    Auto = (int)0,
    Rgba8 = (int)1,
    Rgba16 = (int)2
};

Q_DECLARE_METATYPE(FramebufferObjectFormat)

constexpr inline auto operator == (FramebufferObjectFormat e, int i) -> bool { return (int)e == i; }
constexpr inline auto operator != (FramebufferObjectFormat e, int i) -> bool { return (int)e != i; }
constexpr inline auto operator == (int i, FramebufferObjectFormat e) -> bool { return (int)e == i; }
constexpr inline auto operator != (int i, FramebufferObjectFormat e) -> bool { return (int)e != i; }
constexpr inline auto operator > (FramebufferObjectFormat e, int i) -> bool { return (int)e > i; }
constexpr inline auto operator < (FramebufferObjectFormat e, int i) -> bool { return (int)e < i; }
constexpr inline auto operator >= (FramebufferObjectFormat e, int i) -> bool { return (int)e >= i; }
constexpr inline auto operator <= (FramebufferObjectFormat e, int i) -> bool { return (int)e <= i; }
constexpr inline auto operator > (int i, FramebufferObjectFormat e) -> bool { return i > (int)e; }
constexpr inline auto operator < (int i, FramebufferObjectFormat e) -> bool { return i < (int)e; }
constexpr inline auto operator >= (int i, FramebufferObjectFormat e) -> bool { return i >= (int)e; }
constexpr inline auto operator <= (int i, FramebufferObjectFormat e) -> bool { return i <= (int)e; }
#if FRAMEBUFFEROBJECTFORMAT_IS_FLAG
#include "enumflags.hpp"
using  = EnumFlags<FramebufferObjectFormat>;
constexpr inline auto operator | (FramebufferObjectFormat e1, FramebufferObjectFormat e2) -> 
{ return (::IntType(e1) | ::IntType(e2)); }
constexpr inline auto operator ~ (FramebufferObjectFormat e) -> EnumNot<FramebufferObjectFormat>
{ return EnumNot<FramebufferObjectFormat>(e); }
constexpr inline auto operator & (FramebufferObjectFormat lhs,  rhs) -> EnumAnd<FramebufferObjectFormat>
{ return rhs & lhs; }
Q_DECLARE_METATYPE()
#endif

template<>
class EnumInfo<FramebufferObjectFormat> {
    typedef FramebufferObjectFormat Enum;
public:
    typedef FramebufferObjectFormat type;
    using Data =  OGL::TextureFormat;
    struct Item {
        Enum value;
        QString name, key;
        OGL::TextureFormat data;
    };
    using ItemList = std::array<Item, 3>;
    static constexpr auto size() -> int
    { return 3; }
    static constexpr auto typeName() -> const char*
    { return "FramebufferObjectFormat"; }
    static constexpr auto typeKey() -> const char*
    { return "texture"; }
    static auto typeDescription() -> QString
    { return qApp->translate("EnumInfo", "Texture Format"); }
    static auto item(Enum e) -> const Item*
    { return 0 <= e && e < size() ? &info[(int)e] : nullptr; }
    static auto name(Enum e) -> QString
    { auto i = item(e); return i ? i->name : QString(); }
    static auto key(Enum e) -> QString
    { auto i = item(e); return i ? i->key : QString(); }
    static auto data(Enum e) -> OGL::TextureFormat
    { auto i = item(e); return i ? i->data : OGL::TextureFormat(); }
    static auto description(int e) -> QString
    { return description((Enum)e); }
    static auto description(Enum e) -> QString
    {
        switch (e) {
        case Enum::Auto: return qApp->translate("EnumInfo", "Auto");
        case Enum::Rgba8: return qApp->translate("EnumInfo", "8-bit");
        case Enum::Rgba16: return qApp->translate("EnumInfo", "16-bit");
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
    static auto fromData(const OGL::TextureFormat &data,
                         Enum def = default_()) -> Enum
    {
        auto it = std::find_if(info.cbegin(), info.cend(),
                               [&data] (const Item &item)
                               { return item.data == data; });
        return it != info.cend() ? it->value : def;
    }
    static constexpr auto default_() -> Enum
    { return FramebufferObjectFormat::Auto; }
private:
    static const ItemList info;
};

using FramebufferObjectFormatInfo = EnumInfo<FramebufferObjectFormat>;

#endif
