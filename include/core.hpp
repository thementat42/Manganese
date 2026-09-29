#ifndef MANGANESE_INCLUDE_CORE_HPP
#define MANGANESE_INCLUDE_CORE_HPP

// Some common includes that are used frequently

#include <cstddef>
#include <cstdio>
#include <string>
#include <utility>  // C++ 23 for unreachable() and its associated feature macro

//~ Build type

#ifndef MN_DEBUG  // Defined by CMake (see CMakeLists.txt) -- if for some reason it doesn't exist, use NDEBUG as a
                  // fallback
#ifndef NDEBUG
#define MN_DEBUG 1
#else  //^^ ifndef NDEBUG vv ifdef NDEBUG
#define MN_DEBUG 0
#endif  // NDEBUG
#endif  // MN_DEBUG

/**
 * Indicates that a function will not throw in release builds.
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
#define DISCARD(value) (static_cast<void>((value)))  // Explicitly discard a value
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

[[noreturn]] inline void manganese_unreachable(int /*unused*/) {
    *(static_cast<volatile int*>(nullptr)) = 0;  // still invokes UB (and should crash on most platforms)
    for (;;) {}  // mainly to satisfy the compiler around not returning
}

[[noreturn]] inline void manganese_unreachable(float /*unused*/) {
#ifdef __has_builtin
#if __has_builtin(__builtin_unreachable)
    __builtin_unreachable();
#elif __has_builtin(__builtin_trap)
    __builtin_trap();  // unreachable code shouldn't be hit anyways
#else  // ^^ some kind of builtin vv no builtin
    manganese_unreachable(42);
#endif  // __has_builtin(__builtin_unreachable)
#else  // ^^ builtins detectable vv no way to detect builtins, fallback
    manganese_unreachable(0);
#endif  // __has_builtin
}

[[noreturn]] inline void manganese_unreachable() {
#ifdef __cpp_lib_unreachable
    std::unreachable();
#elif defined(_MSC_VER)
    __assume(false);
#else  // ^^ MSVC vv not MSVC
    manganese_unreachable(42.0F);
#endif  // __cpp_lib_unreachable
}

[[noreturn]] inline void panic(const char* message, const char* file, std::size_t line, const char* func) {
    std::fprintf(stderr, "\033[31mPanic invoked: %s \nIn file %s at line %zu when running %s\033[0m\n", message, file,
                 line, func);
    throw message;
}

[[noreturn]] FORCE_INLINE void panic(const std::string& message, const char* file, std::size_t line, const char* func) {
    panic(message.c_str(), file, line, func);
}

#define BETWEEN(c, low, high) (((low) <= (c)) && ((c) <= (high)))

#ifndef MN_STRINGIFY
#define MN_STRINGIFY(x) #x
#endif  // MN_STRINGIFY

#if MN_DEBUG
#define ASSERT_UNREACHABLE(message)          panic((message), __FILE__, __LINE__, __func__)
#define ASSERT_UNREACHABLE_FMT(message, ...) ASSERT_UNREACHABLE(std::format(message __VA_OPT__(, ) __VA_ARGS__))
#else
#define ASSERT_UNREACHABLE(message)          manganese_unreachable()
#define ASSERT_UNREACHABLE_FMT(message, ...) manganese_unreachable()
#endif  // MN_DEBUG

#ifndef MN_BEGIN_C_LINKAGE
#ifdef __cplusplus
#define MN_BEGIN_C_LINKAGE extern "C" {
#else
#define MN_BEGIN_C_LINKAGE
#endif
#endif

#ifndef MN_END_C_LINKAGE
#ifdef __cplusplus
#define MN_END_C_LINKAGE }
#else
#define MN_END_C_LINKAGE
#endif
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#define MN_RUNTIME_API_FUNCTION __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
#define MN_RUNTIME_API_FUNCTION __attribute__((visibility("default")))
#else
#define MN_RUNTIME_API_FUNCTION
#endif

#endif  // MANGANESE_INCLUDE_CORE_HPP