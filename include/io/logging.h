#ifndef LOGGING_H
#define LOGGING_H

#include <global_macros.h>

#include <iostream>
#include <stdexcept>
#include <format>  // Added for std::format

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
void log(const std::string& message, LogLevel level = LogLevel::Warning, size_t line = 0, size_t col = 0);

void log(std::initializer_list<std::string> messages, LogLevel level = LogLevel::Warning, size_t line = 0, size_t col = 0);

inline void logWarning(const std::string& message, size_t line = 0, size_t col = 0) {
    log(message, LogLevel::Warning, line, col);
}
inline void logError(const std::string& message, size_t line = 0, size_t col = 0) {
    log(message, LogLevel::Error, line, col);
}

inline void logCritical(const std::string& message, size_t line = 0, size_t col = 0) {
    log(message, LogLevel::Critical, line, col);
}


}  // namespace logging
}  // namespace Manganese

#endif  // LOGGING_H