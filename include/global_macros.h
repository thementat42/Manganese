/**
 * @file global_macros.h
 * @brief Global macros and utility definitions for the project.
 */

#ifndef MANGANESE_INCLUDE_GLOBAL_MACROS_H
#define MANGANESE_INCLUDE_GLOBAL_MACROS_H

#include <stdint.h>
#include <utils/compiler_config.h>

#include <iostream>
#include <stdexcept>

#ifndef DEBUG  // Defined by CMake (see CMakeLists.txt) -- if for some reason it doesn't exist, use NDEBUG as a fallback
#ifndef NDEBUG
#define DEBUG 1
#else  //^^ ifndef NDEBUG vv ifdef NDEBUG
#define DEBUG 0
#endif  // NDEBUG
#endif  // DEBUG

#define __PRINT_LOCATION \
    std::cerr << "\033[33m In file: " << __FILE__ << ", at line " << __LINE__ << ": when running " << __func__ << "\033[0m\n";

#define PRINT_LOCATION __PRINT_LOCATION  // Print the location of the log message (in the compiler source, not the user code)

#if __cplusplus >= 202302L
#include <utility>
#define __COMPILER_UNREACHABLE std::unreachable();  // compiler agnostic, but still allows for optimisation
// If < C++23, use a compiler-specific implementation
#elif defined(__GNUC__) || defined(__clang__)
#define __COMPILER_UNREACHABLE __builtin_unreachable();
#elif defined(_MSC_VER)
#define __COMPILER_UNREACHABLE __assume(false);
#else  // If no compiler-specific implementation is available, do nothing
#define __COMPILER_UNREACHABLE
#endif  // __cplusplus >= 202302L

/**
 * In pre-C++23 builds, uses a compile-specific unreachable code marker, if available.
 * MSVC, Clang and GCC are supported.
 * In C++23+, uses `std::unreachable()`
 */
#define COMPILER_UNREACHABLE __COMPILER_UNREACHABLE

#if DEBUG
#define __UNREACHABLE(message)                                                       \
    do {                                                                             \
        std::cerr << "\033[31mUnreachable code reached: " << message << "\n\033[0m"; \
        PRINT_LOCATION;                                                              \
        throw std::runtime_error(message);                                           \
    } while (0);
#else  // ^^ DEBUG vv !DEBUG
#define __UNREACHABLE(message) COMPILER_UNREACHABLE
#endif  // DEBUG

#define ASSERT_UNREACHABLE(message) __UNREACHABLE(message)  // Condition that should never be reached

#if DEBUG
#define __noexcept_if_release           // In debug mode, these functions are more likely to throw -- don't use noexcept
#else                                   // ^^ DEBUG vv !DEBUG
#define __noexcept_if_release noexcept  // In release builds, optimize these functions more
#endif

/**
 * Indicates that a function will not throw in release builds, to enable more optimizations.
 * This is a no-op in debug builds, where more exceptions are expected.
 * Note: This macro should not be used on functions that will actually never throw (in those cases, use `noexcept` directly).
 */
#define noexcept_if_release __noexcept_if_release

#define DISCARD(value) (void)(value)  // Explicitly discard a value (useful for [[noexcept]] functions)

#endif  // MANGANESE_INCLUDE_GLOBAL_MACROS_H