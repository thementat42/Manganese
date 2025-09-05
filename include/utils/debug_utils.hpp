#ifndef MANGANESE_INCLUDE_UTILS_DEBUG_UTILS_HXX
#define MANGANESE_INCLUDE_UTILS_DEBUG_UTILS_HXX

#include <utils/compiler_config.h>

#include <iostream>
#include <stdexcept>

/**
 * @brief Some basic utils for debugging
 * @note This file is included in global_macros for convenience. It should not be directly included
 */

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
#define __MANGANESE_ASSERT_UNREACHABLE(message) COMPILER_UNREACHABLE
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

#endif  // MANGANESE_INCLUDE_UTILS_DEBUG_UTILS_HXX