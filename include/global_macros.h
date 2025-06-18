/**
 * @file global_macros.h
 * @brief Global macros and utility definitions for the project.
 * @details Contains useful macros for debugging, assertions, and C linkage, and common macros used throughout the project.
 */

#ifndef GLOBAL_MACROS_H
#define GLOBAL_MACROS_H

#include <stdint.h>
#include <utils/compiler_config.h>

#include <iostream>
#include <stdexcept>

#ifndef DEBUG    // Defined by CMake (see CMakeLists.txt)
#define DEBUG 0  // Default to release mode if not defined
#endif           // DEBUG


/**
 * This function is intended to be noexcept,
 * except for a default case that should logically never happen
 * If that case happens, something has gone horribly wrong, and the function will throw an exception
 * 
 * This serves as a two way check -- functions marked with this should have a fallback case that throws -- if it doesn't add one
 * If a function has a fallback throw case but not this macro, add it
 * 
 * NOTE: This should only be used on functions that throw on catastrophic failures (e.g. use the UNREACHABLE macro from the logging library). It should not be used on functions that just never throw (those should be marked noexcept)
 */
#define noexcept_except_catastrophic

#define DISCARD(x) (void)(x)  // Explicitly discard a value

#define DISABLE_COPY_AND_ASSIGN(ClassName) \
    ClassName(const ClassName&) = delete;  \
    ClassName& operator=(const ClassName&) = delete;  // Disable copy constructor and assignment operator
#define DISABLE_MOVE(ClassName)      \
    ClassName(ClassName&&) = delete; \
    ClassName& operator=(ClassName&&) = delete;  // Disable move constructor and assignment operator
#define DISABLE_COPY_MOVE(ClassName)   \
    DISABLE_COPY_AND_ASSIGN(ClassName) \
    DISABLE_MOVE(ClassName)  // Disable both copy and move semantics

#define ASSERT_CRITICAL(condition, message)                                               \
    do {                                                                                  \
        if (!(condition)) {                                                               \
            std::cerr << "\033[31mCritical assertion failed: " << message << "\n\033[0m"; \
            __PRINT_LOCATION()                                                            \
            throw std::runtime_error(message);                                            \
        }                                                                                 \
    } while (0);

#if DEBUG
#define ASSERT_DEBUG(condition, message)                                               \
    do {                                                                               \
        if (!(condition)) {                                                            \
            std::cerr << "\033[31mDebug assertion failed: " << message << "\n\033[0m"; \
            __PRINT_LOCATION()                                                         \
        }                                                                              \
    } while (0);
#else                                     // ^ DEBUG ^ | v !DEBUG v
// In release mode, inline the debug functions (since they do nothing)
#define ASSERT_DEBUG(condition, message)  // no-op in release mode
#endif                                    // DEBUG

#endif  // GLOBAL_MACROS_H