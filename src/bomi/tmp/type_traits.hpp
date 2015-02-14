#ifndef TYPE_TEST_HPP
#define TYPE_TEST_HPP

#include <type_traits>
#include "global.hpp"

namespace tmp {

namespace detail {
template<typename T, typename = void>
struct is_callable : std::is_function<T> { };

template<typename T>
struct is_callable<T, typename std::enable_if<
    std::is_same<decltype(void(&T::operator())), void>::value
    >::type> : std::true_type { };

template <typename T>
struct func_traits
    : public func_traits<decltype(&T::operator())>
{};

template <typename T, typename R, typename... Args>
struct func_traits<R(T::*)(Args...) const> {
    SCA args = sizeof...(Args);
    using result_type = R;
    template <size_t n> struct arg
    { using type = typename std::tuple_element<n, std::tuple<Args...>>::type; };
};

}

template<class T, int n>
using func_arg_t = typename detail::func_traits<T>::template arg<n>::type;

template<class T>
SCIA func_args() { return detail::func_traits<T>::args; }

template<class T>
SCIA is_callable() -> bool { return detail::is_callable<T>::value; }

template<class T>
SCIA is_integral() -> bool { return std::is_integral<T>::value; }

template<class T>
SCIA is_floating_point() -> bool { return std::is_floating_point<T>::value; }

template<class T>
SCIA is_arithmetic() -> bool { return std::is_arithmetic<T>::value; }

template<class T>
SCIA is_class() -> bool { return std::is_class<T>::value; }

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

template<bool b, class T = void>
using enable_unless_t = typename std::enable_if<!b, T>::type;

template<class T, class U = void>
using enable_if_enum_class_t = enable_if_t<tmp::is_enum_class<T>(), U>;

template<class T, class S, class U = void>
using enable_if_same_t = enable_if_t<tmp::is_same<T, S>(), U>;

template<class T, class S, class U = void>
using enable_unless_same_t = enable_if_t<!tmp::is_same<T, S>(), U>;

template<class T, class U = void>
using enable_if_arithmetic_t = enable_if_t<tmp::is_arithmetic<T>(), U>;

template<class T, class U = void>
using enable_unless_arithmetic_t = enable_if_t<!tmp::is_arithmetic<T>(), U>;

template<class T, class U = void>
using enable_if_callable_t = enable_if_t<tmp::is_callable<T>(), U>;

template<class T, class U = void>
using enable_unless_callable_t = enable_if_t<!tmp::is_callable<T>(), U>;

template<class T>
using remove_const_t = typename std::remove_const<T>::type;

template<class T>
using remove_ref_t = typename std::remove_reference<T>::type;

template<class T>
using remove_ptr_t = typename std::remove_pointer<T>::type;

template<class T>
using remove_cref_t = remove_const_t<remove_ref_t<T>>;

template<class T, class U = remove_cref_t<T>>
using cval_t = conditional_t<is_class<U>(), const U&, U>;

template <int bits, bool sign> struct integer { /*using type = char;*/ };
template <> struct integer<16, true>  { using type = std::int16_t ; };
template <> struct integer<32, true>  { using type = std::int32_t ; };
template <> struct integer<64, true>  { using type = std::int64_t ; };
template <> struct integer<16, false> { using type = std::uint16_t; };
template <> struct integer<32, false> { using type = std::uint32_t; };
template <> struct integer<64, false> { using type = std::uint64_t; };

template<int bits, bool sign>
using integer_t = typename integer<bits, sign>::type;

template<class T>
using unsigned_t = integer_t<sizeof(T)*8, false>;

template <int bits> struct floating_point { using type = char; };
template <> struct floating_point<32> { using type = float ; };
template <> struct floating_point<64> { using type = double; };

template<int bits>
using floating_point_t = typename floating_point<bits>::type;

}

#endif // TYPE_TEST_HPP
