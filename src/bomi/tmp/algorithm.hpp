#ifndef ALGORITHM_HPP
#define ALGORITHM_HPP

namespace tmp {

namespace detail {
template<class Container>
using it_type = typename std::conditional<std::is_const<Container>::value,
    typename Container::const_iterator, typename Container::iterator>::type;
}

template<class Iter, class Test>
SIA find_if(Iter begin, Iter end, Test test) -> Iter
{ return std::find_if(begin, end, test); }

template<class Container, class Test>
SIA find_if(Container &c, Test test) -> detail::it_type<Container>
{ return std::find_if(std::begin(c), std::end(c), test); }

template<class Iter, class Test>
SIA contains_if(Iter b, Iter e, Test test) -> bool
{ return std::find_if(b, e, test) != e; }

template<class Container, class Test>
SIA contains_if(const Container &c, Test test) -> bool
{ return contains_if(std::begin(c), std::end(c), test); }

template<class Iter, class T>
SIA find(Iter b, Iter e, const T &t) -> Iter
{ return std::find(b, e, t); }

template<class Container, class T>
SIA find(Container &c, const T &t) -> detail::it_type<Container>
{ return std::find(std::begin(c), std::end(c), t); }

template<class Iter, class T>
SIA contains(Iter b, Iter e, const T &t) -> bool
{ return std::find(b, e, t) != e; }

template<class Container, class T>
SIA contains(const Container &c, const T &t) -> bool
{ return contains(std::begin(c), std::end(c), t); }

template<class Iter, class T>
SIA contains_binary(Iter b, Iter e, const T &t) -> bool
{ return std::binary_search(b, e, t); }

template<class Container, class T>
SIA contains_binary(const Container &c, const T &t) -> bool
{ return contains_binary(std::begin(c), std::end(c), t); }

template<class Container, class F>
SIA transform(Container &c, F f) -> Container&
{ for (auto &item : c) f(item); return c; }

template<class Container, class F>
SIA transformed(const Container &c, F f) -> Container
{ Container ret = c; return transform<Container, F>(ret, f); }

template<class Container>
SIA sort(Container &c) -> void
{ std::sort(std::begin(c), std::end(c)); }

template<class Container, class Compare>
SIA sort(Container &c, Compare cmp) -> void
{ std::sort(std::begin(c), std::end(c), cmp); }

template<class T>
SIA max(const T &t1, const T &t2) -> T { return std::max(t1, t2); }

template<class T, class... Args>
SIA max(const T &t1, const T &t2, const Args&... args) -> T
    { return max(max(t1, t2), args...); }

template<class T>
SIA min(const T &t1, const T &t2) -> T { return std::min(t1, t2); }

template<class T, class... Args>
SIA min(const T &t1, const T &t2, const Args&... args) -> T
    { return min(min(t1, t2), args...); }

template<class Container, class T = typename Container::value_type>
SIA min(const Container &c) -> T
    { if (c.empty()) return T(); return *std::min_element(c.begin(), c.end()); }

template<class Container, class T = typename Container::value_type>
SIA max(const Container &c) -> T
    { if (c.empty()) return T(); return *std::max_element(c.begin(), c.end()); }

template<class Container, class T = typename Container::value_type>
SIA take_front(Container &c) -> T
    { if (c.empty()) return T(); T t = std::move(c.front()); c.pop_front(); return t; }

template<class Container, class T = typename Container::value_type>
SIA take_back(Container &c) -> T
    { if (c.empty()) return T(); T t = std::move(c.back()); c.pop_back(); return t; }

}

#endif // ALGORITHM_HPP
