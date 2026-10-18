#ifndef MANGANESE_INCLUDE_UTILS_EXT_NUM_CONFIG
#define MANGANESE_INCLUDE_UTILS_EXT_NUM_CONFIG 1

// native indicates the compiler fully supports that 128-bit type

#include <cstdint>
#include <type_traits>

#if __cplusplus >= 202302L
#include <stdfloat>
#define MN_HAS_STDFLOAT 1
#else
#define MN_HAS_STDFLOAT 0
#endif

// This should be set by a build system (e.g. CMake)
#ifndef MN_USE_COMPILER_EXTENDED_TYPES
#define MN_USE_COMPILER_EXTENDED_TYPES 0
#endif

namespace Manganese::utils {

#if MN_USE_COMPILER_EXTENDED_TYPES == 1
#define MN_NATIVE_I128 1
// use the compiler's 128-bit integer
using int128_t = __int128;
using uint128_t = unsigned __int128;
#else
#define MN_NATIVE_I128 0
// fallback to a (slower) software implementation
template <bool IsSigned>
struct _basic_int128;
using int128_t = _basic_int128<true>;
using uint128_t = _basic_int128<false>;
#endif

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

// #if !MN_USE_COMPILER_EXTENDED_TYPES
// namespace std {
// // Native integer + (u)int128
// template <class T>
//     requires(Manganese::utils::Integral<T> && !std::is_same_v<std::remove_cvref_t<T>, Manganese::utils::int128_t>)
// struct common_type<Manganese::utils::int128_t, T> {
//     using type = Manganese::utils::int128_t;
// };
// template <class T>
//     requires(Manganese::utils::Integral<T> && !std::is_same_v<std::remove_cvref_t<T>, Manganese::utils::int128_t>)
// struct common_type<T, Manganese::utils::int128_t> {
//     using type = Manganese::utils::int128_t;
// };

// template <class T>
//     requires(Manganese::utils::Integral<T> && (!std::is_same_v<std::remove_cvref_t<T>, Manganese::utils::uint128_t>))
// struct common_type<Manganese::utils::uint128_t, T> {
//     // If mixed with a signed integer type, promote to signed int128_t to preserve sign correctness.
//     // Otherwise, keep it as an unsigned uint128_t.
//     using type = std::conditional_t<Manganese::utils::SignedIntegral<T>, Manganese::utils::int128_t,
//                                     Manganese::utils::uint128_t>;
// };

// template <class T>
//     requires(Manganese::utils::Integral<T> && (!std::is_same_v<std::remove_cvref_t<T>, Manganese::utils::uint128_t>))
// struct common_type<T, Manganese::utils::uint128_t> {
//     using type = std::conditional_t<Manganese::utils::SignedIntegral<T>, Manganese::utils::int128_t,
//                                     Manganese::utils::uint128_t>;
// };

// // Native float + (u)int128
// template <class T>
//     requires(Manganese::utils::FloatingPoint<T>)
// struct common_type<T, Manganese::utils::int128_t> {
//     using type = Manganese::utils::float64_t;
// };
// template <class T>
//     requires(Manganese::utils::FloatingPoint<T>)
// struct common_type<Manganese::utils::int128_t, T> {
//     using type = Manganese::utils::float64_t;
// };

// template <class T>
//     requires(Manganese::utils::FloatingPoint<T>)
// struct common_type<T, Manganese::utils::uint128_t> {
//     using type = Manganese::utils::float64_t;
// };
// template <class T>
//     requires(Manganese::utils::FloatingPoint<T>)
// struct common_type<Manganese::utils::uint128_t, T> {
//     using type = Manganese::utils::float64_t;
// };

// template <>
// struct common_type<Manganese::utils::int128_t, Manganese::utils::uint128_t> {
//     using type = Manganese::utils::int128_t;
// };

// template <>
// struct common_type<Manganese::utils::uint128_t, Manganese::utils::int128_t> {
//     using type = Manganese::utils::int128_t;
// };

// // int128 and uint128

// }  // namespace std
// #endif  // !MN_USE_COMPILER_EXTENDED_TYPES

#endif  // MANGANESE_INCLUDE_UTILS_EXT_NUM_CONFIG