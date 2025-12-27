/**
 * @file global_macros.hpp
 * @brief Global macros and utility definitions for the project.
 */

#ifndef MANGANESE_INCLUDE_GLOBAL_MACROS_HPP
#define MANGANESE_INCLUDE_GLOBAL_MACROS_HPP

#include <stdint.h>
#include <utils/compiler_config.h>

#include <utils/debug_utils.hpp>

// Some common includes that are used frequently

/**
 * Indicates that a function will not throw in release builds, to enable more optimizations.
 * This is a no-op in debug builds, where more exceptions are expected.
 * Note: This macro should not be used on functions that will actually never throw (in those cases, use `noexcept`
 * directly).
 */

#if DEBUG
#define noexcept_if_release  // In debug mode, these functions are more likely to throw -- don't use noexcept
#else  // ^^ DEBUG vv !DEBUG
#define noexcept_if_release noexcept  // In release builds, optimize these functions more
#endif

#ifndef DISCARD
#define DISCARD(value) (void)(value)  // Explicitly discard a value
#endif  // DISCARD

//~ Build type

#ifndef DEBUG  // Defined by CMake (see CMakeLists.txt) -- if for some reason it doesn't exist, use NDEBUG as a fallback
#ifndef NDEBUG
#define DEBUG 1
#else  //^^ ifndef NDEBUG vv ifdef NDEBUG
#define DEBUG 0
#endif  // NDEBUG
#endif  // DEBUG

//~ Force inline
#if defined(__clang__) || defined(__GNUC__)
#define FORCE_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#define FORCE_INLINE __forceinline
#else
// No known force inline, fallback to `inline` keyword
#define FORCE_INLINE inline
#endif

//~ Unreachable Code

[[noreturn]] inline void manganese_unreachable() {
#if __cplusplus >= 202302L
    std::unreachable();  // compiler agnostic, but still allows for optimisation
// If < C++23, use a compiler-specific implementation
#elif defined(__GNUC__) || defined(__clang__)
    __builtin_unreachable();
#elif defined(_MSC_VER)
    __assume(false);
#else  // If no (known) compiler-specific implementation is available, fall back to nothing
       // Still invoked undefined behaviour
#endif  // __cplusplus >= 202302L
}

#endif  // MANGANESE_INCLUDE_GLOBAL_MACROS_HPP