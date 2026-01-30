/**
 * @file global_macros.hpp
 * @brief Global macros and utility definitions for the project.
 */

#ifndef MANGANESE_INCLUDE_GLOBAL_MACROS_HPP
#define MANGANESE_INCLUDE_GLOBAL_MACROS_HPP

#include <stdint.h>

#include <iostream>
#include <utility>

// Some common includes that are used frequently

/**
 * Indicates that a function will not throw in release builds, to enable more optimizations.
 * This is a no-op in debug builds, where more exceptions are expected.
 * Note: This macro should not be used on functions that will actually never throw (in those cases, use `noexcept`
 * directly).
 */

#if DEBUG
#define NOEXCEPT_IF_RELEASE  // In debug mode, these functions are more likely to throw -- don't use noexcept
#else  // ^^ DEBUG vv !DEBUG
#define NOEXCEPT_IF_RELEASE noexcept  // In release builds, optimize these functions more
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

[[noreturn]] inline void manganese_unreachable(const char* message = "", const char* file = "", unsigned line = 0,
                                               const char* func = "") NOEXCEPT_IF_RELEASE {
#if DEBUG
    std::cerr << "\033[31mUnreachable code reached: " << message << "\n\033[0m";
    std::cerr << "\033[33m In file: " << file << ", at line " << line << ": when running " << func << "\033[0m\n";
    throw(message);
#else
#if __cplusplus >= 202302L
    std::unreachable();  // compiler agnostic, but still allows for optimisation
// If < C++23, use a compiler-specific implementation
#elif defined(__GNUC__) || defined(__clang__)
    __builtin_unreachable();
#elif defined(_MSC_VER)
    __assume(false);
#else
    // still invokes UB
#endif  // __cplusplus >= 202302L
#endif  // DEBUG
}

[[noreturn]] inline void manganese_unreachable(const std::string& message, const char* file, unsigned line,
                                               const char* func) {
    manganese_unreachable(message.c_str(), file, line, func);
}

#define ASSERT_UNREACHABLE(message) manganese_unreachable((message), __FILE__, __LINE__, __func__)

#endif  // MANGANESE_INCLUDE_GLOBAL_MACROS_HPP