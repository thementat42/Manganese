#ifndef MANGANESE_INCLUDE_UTILS_NUMBER_TYPES
#define MANGANESE_INCLUDE_UTILS_NUMBER_TYPES 1

// native indicates the compiler fully supports that 128-bit type

#include <cstdint>
#include <type_traits>

#if __cplusplus >= 202302L
#include <stdfloat>
#define MN_HAS_STDFLOAT 1
#else
#define MN_HAS_STDFLOAT 0
#endif  // __cplusplus >= 202302L

#ifndef __SIZEOF_INT128__
#error Manganese requires a compiler with support for __int128
#endif  //  __SIZEOF_INT128__

namespace Manganese::utils {

using int128_t = __int128;
using uint128_t = unsigned __int128;

// C++23 added (optional) support for fixed-width floating point numbers
// If possible, use those
// otherwise, fall back on the assumption that float is 32 bits and double is 64 bits (usually true on 64-bit systems)

#if defined(__STDCPP_FLOAT32_T__) && MN_HAS_STDFLOAT
using float32_t = std::float32_t;
#else
using float32_t = float;
#endif  // defined(__STDCPP_FLOAT32_T__)

#if defined(__STDCPP_FLOAT64_T__) && MN_HAS_STDFLOAT
using float64_t = std::float64_t;
#else
using float64_t = double;
#endif  // defined(__STDCPP_FLOAT64_T__)

namespace detail {
template <class T, class... Types>
constexpr inline bool is_any_of = (std::is_same_v<std::remove_cvref_t<T>, Types> || ...);

}  // namespace detail

// Manually define these traits to avoid any hardware-based weirdness
// e.g. long double is 80 bits on x86_64
template <class T>
concept FloatingPoint = detail::is_any_of<T, float32_t, float64_t>;

template <class T>
concept SignedIntegral = detail::is_any_of<T, std::int8_t, std::int16_t, std::int32_t, std::int64_t, int128_t>;
template <class T>
concept UnsignedIntegral = detail::is_any_of<T, std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t, uint128_t>;
template <class T>
concept Integral = SignedIntegral<T> || UnsignedIntegral<T>;

template <class T>
concept Numeric = Integral<T> || FloatingPoint<T>;

}  // namespace Manganese::utils

#endif  // MANGANESE_INCLUDE_UTILS_NUMBER_TYPES