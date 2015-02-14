#ifndef STATIC_FOR_HPP
#define STATIC_FOR_HPP

#include "global_def.hpp"

namespace tmp {

namespace detail {
template<int idx, int size, bool go = idx < size>
struct static_for { };

template<int idx, int size>
struct static_for<idx, size, true> {
    template<class... Args, class F>
    SIA run_each(const std::tuple<Args...> &tuple, const F &func) -> void
    {
        func(std::get<idx>(tuple));
        static_for<idx+1, size>::run_each(tuple, func);
    }
};

template<int idx, int size>
struct static_for<idx, size, false> {
    template<class T, class F>
    SIA run_each(const T &, const F &) -> void { }
    template<class T, class F>
    SIA run_each(T &&, const F &) -> void { }
};

template<int idx, int size, bool go = idx < size>
struct static_for_breakable { };

template<int idx, int size>
struct static_for_breakable<idx, size, true> {
    template<class... Args, class F>
    SIA run_each(const std::tuple<Args...> &tuple, const F &func) -> bool
    {
        return func(std::get<idx>(tuple))
               && static_for_breakable<idx+1, size>::run_each(tuple, func);
    }
};

template<int idx, int size>
struct static_for_breakable<idx, size, false> {
    template<class T, class F>
    SCIA run_each(const T &, const F &) -> bool { return true; }
    template<class T, class F>
    SCIA run_each(T &&, const F &) -> bool { return true; }
};
}

template<int idx, int size, class... Args, class F>
SIA static_for_each(const std::tuple<Args...> &tuple, const F &func) -> void
{ detail::static_for<idx, size>::run_each(tuple, func); }


template<int idx, int size, class... Args, class F>
SIA static_for_each_breakable(const std::tuple<Args...> &tuple, const F &func) -> bool
{ return detail::static_for_breakable<idx, size>::run_each(tuple, func); }

}

#endif // STATIC_FOR_HPP
