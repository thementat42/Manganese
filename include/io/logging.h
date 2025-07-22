

/**
 * @file logging.h
 * @brief Provides logging utilities for the Manganese project, including internal and user-facing logging functions with support for log levels and colored terminal output.
 *
 * This header defines logging functions and log levels for both internal compiler messages and user-facing errors or warnings.
 * It also includes ANSI color codes for enhanced terminal output readability.
 *
*/
#ifndef MANGANESE_INCLUDE_IO_LOGGING_H
#define MANGANESE_INCLUDE_IO_LOGGING_H

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
void logInternal(const std::string& message, LogLevel level = LogLevel::Info, std::ostream& out = std::cerr) noexcept_if_release;

/**
 * @brief Logging function for the user (e.g., syntax errors)
 * @param message The message to log
 * @param line The line number in the user code where the error occurred
 * @param col The column number in the user code where the error occurred
 * @param level The log level (default is Warning)
 * @param out The output stream to log to (default is std::cerr)
 */
void log(const std::string& message, LogLevel level = LogLevel::Warning, size_t line = 0, size_t col = 0, std::ostream& out = std::cerr) noexcept;

void log(std::initializer_list<std::string> messages, LogLevel level = LogLevel::Warning, size_t line = 0, size_t col = 0) noexcept;

inline void logWarning(const std::string& message, size_t line = 0, size_t col = 0, std::ostream& out = std::cerr) noexcept{
    log(message, LogLevel::Warning, line, col, out);
}
inline void logError(const std::string& message, size_t line = 0, size_t col = 0, std::ostream& out = std::cerr) noexcept {
    log(message, LogLevel::Error, line, col, out);
}

inline void logCritical(const std::string& message, size_t line = 0, size_t col = 0, std::ostream& out = std::cerr) noexcept {
    log(message, LogLevel::Critical, line, col, out);
}


}  // namespace logging
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_IO_LOGGING_H