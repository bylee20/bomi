#ifndef TMP_HPP
#define TMP_HPP

#include <type_traits>
#include <cstdint>

namespace Tmp { // simple template meta progamming
	template <typename T1, typename T2> constexpr bool isSame() { return std::is_same<T1, T2>::value; }
	template <typename T> constexpr bool isInteger() { return std::is_integral<T>::value; }

	// ok ? T1 : T2
	template <bool ok, typename T1, typename T2> struct Ternary {};
	template <typename T1, typename T2> struct Ternary<true, T1, T2> { typedef T1 ans; };
	template <typename T1, typename T2> struct Ternary<false, T1, T2> { typedef T2 ans; };

	template <int N> constexpr int log2() { static_assert(N != 0, "wrong argument for log2"); return log2<N/2>() + 1; }
	template <> constexpr int log2<1>() { return 0; }
	template <typename T> constexpr int log2bitsof() { return log2<sizeof(T)*8>(); }

	template <int bits, bool sign> struct IntegerType {};
	template <> struct IntegerType<16, true>  { typedef std::int16_t  ans; };
	template <> struct IntegerType<32, true>  { typedef std::int32_t  ans; };
	template <> struct IntegerType<64, true>  { typedef std::int64_t  ans; };
	template <> struct IntegerType<16, false> { typedef std::uint16_t ans; };
	template <> struct IntegerType<32, false> { typedef std::uint32_t ans; };
	template <> struct IntegerType<64, false> { typedef std::uint64_t ans; };

	template <int bits> struct FloatType {};
	template <> struct FloatType<32> { typedef float  ans; };
	template <> struct FloatType<64> { typedef double ans; };
}


#endif // TMP_HPP
