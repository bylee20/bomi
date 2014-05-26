#ifndef __ENUM_UPPERS_HPP
#define __ENUM_UPPERS_HPP

#include "enums.hpp"
#define __ENUM_UPPERS_IS_FLAG __ENUM_IS_FLAG
__ENUM_HEADER_CONTENTS
enum class __ENUM_NAME : int {
__ENUM_VALUES
};

Q_DECLARE_METATYPE(__ENUM_NAME)

constexpr inline auto operator == (__ENUM_NAME e, int i) -> bool { return (int)e == i; }
constexpr inline auto operator != (__ENUM_NAME e, int i) -> bool { return (int)e != i; }
constexpr inline auto operator == (int i, __ENUM_NAME e) -> bool { return (int)e == i; }
constexpr inline auto operator != (int i, __ENUM_NAME e) -> bool { return (int)e != i; }
constexpr inline auto operator > (__ENUM_NAME e, int i) -> bool { return (int)e > i; }
constexpr inline auto operator < (__ENUM_NAME e, int i) -> bool { return (int)e < i; }
constexpr inline auto operator >= (__ENUM_NAME e, int i) -> bool { return (int)e >= i; }
constexpr inline auto operator <= (__ENUM_NAME e, int i) -> bool { return (int)e <= i; }
constexpr inline auto operator > (int i, __ENUM_NAME e) -> bool { return i > (int)e; }
constexpr inline auto operator < (int i, __ENUM_NAME e) -> bool { return i < (int)e; }
constexpr inline auto operator >= (int i, __ENUM_NAME e) -> bool { return i >= (int)e; }
constexpr inline auto operator <= (int i, __ENUM_NAME e) -> bool { return i <= (int)e; }
#if __ENUM_UPPERS_IS_FLAG
#include "enumflags.hpp"
using __ENUM_FLAGS_NAME = EnumFlags<__ENUM_NAME>;
constexpr inline auto operator | (__ENUM_NAME e1, __ENUM_NAME e2) -> __ENUM_FLAGS_NAME
{ return __ENUM_FLAGS_NAME(__ENUM_FLAGS_NAME::IntType(e1) | __ENUM_FLAGS_NAME::IntType(e2)); }
constexpr inline auto operator ~ (__ENUM_NAME e) -> EnumNot<__ENUM_NAME>
{ return EnumNot<__ENUM_NAME>(e); }
constexpr inline auto operator & (__ENUM_NAME lhs, __ENUM_FLAGS_NAME rhs) -> EnumAnd<__ENUM_NAME>
{ return rhs & lhs; }
Q_DECLARE_METATYPE(__ENUM_FLAGS_NAME)
#endif

template<>
class EnumInfo<__ENUM_NAME> {
    typedef __ENUM_NAME Enum;
public:
    typedef __ENUM_NAME type;
    using Data =  __ENUM_DATA_TYPE;
    struct Item {
        Enum value;
        QString name, key;
        __ENUM_DATA_TYPE data;
    };
    using ItemList = std::array<Item, __ENUM_COUNT>;
    static constexpr auto size() -> int
    { return __ENUM_COUNT; }
    static constexpr auto typeName() -> const char*
    { return "__ENUM_NAME"; }
    static constexpr auto typeKey() -> const char*
    { return "__ENUM_KEY"; }
    static auto typeDescription() -> QString
    { return qApp->translate("EnumInfo", "__ENUM_DESC"); }
    static auto item(Enum e) -> const Item*
    { __ENUM_FUNC_ITEM }
    static auto name(Enum e) -> QString
    { auto i = item(e); return i ? i->name : QString(); }
    static auto key(Enum e) -> QString
    { auto i = item(e); return i ? i->key : QString(); }
    static auto data(Enum e) -> __ENUM_DATA_TYPE
    { auto i = item(e); return i ? i->data : __ENUM_DATA_TYPE(); }
    static auto description(int e) -> QString
    { return description((Enum)e); }
    static auto description(Enum e) -> QString
    {
        switch (e) {
__ENUM_FUNC_DESC_CASES
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
    static auto fromData(const __ENUM_DATA_TYPE &data,
                         Enum def = default_()) -> Enum
    {
        auto it = std::find_if(info.cbegin(), info.cend(),
                               [&data] (const Item &item)
                               { return item.data == data; });
        return it != info.cend() ? it->value : def;
    }
    static constexpr auto default_() -> Enum
    { return __ENUM_NAME::__ENUM_DEFAULT; }
private:
    static const ItemList info;
};

using __ENUM_NAMEInfo = EnumInfo<__ENUM_NAME>;
__ENUM_FOOTER_CONTENTS
#endif
