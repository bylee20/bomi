#ifndef ENUMFLAGS_HPP
#define ENUMFLAGS_HPP

#include "global.hpp"
#include <QJsonArray>
#include <QDebug>

template<class T>
class EnumFlags;

template<class T>
using UnderType = typename std::underlying_type<T>::type;

template<class T>
class EnumNot {
    friend class EnumFlags<T>;
    friend constexpr auto operator ~ (T rhs) -> EnumNot<T>;
    constexpr EnumNot(T flag) noexcept : m(~(UnderType<T>)flag) { }
    UnderType<T> m;
};

template<class T>
class EnumAnd {
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

    auto operator &= (Flags rhs) noexcept -> Flags&
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
    auto toJson() const -> QJsonArray
    {
        QJsonArray json;
        for (auto &item : EnumInfo<T>::items()) {
            if (contains(item.value))
                json.push_back(item.name);
        }
        return json;
    }
    auto setFromJson(const QJsonArray &json) -> bool
    {
        IntType m = 0;
        for (auto it : json)
            m |= (IntType)EnumInfo<T>::from(it.toString(), (T)0);
        this->m = m;
        return true;
    }
    auto toString() -> QString
    {
        QString ret;
        for (auto &item : EnumInfo<T>::items()) {
            if (contains(item.value))
                ret += item.name % '|'_q;
        }
        ret.chop(1);
        return ret;
    }
    static auto fromString(const QString &str) -> EnumFlags<T>
    {
        const auto list = str.split('|'_q);
        EnumFlags<T> flags = 0;
        for (auto &s : list)
            flags |= _EnumFrom<T>(s);
        return flags;
    }
    auto description() const -> QString
    {
        QString ret;
        for (auto &item : EnumInfo<T>::items()) {
            if (contains(item.value))
                ret += EnumInfo<T>::description(item.value) % ','_q;
        }
        ret.chop(1); return ret;
    }
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

template<class T>
auto _EnumFlagsDescription(EnumFlags<T> flags) -> QString
{
    return flags.description();
}


#endif // ENUMFLAGS_HPP
