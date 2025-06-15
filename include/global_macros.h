/**
 * @file global_macros.h
 * @brief Global macros and utility definitions for the project.
 * @details Contains useful macros for debugging, assertions, and C linkage, and common macros used throughout the project.
 */

#ifndef GLOBAL_MACROS_H
#define GLOBAL_MACROS_H

#include <stdint.h>

#include <iostream>

#ifndef DEBUG    // Defined by CMake (see CMakeLists.txt)
#define DEBUG 0  // Default to release mode if not defined
#endif           // DEBUG

#define DISCARD(x) (void)(x)  // Explicitly discard a value

#if defined(__clang__)
#define __DISABLE_CONVERSION_WARNING \
    _Pragma("clang diagnostic push") \
        _Pragma("clang diagnostic ignored \"-Wconversion\"")

#define __ENABLE_CONVERSION_WARNING \
    _Pragma("clang diagnostic pop")

#elif defined(__GNUC__)
#define __DISABLE_CONVERSION_WARNING \
    _Pragma("GCC diagnostic push")   \
        _Pragma("GCC diagnostic ignored \"-Wconversion\"")

#define __ENABLE_CONVERSION_WARNING \
    _Pragma("GCC diagnostic pop")

#elif defined(_MSC_VER)
#define __DISABLE_CONVERSION_WARNING \
    __pragma(warning(push))          \
        __pragma(warning(disable : 4244))  // 4244 is the warning code for conversion from 'type1' to 'type2', possible loss of data

#define __ENABLE_CONVERSION_WARNING \
    __pragma(warning(pop))

#else
#define __DISABLE_CONVERSION_WARNING
#define __ENABLE_CONVERSION_WARNING
#endif

#define DISABLE_CONVERSION_WARNING __DISABLE_CONVERSION_WARNING  // Supress a warning about a conversion causing data loss
#define ENABLE_CONVERSION_WARNING __ENABLE_CONVERSION_WARNING    // Re-enable the conversion warning

#ifdef __cplusplus
#define EXT_C_BEGIN extern "C" {
#define EXT_C_END }
#define MANGANESE_BEGIN namespace Manganese {
#define MANGANESE_END }

#define DISABLE_COPY_AND_ASSIGN(ClassName) \
    ClassName(const ClassName&) = delete;  \
    ClassName& operator=(const ClassName&) = delete;  // Disable copy constructor and assignment operator
#define DISABLE_MOVE(ClassName)      \
    ClassName(ClassName&&) = delete; \
    ClassName& operator=(ClassName&&) = delete;  // Disable move constructor and assignment operator
#define DISABLE_COPY_MOVE(ClassName)   \
    DISABLE_COPY_AND_ASSIGN(ClassName) \
    DISABLE_MOVE(ClassName)  // Disable both copy and move semantics
#endif                       // __cplusplus

#define ASSERT_CRITICAL(condition, message)                                               \
    do {                                                                                  \
        if (!(condition)) {                                                               \
            std::cerr << "\033[31mCritical assertion failed: " << message << "\n\033[0m"; \
            __PRINT_LOCATION()                                                            \
            exit(EXIT_FAILURE);                                                           \
        }                                                                                 \
    } while (0)

#if DEBUG
#define ASSERT_DEBUG(condition, message)                                               \
    do {                                                                               \
        if (!(condition)) {                                                            \
            std::cerr << "\033[31mDebug assertion failed: " << message << "\n\033[0m"; \
            __PRINT_LOCATION()                                                         \
        }                                                                              \
    } while (0)
#else                                     // ^ DEBUG ^ | v !DEBUG v
// In release mode, inline the debug functions (since they do nothing)
#define ASSERT_DEBUG(condition, message)  // no-op in release mode
#endif                                    // DEBUG

#endif  // GLOBAL_MACROS_H