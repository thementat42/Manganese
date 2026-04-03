#ifndef MNSTL_EXT_NUM_CONFIG
#define MNSTL_EXT_NUM_CONFIG 1

#include <stdint.h>
#include <type_traits>

#if __cplusplus >= 202302L
#include <stdfloat>
#define HAS_STDFLOAT 1
#else
#define HAS_STDFLOAT 0
#endif

// native indicates the compiler fully supports that 128-bit type

namespace mnstl {
#if MNSTL_USE_COMPILER_INT128
#define MNSTL_NATIVE_I128 1
// use the compiler's 128-bit integer
typedef __int128 int128_t;
typedef unsigned __int128 uint128_t;
#else
// fallback to a (slower) software implementation
template <bool IsSigned>
struct _basic_int128;
typedef _basic_int128<true> int128_t;
typedef _basic_int128<false> uint128_t;
#define MNSTL_NATIVE_I128 0
#endif

// C++23 added (optional) support for fixed-width floating point numbers
// If possible, use those
// otherwise, fall back on the assumption that float is 32 bits and double is 64 bits (usually true on 64-bit systems)

#if defined(__STDCPP_FLOAT32_T__) && HAS_STDFLOAT
typedef std::float32_t float32_t;
#else
typedef float float32_t;
#endif  // defined(__STDCPP_FLOAT32_T__)

#if defined(__STDCPP_FLOAT64_T__) && HAS_STDFLOAT
typedef std::float64_t float64_t;
#else
typedef double float64_t;
#endif  // defined(__STDCPP_FLOAT64_T__)

namespace detail {
template <class T, class... Types>
constexpr inline bool is_one_of = (std::is_same_v<std::remove_cvref_t<T>, Types> || ...);

}  // namespace detail

template <class T>
concept FloatingPoint = detail::is_one_of<T, float32_t, float64_t, long double>;
template <class T>
concept SignedIntegral = detail::is_one_of<T, int8_t, int16_t, int32_t, int64_t, int128_t>;

template <class T>
concept UnsignedIntegral = detail::is_one_of<T, uint8_t, uint16_t, uint32_t, uint64_t, uint128_t>;
template <class T>
concept Integral = SignedIntegral<T> || UnsignedIntegral<T>;
template <class T>
concept Numeric = Integral<T> || FloatingPoint<T>;

}  // namespace mnstl

#endif  // MNSTL_EXT_NUM_CONFIG