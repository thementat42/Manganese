/**
 * @file compiler_config.h
 * @brief This file contains configuration settings for the compiler, such as enabling/disabling warnings,
 * and other compiler-specific settings.
 * This file is included in the global_macros.hpp file for convenience.
 */

#ifndef MANGANESE_INCLUDE_UTILS_COMPILER_CONFIG_H
#define MANGANESE_INCLUDE_UTILS_COMPILER_CONFIG_H

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

#if __cplusplus >= 202302L
#define __MANGANESE_UNREACHABLE std::unreachable();  // compiler agnostic, but still allows for optimisation
// If < C++23, use a compiler-specific implementation
#elif defined(__GNUC__) || defined(__clang__)
#define __MANGANESE_UNREACHABLE __builtin_unreachable();
#elif defined(_MSC_VER)
#define __MANGANESE_UNREACHABLE __assume(false);
#else  // If no (known) compiler-specific implementation is available, fall back to nothing
#define __MANGANESE_UNREACHABLE
#endif  // __cplusplus >= 202302L

/**
 * In pre-C++23 builds, uses a compile-specific unreachable code marker, if available.
 * MSVC, Clang and GCC are supported.
 * In C++23+, uses `std::unreachable()`
 */
#define COMPILER_UNREACHABLE __MANGANESE_UNREACHABLE

#endif  // MANGANESE_INCLUDE_UTILS_COMPILER_CONFIG_H