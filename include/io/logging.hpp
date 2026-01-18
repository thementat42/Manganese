

/**
 * @file logging.hpp
 * @brief Provides logging utilities for the Manganese project, including internal and user-facing logging functions
 * with support for log levels and colored terminal output.
 *
 * This header defines logging functions and log levels for both internal compiler messages and user-facing errors or
 * warnings. It also includes ANSI color codes for enhanced terminal output readability.
 *
 */
#ifndef MANGANESE_INCLUDE_IO_LOGGING_HPP
#define MANGANESE_INCLUDE_IO_LOGGING_HPP

#include <format>  // Include format here so any files that use logging have it included
#include <global_macros.hpp>


// ANSI color codes for terminal output
constexpr const char* GREEN = "\033[32m";
constexpr const char* PINK = "\033[95m";
constexpr const char* RED = "\033[31m";
constexpr const char* YELLOW = "\033[33m";
constexpr const char* BLUE = "\033[34m";
constexpr const char* CYAN = "\033[36m";
constexpr const char* CRITICAL = "\033[91;1m";
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

template <class... Args>
void logInternal(LogLevel level, std::format_string<Args...> fmt, Args&&... args) NOEXCEPT_IF_RELEASE;

template <class... Args>
void log(LogLevel level, size_t line, size_t col, std::format_string<Args...> fmt, Args&&... args) noexcept;

template <class... Args>
FORCE_INLINE void logWarning(size_t line, size_t col, std::format_string<Args...> fmt, Args&&... args) noexcept {
    log(LogLevel::Warning, line, col, fmt, std::forward<Args>(args)...);
}

template <class... Args>
FORCE_INLINE void logError(size_t line, size_t col, std::format_string<Args...> fmt, Args&&... args) noexcept {
    log(LogLevel::Error, line, col, fmt, std::forward<Args>(args)...);
}

template <class... Args>
FORCE_INLINE void logCritical(size_t line, size_t col, std::format_string<Args...> fmt, Args&&... args) noexcept {
    log(LogLevel::Critical, line, col, fmt, std::forward<Args>(args)...);
}

}  // namespace logging
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_IO_LOGGING_HPP