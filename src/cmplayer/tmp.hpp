#ifndef TMP_HPP
#define TMP_HPP

#include <type_traits>
#include <cstdint>

namespace tmp { // simple template meta progamming

template <int N> constexpr int log2() { static_assert(N != 0, "wrong argument for log2"); return log2<N/2>() + 1; }
template <> constexpr int log2<1>() { return 0; }
template <typename T> constexpr int log2bitsof() { return log2<sizeof(T)*8>(); }

template <int bits, bool sign> struct integer { /*typedef char type;*/ };
template <> struct integer<16, true>  { typedef std::int16_t  type; };
template <> struct integer<32, true>  { typedef std::int32_t  type; };
template <> struct integer<64, true>  { typedef std::int64_t  type; };
template <> struct integer<16, false> { typedef std::uint16_t type; };
template <> struct integer<32, false> { typedef std::uint32_t type; };
template <> struct integer<64, false> { typedef std::uint64_t type; };

template <int bits> struct floating_point { typedef char type; };
template <> struct floating_point<32> { typedef float  type; };
template <> struct floating_point<64> { typedef double type; };

template<class... Args> static inline void pass(const Args &...) { }

template<int idx, int size, bool go = idx < size>
struct static_for { };

template<int idx, int size>
struct static_for<idx, size, true> {
	template<class... Args, class F>
	static inline void run(const std::tuple<Args...> &tuple, const F &func) {
		func(std::get<idx>(tuple));
		static_for<idx+1, size>::run(tuple, func);
	}
};

template<int idx, int size>
struct static_for<idx, size, false> {
	template<class T, class F>
	static inline void run(const T &, const F &) { }
	template<class T, class F>
	static inline void run(T &&, const F &) { }
};

template<int... S>                  struct index_list { };

namespace detail {

template<int diff, int N, int... S>
struct index_list_generate_interval : public index_list_generate_interval<diff, N-diff, N-diff, S...> { };

template<int diff, int ...S>
struct index_list_generate_interval<diff, 0, S...> { typedef index_list<S...> type; };

template<class F, class... Args, int... I>
static inline void call_with_tuple_impl(F &&func, std::tuple<Args...> &&tuple, index_list<I...>) {
	func(std::get<I>(tuple)...);
}

template<class F, class... Args, int... I>
static inline void call_with_tuple_impl(F &&func, std::tuple<Args...> &tuple, index_list<I...>) {
	func(std::get<I>(tuple)...);
}

template<class F, class... Args, int... I>
static inline void call_with_tuple_impl(F &&func, const std::tuple<Args...> &tuple, index_list<I...>) {
	func(std::get<I>(tuple)...);
}

}

template<int N, int interval = 1>
static inline auto make_tuple_index()
		-> decltype(typename detail::index_list_generate_interval<interval, N%interval == 0 ? N : (N/interval+1)*interval>::type()) {
	return typename detail::index_list_generate_interval<interval, N%interval == 0 ? N : (N/interval+1)*interval>::type();
}

template<int interval = 1, class... Args>
static inline auto make_tuple_index(const std::tuple<Args...> &) -> decltype(make_tuple_index<sizeof...(Args), interval>()) {
	return make_tuple_index<sizeof...(Args), interval>();
}

template<class... Args, int... I>
static inline auto extract_tuple(const std::tuple<Args...> &tuple, index_list<I...>) -> decltype(std::tie(std::get<I>(tuple)...)) {
	return std::tie(std::get<I>(tuple)...);
}

template<class F, class... Args, int... I>
static inline void call_with_tuple(const F &func, std::tuple<Args...> &&tuple, index_list<I...> index) {
	detail::call_with_tuple_impl(func, tuple, index);
}

template<class F, class... Args, int... I>
static inline void call_with_tuple(const F &func, std::tuple<Args...> &tuple, index_list<I...> index) {
	detail::call_with_tuple_impl(func, tuple, index);
}

template<class F, class... Args, int... I>
static inline void call_with_tuple(const F &func, const std::tuple<Args...> &tuple, index_list<I...> index) {
	detail::call_with_tuple_impl(func, tuple, index);
}

template<class F, class... Args>
static inline void call_with_tuple(const F &func, std::tuple<Args...> &&tuple) {
	detail::call_with_tuple_impl(func, tuple, make_tuple_index(tuple));
}

template<class F, class... Args>
static inline void call_with_tuple(const F &func, std::tuple<Args...> &tuple) {
	detail::call_with_tuple_impl(func, tuple, make_tuple_index(tuple));
}

template<class F, class... Args>
static inline void call_with_tuple(const F &func, const std::tuple<Args...> &tuple) {
	detail::call_with_tuple_impl(func, tuple, make_tuple_index(tuple));
}

}


#endif // TMP_HPP
