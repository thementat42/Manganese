/**
 * @file compiler_config.h
 * @brief This file contains configuration settings for the compiler, such as enabling/disabling warnings,
 * debugging options, and other compiler-specific settings.
 * This file is included in the global_macros.h file for convenience.
 */

#ifndef COMPILER_CONFIG_H
#define COMPILER_CONFIG_H

//~ Conversion Warning

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

//~ Sign Comparison Warning

#if defined(__clang__)
#define __DISABLE_SIGN_COMPARE_WARNING \
    _Pragma("clang diagnostic push")   \
        _Pragma("clang diagnostic ignored \"-Wsign-compare\"")

#define __ENABLE_SIGN_COMPARE_WARNING \
    _Pragma("clang diagnostic pop")

#elif defined(__GNUC__)
#define __DISABLE_SIGN_COMPARE_WARNING \
    _Pragma("GCC diagnostic push")     \
        _Pragma("GCC diagnostic ignored \"-Wsign-compare\"")

#define __ENABLE_SIGN_COMPARE_WARNING \
    _Pragma("GCC diagnostic pop")

#elif defined(_MSC_VER)
#define __DISABLE_SIGN_COMPARE_WARNING \
    __pragma(warning(push))            \
        __pragma(warning(disable : 4018))  // 4018: signed/unsigned mismatch

#define __ENABLE_SIGN_COMPARE_WARNING \
    __pragma(warning(pop))

#else
#define __DISABLE_SIGN_COMPARE_WARNING
#define __ENABLE_SIGN_COMPARE_WARNING
#endif

//~ Format Warning

#if defined(__clang__)
#define __DISABLE_FORMAT_WARNING     \
    _Pragma("clang diagnostic push") \
        _Pragma("clang diagnostic ignored \"-Wformat\"")

#define __ENABLE_FORMAT_WARNING \
    _Pragma("clang diagnostic pop")

#elif defined(__GNUC__)
#define __DISABLE_FORMAT_WARNING   \
    _Pragma("GCC diagnostic push") \
        _Pragma("GCC diagnostic ignored \"-Wformat\"")

#define __ENABLE_FORMAT_WARNING \
    _Pragma("GCC diagnostic pop")

#elif defined(_MSC_VER)
#define __DISABLE_FORMAT_WARNING \
    __pragma(warning(push))      \
        __pragma(warning(disable : 4477))  // 4477: format string '%x' requires an argument of type 'y', but variadic argument z has type 'w'

#define __ENABLE_FORMAT_WARNING \
    __pragma(warning(pop))

#else
#define __DISABLE_FORMAT_WARNING
#define __ENABLE_FORMAT_WARNING
#endif

#if DEBUG
// In debug builds, enable the warnings so -Werror doesn't throw on them
#define DISABLE_CONVERSION_WARNING __DISABLE_CONVERSION_WARNING  // Supress a warning about a conversion causing data loss
#define ENABLE_CONVERSION_WARNING __ENABLE_CONVERSION_WARNING    // Re-enable the conversion warning

#define DISABLE_SIGN_COMPARE_WARNING __DISABLE_SIGN_COMPARE_WARNING  // Suppress warnings about signed/unsigned comparison
#define ENABLE_SIGN_COMPARE_WARNING __ENABLE_SIGN_COMPARE_WARNING    // Re-enable the sign comparison warning

#define DISABLE_FORMAT_WARNING __DISABLE_FORMAT_WARNING  // Suppress warnings about format string mismatches
#define ENABLE_FORMAT_WARNING __ENABLE_FORMAT_WARNING    // Re-enable the format warning
#else  // ^^ DEBUG vv !DEBUG
// Release builds don't use -Werror, so there's no need to disable warnings
#define DISABLE_CONVERSION_WARNING
#define ENABLE_CONVERSION_WARNING
#define DISABLE_SIGN_COMPARE_WARNING
#define ENABLE_SIGN_COMPARE_WARNING
#define DISABLE_FORMAT_WARNING
#define ENABLE_FORMAT_WARNING
#endif  // DEBUG

#endif  // COMPILER_CONFIG_H