#ifndef MANGANESE_INCLUDE_IO_LOGGING_HPP
#define MANGANESE_INCLUDE_IO_LOGGING_HPP

#include <format>  // Include format here so any files that use logging have it included
#include <core.hpp>
#include <iostream>

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

enum class LogLevel : uint8_t {
    Info,
    Warning,
    Error,
    Critical
};

template <class... Args>
void logInternal(LogLevel level, std::format_string<Args...> fmt, Args&&... args) NOEXCEPT_IF_RELEASE {
#if MN_DEBUG
    auto message = std::format(fmt, std::forward<Args>(args)...);
    switch (level) {
        case LogLevel::Info: std::cerr << BLUE << "[Internal Info] " << message << RESET << "\n"; break;
        case LogLevel::Warning: std::cerr << YELLOW << "[Internal Warning] " << message << RESET << "\n"; break;
        case LogLevel::Error: std::cerr << RED << "[Internal Error] " << message << RESET << "\n"; break;
        case LogLevel::Critical:
            std::cerr << RED << "[Internal Critical Error] " << message << RESET << "\n";
            std::cerr << "Critical error encountered";
            throw std::runtime_error("Critical error");
    }
#else  // ^^ MN_DEBUG vv !MN_DEBUG
    DISCARD(level);
    DISCARD(fmt);
    (void)((void)args, ...);
    return;  // No internal logging in non-debug builds
#endif  // MN_DEBUG
}

template <class... Args>
void log(LogLevel level, size_t line, size_t col, std::format_string<Args...> fmt, Args&&... args) noexcept {
    auto message = std::format(fmt, std::forward<Args>(args)...);
    switch (level) {
        case LogLevel::Info: return;  // No user info
        case LogLevel::Warning: std::cerr << YELLOW << "Warning: " << message << RESET; break;
        case LogLevel::Error: std::cerr << RED << "Error: " << message << RESET; break;
        case LogLevel::Critical:
            std::cerr << CRITICAL << "Critical error: " << message << " Compilation aborted." << RESET;
            break;
    }
    std::cerr << " (line " << line << ", column " << col << ")\n";
}

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