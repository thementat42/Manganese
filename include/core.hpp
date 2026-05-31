/**
 * @file core.hpp
 * @brief Global macros and utility definitions for the project.
 */

#ifndef MANGANESE_INCLUDE_GLOBAL_MACROS_HPP
#define MANGANESE_INCLUDE_GLOBAL_MACROS_HPP

// Some common includes that are used frequently

#include <stdint.h>
#include <stdio.h>
#include <string>

//~ Build type

#ifndef MN_DEBUG  // Defined by CMake (see CMakeLists.txt) -- if for some reason it doesn't exist, use NDEBUG as a fallback
#ifndef NDEBUG
#define MN_DEBUG 1
#else  //^^ ifndef NDEBUG vv ifdef NDEBUG
#define MN_DEBUG 0
#endif  // NDEBUG
#endif  // MN_DEBUG

/**
 * Indicates that a function will not throw in release builds, to enable more optimizations.
 * This is a no-op in debug builds, where more exceptions are expected.
 * Note: This macro should not be used on functions that will actually never throw (in those cases, use `noexcept`
 * directly).
 */

#if MN_DEBUG
#define NOEXCEPT_IF_RELEASE  // In debug mode, these can throw (e.g., using ASSERT_UNREACHABLE)
#else  // ^^ MN_DEBUG vv !MN_DEBUG
#define NOEXCEPT_IF_RELEASE noexcept  // In release builds, optimize these functions more
#endif

#ifndef DISCARD
#define DISCARD(value) (void)(value)  // Explicitly discard a value
#endif  // DISCARD

//~ Force inline
#if defined(__clang__) || defined(__GNUC__)
#define FORCE_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#define FORCE_INLINE __forceinline
#else
// No known force inline, fallback to `inline` keyword
#define FORCE_INLINE inline
#endif

[[noreturn]] inline void manganese_unreachable() {
#if __cplusplus >= 202302L
    std::unreachable();  // compiler agnostic, but still allows for optimisation
// If < C++23, use a compiler-specific implementation
#elif defined(__GNUC__) || defined(__clang__) || (defined(__has_builtin) && __has_builtin(__builtin_unreachable))
    __builtin_unreachable();
#elif defined(_MSC_VER)
    __assume(false);
#else
    // still invokes UB
#endif  // __cplusplus >= 202302L
}

inline void panic(const char* message = "", const char* file = "", size_t line = 0,
                               const char* func = "") {
    fprintf(stderr, "\033[31mUnreachable code reached: %s \nIn file %s at line %zu when running %s\033[0m\n", message,
            file, line, func);
    throw(message);
}

FORCE_INLINE void panic(const std::string& message, const char* file, size_t line, const char* func) {
    panic(message.c_str(), file, line, func);
}

#if MN_DEBUG
#define ASSERT_UNREACHABLE(message) \
    panic((message), __FILE__, __LINE__, __func__)
#else
#define ASSERT_UNREACHABLE(message) manganese_unreachable()
#endif  // MN_DEBUG

#endif  // MANGANESE_INCLUDE_GLOBAL_MACROS_HPP