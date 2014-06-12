#ifndef ALGORITHM_HPP
#define ALGORITHM_HPP

namespace tmp {

template<class Container, class Test>
SIA find_if(const Container &c, Test test) -> typename Container::const_iterator
{ return std::find_if(std::begin(c), std::end(c), test); }

template<class Container, class Test>
SIA contains_if(const Container &c, Test test) -> bool
{
    const auto end = std::end(c);
    return std::find_if(std::begin(c), end, test) != end;
}

template<class Container, class T>
SIA find(const Container &c, const T &t) -> typename Container::const_iterator
{ return std::find(std::begin(c), std::end(c), t); }

template<class Container, class T>
SIA contains(const Container &c, const T &t) -> bool
{
    const auto end = std::end(c);
    return std::find(std::begin(c), end, t) != end;
}

template<class Container, class F>
SIA transform(Container &c, F f) -> Container&
{ for (auto &item : c) f(item); return c; }

template<class Container, class F>
SIA transformed(const Container &c, F f) -> Container
{ Container ret = c; return transform<Container, F>(ret, f); }

}

#endif // ALGORITHM_HPP
