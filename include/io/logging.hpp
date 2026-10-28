#ifndef MANGANESE_INCLUDE_IO_LOGGING_HPP
#define MANGANESE_INCLUDE_IO_LOGGING_HPP

#include <core.hpp>
#include <cstddef>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// defined in CMake; if a terminal doesn't support ANSI colour sequences
// This macro can disable them
#if NO_ANSI_COLOURS
#define ANSI_VAL(val) ""
#else
#define ANSI_VAL(val) val
#endif

// ANSI colour codes for terminal output
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

struct LogRecord {
    LogLevel level;
    std::size_t line;
    std::size_t col;
    std::string message;
};

struct [[nodiscard]] ThreadLogBufferGuard {
    std::vector<LogRecord> records;
    std::vector<LogRecord>* oldBuffer = nullptr;

    ThreadLogBufferGuard();
    ~ThreadLogBufferGuard();

    ThreadLogBufferGuard(const ThreadLogBufferGuard&) = delete;
    ThreadLogBufferGuard& operator=(const ThreadLogBufferGuard&) = delete;
};

void writeToStderr(std::string_view prefixColor, std::string_view label, std::string_view message, std::size_t line,
                   std::size_t col);

void logInternalRaw(LogLevel level, std::string_view message);
void logRaw(LogLevel level, std::size_t line, std::size_t col, std::string_view message);

// Template functions kept in header because they need std::format_string evaluation
template <class... Args>
void logInternal(LogLevel level, std::format_string<Args...> fmt, Args&&... args) {
#if MN_DEBUG
    std::string message = std::format(fmt, std::forward<Args>(args)...);
    logInternalRaw(level, message);
    if (level == LogLevel::Critical) {
        // Re-construct formatted string for exception message if needed
        throw std::runtime_error{message};
    }
#else
    DISCARD(level);
    DISCARD(fmt);
    ((void)(args), ...);
#endif
}

template <class... Args>
void log(LogLevel level, std::size_t line, std::size_t col, std::format_string<Args...> fmt, Args&&... args) {
    if (level == LogLevel::Info) { return; }
    std::string message = std::format(fmt, std::forward<Args>(args)...);
    logRaw(level, line, col, message);
}

// Convenience wrappers

template <class... Args>
inline void logWarning(std::size_t line, std::size_t col, std::format_string<Args...> fmt, Args&&... args) {
    log(LogLevel::Warning, line, col, fmt, std::forward<Args>(args)...);
}

template <class... Args>
inline void logError(std::size_t line, std::size_t col, std::format_string<Args...> fmt, Args&&... args) {
    log(LogLevel::Error, line, col, fmt, std::forward<Args>(args)...);
}

template <class... Args>
inline void logCritical(std::size_t line, std::size_t col, std::format_string<Args...> fmt, Args&&... args) {
    log(LogLevel::Critical, line, col, fmt, std::forward<Args>(args)...);
}

}  // namespace Manganese::logging

#endif  // MANGANESE_INCLUDE_IO_LOGGING_HPP