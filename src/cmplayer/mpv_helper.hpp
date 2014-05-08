#ifndef MPV_HELPER_HPP
#define MPV_HELPER_HPP

extern "C" {
#include <options/m_option.h>
}

namespace mpv {

template<class T> static inline const m_option_type_t *get_option_type();
template<> inline const m_option_type_t *get_option_type<char*>() { return &m_option_type_string; }
template<> inline const m_option_type_t *get_option_type<int>() { return &m_option_type_int; }

static m_option null_option;

static inline auto make_option(const char *name, int offset, const m_option_type_t *type) -> m_option {
    m_option opt = null_option;
    opt.name = name;
    opt.offset = offset;
    opt.is_new_option = 1;
    opt.type = type;
    return opt;
}

}

#define MPV_OPTION(member) mpv::make_option(#member, \
    offsetof(MPV_OPTION_BASE, member), mpv::get_option_type<decltype(MPV_OPTION_BASE::member)>())

#endif // MPV_HELPER_HPP
