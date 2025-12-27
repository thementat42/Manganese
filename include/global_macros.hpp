/**
 * @file global_macros.hpp
 * @brief Global macros and utility definitions for the project.
 */

#ifndef MANGANESE_INCLUDE_GLOBAL_MACROS_HPP
#define MANGANESE_INCLUDE_GLOBAL_MACROS_HPP

#include <stdint.h>
#include <iostream>
#include <stdexcept>
#include <utility>

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

#define __MANGANESE_PRINT_LOCATION                                                                             \
    std::cerr << "\033[33m In file: " << __FILE__ << ", at line " << __LINE__ << ": when running " << __func__ \
              << "\033[0m\n";

#define PRINT_LOCATION \
    __MANGANESE_PRINT_LOCATION  // Print the location of the log message (in the compiler source, not the user code)

#if DEBUG
#define __MANGANESE_ASSERT_UNREACHABLE(message)                                      \
    do {                                                                             \
        std::cerr << "\033[31mUnreachable code reached: " << message << "\n\033[0m"; \
        PRINT_LOCATION;                                                              \
        throw std::runtime_error(message);                                           \
    } while (0);
#else  // ^^ DEBUG vv !DEBUG
#define __MANGANESE_ASSERT_UNREACHABLE(message) manganese_unreachable();
#endif  // DEBUG

/**
 * Indicates a place that should never be reached
 * In debug mode, throws an std::runtime_error
 * In release mode, marks the block as unreachable, for optimizations
 * @note see
 */
#define ASSERT_UNREACHABLE(message) __MANGANESE_ASSERT_UNREACHABLE(message)

#if DEBUG
#define __MANGANESE_NOT_IMPLEMENTED(reason)                                                                              \
    do {                                                                                                                 \
        std::cerr << "\033[31m" << __func__ << "is not implemented yet!";                                                \
        std::cerr << "Reason: " << reason << "\n";                                                                       \
        std::cerr                                                                                                        \
            << " (if this occurred in a test run, make sure to implement the function, then re-run the tests)\n\033[0m"; \
        PRINT_LOCATION;                                                                                                  \
        throw std::runtime_error("Not implemented yet");                                                                 \
    } while (0);
#else  // ^^ DEBUG vv !DEBUG
#define __MANGANESE_NOT_IMPLEMENTED(reason)
#endif  // DEBUG

#define NOT_IMPLEMENTED(reason) __MANGANESE_NOT_IMPLEMENTED(reason)  // Indicates that a function is not implemented

#endif  // MANGANESE_INCLUDE_GLOBAL_MACROS_HPP