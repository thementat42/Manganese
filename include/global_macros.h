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

#ifndef DEBUG    // Defined by CMake (see CMakeLists.txt) -- if for some reason it doesn't exist, default to 0
#define DEBUG 0  // Whether to enable debugging features
#endif           // DEBUG

#define PRINT_LOCATION_HELPER \
    std::cerr << "\033[33m In file: " << __FILE__ << ", at line " << __LINE__ << ": when running " << __func__ << "\033[0m\n";

#define PRINT_LOCATION PRINT_LOCATION_HELPER  // Print the location of the log message (in the compiler source, not the user code)

#if __cplusplus >= 202302L
#include <utility>
#define COMPILER_UNREACHABLE std::unreachable();  // compiler agnostic, but still allows for optimisation
// If < C++23, use a compiler-specific implementation
#elif defined(__GNUC__) || defined(__clang__)
#define COMPILER_UNREACHABLE __builtin_unreachable();
#elif defined(_MSC_VER)
#define COMPILER_UNREACHABLE __assume(false);
#else  // If no compiler-specific implementation is available, do nothing
#define COMPILER_UNREACHABLE
#endif  // __cplusplus >= 202302L

#define __UNREACHABLE(message)                                                       \
    do {                                                                             \
        std::cerr << "\033[31mUnreachable code reached: " << message << "\n\033[0m"; \
        PRINT_LOCATION;                                                              \
        COMPILER_UNREACHABLE;                                                        \
        throw std::runtime_error(message);                                           \
    } while (0);

#define ASSERT_UNREACHABLE(message) __UNREACHABLE(message)  // Condition that should never be reached

/**
 * Indicates that a function is intended to be noexcept,
 * except for a default case that should logically never happen
 * If that case happens, something has gone horribly wrong, and the function will throw an exception
 *
 * This serves as a two way check -- functions marked with this should have a fallback case that throws -- if it doesn't add one
 * If a function has a fallback throw case but not this macro, add it
 *
 * NOTE: This should only be used on functions that throw on catastrophic failures (e.g. use the ASSERT_UNREACHABLE macro from the logging library). It should not be used on functions that just never throw (those should be marked noexcept)
 */
#if DEBUG
#define noexcept_debug  // In debug mode, these functions are more likely to throw -- don't use noexcept
#else // ^^ DEBUG vv !DEBUG
#define noexcept_debug noexcept  // In release builds, optimize these functions more
#endif

#define DISCARD(value) (void)(value)  // Explicitly discard a value (useful for [[noexcept]] functions)

#endif  // MANGANESE_INCLUDE_GLOBAL_MACROS_H