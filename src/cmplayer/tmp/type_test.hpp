#ifndef TYPE_TEST_HPP
#define TYPE_TEST_HPP

#include <type_traits>

namespace tmp {

template<class T>
SCIA is_integral() -> bool { return std::is_integral<T>::value; }

template<class T>
SCIA is_floating_point() -> bool { return std::is_floating_point<T>::value; }

template<class T>
SCIA is_arithmetic() -> bool { return std::is_arithmetic<T>::value; }

template<class T, class S>
SCIA is_same() -> bool { return std::is_same<T, S>::value; }

template<class T>
SCIA is_enum() -> bool { return std::is_enum<T>::value; }

template<class From, class To>
SCIA is_convertible() -> bool { return std::is_convertible<From, To>::value; }

template<class T>
SCIA is_enum_class() -> bool {return is_enum<T>() && !is_convertible<T, int>();}

namespace detail {
template<class T, class S, class... Args>
struct are_same {
    SCA value = is_same<T, S>() && are_same<T, Args...>::value;
};
template<class T, class S>
struct are_same<T, S> {
    SCA value = is_same<T, S>();
};

template<class T, class S, class... Args>
struct is_one_of {
    SCA value = is_same<T, S>() || is_one_of<T, Args...>::value;
};
template<class T, class S>
struct is_one_of<T, S> {
    SCA value = is_same<T, S>();
};
}

template<class T, class S, class... Args>
SCIA are_same() -> bool { return detail::are_same<T, S, Args...>::value; }

template<class T, class S, class... Args>
SCIA is_one_of() -> bool { return detail::is_one_of<T, S, Args...>::value; }

template<bool b, class T, class S>
using conditional_t = typename std::conditional<b, T, S>::type;

template<bool b, class T = void>
using enable_if_t = typename std::enable_if<b, T>::type;

template<class T, class U = void>
using enable_if_enum_class_t = enable_if_t<tmp::is_enum_class<T>(), U>;

template<class T, class S, class U = void>
using enable_if_same_t = enable_if_t<tmp::is_same<T, S>(), U>;

}

#endif // TYPE_TEST_HPP
