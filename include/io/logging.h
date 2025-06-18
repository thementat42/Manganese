#ifndef LOGGING_H
#define LOGGING_H

#include <global_macros.h>

#include <iostream>
#if __cplusplus >= 202302L
#include <utility>
#define COMPILER_UNREACHABLE std::unreachable();  // compiler agnostic, but still allows for optimisation
// If < C++23, use a compiler-specific implementation
#elif defined(__GNUC__) || defined(__clang__)
#define COMPILER_UNREACHABLE __builtin_unreachable();
#elif defined(_MSC_VER)
#define COMPILER_UNREACHABLE __assume(false);
#else
#define COMPILER_UNREACHABLE
#endif  // __cplusplus >= 202302L

#ifndef __PRINT_LOCATION
#define __PRINT_LOCATION \
    std::cerr << "\033[33m In file:" << __FILE__ << ", at" << __LINE__ << ": when running " << __func__ << "\033[0m\n";

#define PRINT_LOCATION __PRINT_LOCATION  // Print the location of the log message (in the compiler source, not the user code)

#define __UNREACHABLE(message)                                                       \
    do {                                                                             \
        std::cerr << "\033[31mUnreachable code reached: " << message << "\n\033[0m"; \
        PRINT_LOCATION                                                               \
        COMPILER_UNREACHABLE                                                         \
        exit(EXIT_FAILURE);                                                          \
    } while (0)

#define UNREACHABLE(message) __UNREACHABLE(message)  // Condition that should never be reached
#endif                                               // __PRINT_LOCATION

// ANSI color codes for terminal output
constexpr const char* GREEN = "\033[32m";
constexpr const char* PINK = "\033[95m";
constexpr const char* RED = "\033[31m";
constexpr const char* YELLOW = "\033[33m";
constexpr const char* BLUE = "\033[34m";
constexpr const char* CYAN = "\033[36m";
constexpr const char* RESET = "\033[0m";

namespace Manganese {
namespace logging {

enum class LogLevel {
    Info,
    Warning,
    Error,
    Critical
};

/**
 * @brief Internal logging function for the compiler (e.g., debug messages, critical bugs)
 * @param message The message to log
 * @param level The log level (default is Info)
 *
 */
void logInternal(const std::string& message, LogLevel level = LogLevel::Info);

/**
 * @brief Logging function for the user (e.g., syntax errors)
 * @param message The message to log
 * @param line The line number in the user code where the error occurred
 * @param col The column number in the user code where the error occurred
 * @param level The log level (default is Warning)
 */
void logUser(const std::string& message, LogLevel level = LogLevel::Warning, size_t line = 0, size_t col = 0);

void logUser(std::initializer_list<std::string> messages, LogLevel level = LogLevel::Warning, size_t line = 0, size_t col = 0);

}  // namespace logging
}  // namespace Manganese

#endif  // LOGGING_H