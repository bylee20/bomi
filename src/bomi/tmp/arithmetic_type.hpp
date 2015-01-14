#ifndef ARITHMETIC_TYPE_HPP
#define ARITHMETIC_TYPE_HPP

namespace tmp {

template <int bits, bool sign> struct integer { /*using type = char;*/ };
template <> struct integer<16, true>  { using type = std::int16_t ; };
template <> struct integer<32, true>  { using type = std::int32_t ; };
template <> struct integer<64, true>  { using type = std::int64_t ; };
template <> struct integer<16, false> { using type = std::uint16_t; };
template <> struct integer<32, false> { using type = std::uint32_t; };
template <> struct integer<64, false> { using type = std::uint64_t; };

template<int bits, bool sign>
using integer_t = typename integer<bits, sign>::type;

template <int bits> struct floating_point { using type = char; };
template <> struct floating_point<32> { using type = float ; };
template <> struct floating_point<64> { using type = double; };

template<int bits>
using floating_point_t = typename floating_point<bits>::type;

}

#endif // ARITHMETIC_TYPE_HPP
