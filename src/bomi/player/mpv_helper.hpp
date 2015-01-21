#ifndef MPV_HELPER_HPP
#define MPV_HELPER_HPP

extern "C" {
#include <options/m_option.h>
}

#ifdef bool
#undef bool
#endif

template<class T>
SIA address_cast(const char *address, int base = 10)
-> typename std::enable_if<std::is_pointer<T>::value, T>::type
{
    bool ok = false;
    const quintptr ptr = QString::fromLatin1(address).toULongLong(&ok, base);
    return ok ? (T)(void*)(ptr) : (T)nullptr;
}

template<class T, class U>
SIA address_cast(U *ptr, int base = 10)
-> typename std::enable_if<std::is_same<T, QByteArray>::value
                           || std::is_same<T, QString>::value, T>::type
{
    return T::number((quint64)(quintptr)(void*)ptr, base);
}

namespace mpv {

template<class T>
static inline const m_option_type_t *get_option_type();
template<>
inline const m_option_type_t *get_option_type<char*>()
    { return &m_option_type_string; }
template<>
inline const m_option_type_t *get_option_type<int>()
    { return &m_option_type_int; }

static m_option null_option;

SIA make_option(const char *name, int offset,
                const m_option_type_t *type) -> m_option
{
    m_option opt = null_option;
    opt.name = name;
    opt.offset = offset;
    opt.type = type;
    return opt;
}

}

#define MPV_OPTION(member) mpv::make_option(#member, \
    offsetof(MPV_OPTION_BASE, member), mpv::get_option_type<decltype(MPV_OPTION_BASE::member)>())

#endif // MPV_HELPER_HPP
