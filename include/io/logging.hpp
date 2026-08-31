#ifndef MANGANESE_INCLUDE_IO_LOGGING_HPP
#define MANGANESE_INCLUDE_IO_LOGGING_HPP

#include <core.hpp>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

// defined in CMake; if a terminal doesn't support ANSI colour sequences
// This macro can disable them
#if NO_ANSI_COLOURS
#define ANSI_VAL(val) ""
#else
#define ANSI_VAL(val) val
#endif

// ANSI color codes for terminal output
namespace ansi {
constexpr inline std::string_view GREEN = ANSI_VAL("\033[32m");
constexpr inline std::string_view PINK = ANSI_VAL("\033[95m");
constexpr inline std::string_view RED = ANSI_VAL("\033[31m");
constexpr inline std::string_view YELLOW = ANSI_VAL("\033[33m");
constexpr inline std::string_view BLUE = ANSI_VAL("\033[34m");
constexpr inline std::string_view CYAN = ANSI_VAL("\033[36m");
constexpr inline std::string_view CRITICAL = ANSI_VAL("\033[91;1m");
constexpr inline std::string_view RESET = ANSI_VAL("\033[0m");
}  // namespace ansi

namespace Manganese::logging {

enum class LogLevel : std::uint8_t {
    Info,
    Warning,
    Error,
    Critical
};

template <class... Args>
void logInternal(LogLevel level, std::format_string<Args...> fmt, Args&&... args) {
#if MN_DEBUG
    std::string message = std::format(fmt, std::forward<Args>(args)...);
    switch (level) {
        case LogLevel::Info: std::cerr << ansi::BLUE << "[Internal Info] " << message << ansi::RESET << "\n"; break;
        case LogLevel::Warning:
            std::cerr << ansi::YELLOW << "[Internal Warning] " << message << ansi::RESET << "\n";
            break;
        case LogLevel::Error: std::cerr << ansi::RED << "[Internal Error] " << message << ansi::RESET << "\n"; break;
        case LogLevel::Critical:
            std::cerr << ansi::RED << "[Internal Critical Error] " << message << ansi::RESET << "\n";
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
void log(LogLevel level, std::size_t line, std::size_t col, std::format_string<Args...> fmt, Args&&... args) {
    std::string message = std::format(fmt, std::forward<Args>(args)...);
    switch (level) {
        case LogLevel::Info: return;  // No user info
        case LogLevel::Warning: std::cerr << ansi::YELLOW << "Warning: " << message << ansi::RESET; break;
        case LogLevel::Error: std::cerr << ansi::RED << "Error: " << message << ansi::RESET; break;
        case LogLevel::Critical:
            std::cerr << ansi::CRITICAL << "Critical error: " << message << " Compilation aborted." << ansi::RESET;
            break;
    }
    std::cerr << " (line " << line << ", column " << col << ")\n";
}

template <class... Args>
FORCE_INLINE void logWarning(std::size_t line, std::size_t col, std::format_string<Args...> fmt, Args&&... args) {
    log(LogLevel::Warning, line, col, fmt, std::forward<Args>(args)...);
}

template <class... Args>
FORCE_INLINE void logError(std::size_t line, std::size_t col, std::format_string<Args...> fmt, Args&&... args) {
    log(LogLevel::Error, line, col, fmt, std::forward<Args>(args)...);
}

template <class... Args>
FORCE_INLINE void logCritical(std::size_t line, std::size_t col, std::format_string<Args...> fmt, Args&&... args) {
    log(LogLevel::Critical, line, col, fmt, std::forward<Args>(args)...);
}

}  // namespace Manganese::logging

#endif  // MANGANESE_INCLUDE_IO_LOGGING_HPP