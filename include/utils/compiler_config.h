/**
 * @file compiler_config.h
 * @brief This file contains configuration settings for the compiler, such as enabling/disabling warnings,
 * debugging options, and other compiler-specific settings.
 * This file is included in the global_macros.h file for convenience.
 */

#ifndef COMPILER_CONFIG_H
#define COMPILER_CONFIG_H

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

#endif  // COMPILER_CONFIG_H