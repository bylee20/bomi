#ifndef ENUMFLAGS_HPP
#define ENUMFLAGS_HPP

#include "stdafx.hpp"

template<class T>
class EnumFlags;

template<class T>
using UnderType = typename std::underlying_type<T>::type;

template<class T>
class EnumNot {
    static_assert(std::is_enum_class<T>::value, "wrong enum type." );
    friend class EnumFlags<T>;
    friend constexpr auto operator ~ (T rhs) -> EnumNot<T>;
    constexpr EnumNot(T flag) noexcept : m(~(UnderType<T>)flag) { }
    UnderType<T> m;
};

template<class T>
class EnumAnd {
    static_assert(std::is_enum_class<T>::value, "wrong enum type." );
public:
    constexpr EnumAnd(T t) noexcept : m(t) { }
    constexpr operator bool() const noexcept { return (UnderType<T>)m; }
    constexpr operator T() const noexcept { return m; }
    constexpr operator EnumFlags<T>() const noexcept { return m; }
private:
    T m;
};

template<class T>
class EnumFlags {
    static_assert(std::is_enum_class<T>::value, "wrong enum type." );
    using IntType = UnderType<T>;
    using Flags = EnumFlags<T>;
    constexpr explicit EnumFlags(IntType m) noexcept : m(m) { }
    friend constexpr auto operator | (T lhs, T rhs) -> Flags;
    friend constexpr auto operator | (T lhs, Flags rhs) -> Flags;
    struct Private;
    using Zero = int(Private::*);
public:
    constexpr EnumFlags(Zero = 0) noexcept : m(0) { }
    constexpr EnumFlags(T flag) noexcept : m(static_cast<IntType>(flag)) { }

    constexpr explicit operator IntType() const noexcept { return m; }
    constexpr operator bool() const noexcept { return m; }

    auto operator |= (Flags rhs) noexcept -> Flags&
        { m |= rhs.m; return *this; }
    constexpr auto operator | (Flags rhs) const noexcept -> Flags
        { return Flags(m | rhs.m); }
    constexpr auto operator | (T rhs) const noexcept -> Flags
        { return Flags(m | (IntType)rhs); }

    constexpr auto operator &= (Flags rhs) noexcept -> Flags&
        { m &= rhs.m; return *this; }
    auto operator &= (EnumNot<T> rhs) noexcept -> Flags&
        { m &= rhs.m; return *this; }
    constexpr auto operator & (Flags rhs) const noexcept -> Flags
        { return Flags(m & rhs.m); }
    constexpr auto operator & (T rhs) const noexcept -> EnumAnd<T>
        { return T(m & (IntType)rhs); }

    constexpr auto operator == (Flags rhs) const noexcept -> bool
        { return m == rhs.m; }
    constexpr auto operator == (T rhs) const noexcept -> bool
        { return m == (IntType)rhs; }
    constexpr auto operator == (IntType rhs) const noexcept -> bool
        { return m == rhs; }
    constexpr auto operator != (Flags rhs) const noexcept -> bool
        { return m != rhs.m; }
    constexpr auto operator != (T rhs) const noexcept -> bool
        { return m != (IntType)rhs; }
    constexpr auto operator != (IntType rhs) const noexcept -> bool
        { return m != rhs; }

    constexpr auto contains(T t) const noexcept -> bool
        { return m & (IntType)t; }
    auto clear() noexcept -> void { m = 0; }
    auto set(T t) noexcept -> void { m |= (IntType)t; }
    auto set(T t, bool on) noexcept -> void { on ? set(t) : unset(t); }
    auto unset(T t) noexcept -> void { m &= ~(IntType)t; }
private:
    IntType m = 0;
};

template<class T>
class EnumInfo;

template<class T>
inline QDebug operator << (QDebug dbg, EnumFlags<T> flags)
{
    dbg.nospace() << '(';
    int count = 0;
    for (auto &item : EnumInfo<T>::items()) {
        if (flags.contains(item.value)) {
            if (count > 0)
                dbg << '|';
            dbg << EnumInfo<T>::typeName()
                << "::" << item.name.toLatin1().constData();
            ++count;
        }
    }
    dbg << ')';
    return dbg.space();
}

#endif // ENUMFLAGS_HPP
