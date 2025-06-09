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

// TODO: Replace printing macros with a proper logging system

#define __PRINT_LOCATION() \
    std::cerr << "\033[33m" << __FILE__ << ", " << __LINE__ << ": " << __func__ << "\033[0m\n";  // Used inside other macros only: Print the file, line number, and function name

#define LOG_LINE_COL(line, col) \
    std::cerr << "(line " << line << ", column " << col << ")\n";  // Used for error messages: Print the line and column number

#define UNREACHABLE(message)                                                         \
    do {                                                                             \
        std::cerr << "\033[31mUnreachable code reached: " << message << "\n\033[0m"; \
        __PRINT_LOCATION()                                                           \
        exit(EXIT_FAILURE);                                                          \
    } while (0)

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

#ifdef __cplusplus
#define EXT_C_BEGIN extern "C" {
#define EXT_C_END }
#else  // ^^ __cplusplus vv !__cplusplus
#define EXT_C_BEGIN
#define EXT_C_END
#endif  // __cplusplus

#endif  // GLOBAL_MACROS_H