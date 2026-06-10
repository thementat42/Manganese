#ifndef MNSTL_EXT_NUM_CONFIG
#define MNSTL_EXT_NUM_CONFIG 1

// native indicates the compiler fully supports that 128-bit type

#include <cstdint>
#include <type_traits>

#if __cplusplus >= 202302L
#include <stdfloat>
#define MNSTL_HAS_STDFLOAT 1
#else
#define MNSTL_HAS_STDFLOAT 0
#endif


// This should be set by a build system (e.g. CMake)
#ifndef MNSTL_USE_COMPILER_EXTENDED_TYPES
#define MNSTL_USE_COMPILER_EXTENDED_TYPES 0
#endif

namespace mnstl {

#if MNSTL_USE_COMPILER_EXTENDED_TYPES == 1
#define MNSTL_NATIVE_I128 1
// use the compiler's 128-bit integer
using int128_t = __int128;
using uint128_t = unsigned __int128;
#else
#define MNSTL_NATIVE_I128 0
// fallback to a (slower) software implementation
template <bool IsSigned>
struct _basic_int128;
using int128_t = _basic_int128<true>;
using uint128_t = _basic_int128<false>;
#endif

// C++23 added (optional) support for fixed-width floating point numbers
// If possible, use those
// otherwise, fall back on the assumption that float is 32 bits and double is 64 bits (usually true on 64-bit systems)

#if defined(__STDCPP_FLOAT32_T__) && MNSTL_HAS_STDFLOAT
using float32_t = std::float32_t;
#else
using float32_t = float;
#endif  // defined(__STDCPP_FLOAT32_T__)

#if defined(__STDCPP_FLOAT64_T__) && MNSTL_HAS_STDFLOAT
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
concept SignedIntegral = detail::is_any_of<T, int8_t, int16_t, int32_t, int64_t, int128_t>;
template <class T>
concept UnsignedIntegral = detail::is_any_of<T, uint8_t, uint16_t, uint32_t, uint64_t, uint128_t>;
template <class T>
concept Integral = SignedIntegral<T> || UnsignedIntegral<T>;

template <class T>
concept Numeric = Integral<T> || FloatingPoint<T>;

// make_signed/make_unsigned implementation for these types

template <class T>
struct mnstl_make_signed {
    using type = std::make_signed_t<T>;
};

template <>
struct mnstl_make_signed<int128_t> {
    using type = int128_t;
};
template <>
struct mnstl_make_signed<const int128_t> {
    using type = const int128_t;
};
template <>
struct mnstl_make_signed<volatile int128_t> {
    using type = volatile int128_t;
};
template <>
struct mnstl_make_signed<const volatile int128_t> {
    using type = const volatile int128_t;
};

template <>
struct mnstl_make_signed<uint128_t> {
    using type = int128_t;
};
template <>
struct mnstl_make_signed<const uint128_t> {
    using type = const int128_t;
};
template <>
struct mnstl_make_signed<volatile uint128_t> {
    using type = volatile int128_t;
};
template <>
struct mnstl_make_signed<const volatile uint128_t> {
    using type = const volatile int128_t;
};

template <class T>
struct mnstl_make_unsigned {
    using type = std::make_unsigned_t<T>;
};

template <>
struct mnstl_make_unsigned<int128_t> {
    using type = uint128_t;
};
template <>
struct mnstl_make_unsigned<const int128_t> {
    using type = const uint128_t;
};
template <>
struct mnstl_make_unsigned<volatile int128_t> {
    using type = volatile uint128_t;
};
template <>
struct mnstl_make_unsigned<const volatile int128_t> {
    using type = const volatile uint128_t;
};

template <>
struct mnstl_make_unsigned<uint128_t> {
    using type = uint128_t;
};
template <>
struct mnstl_make_unsigned<const uint128_t> {
    using type = const uint128_t;
};
template <>
struct mnstl_make_unsigned<volatile uint128_t> {
    using type = volatile uint128_t;
};
template <>
struct mnstl_make_unsigned<const volatile uint128_t> {
    using type = const volatile uint128_t;
};

template <class T>
using mnstl_make_signed_t = typename mnstl_make_signed<std::remove_reference_t<T>>::type;
template <class T>
using mnstl_make_unsigned_t = typename mnstl_make_unsigned<std::remove_reference_t<T>>::type;

}  // namespace mnstl

#endif  // MNSTL_EXT_NUM_CONFIG