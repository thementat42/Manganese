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

inline void writeToStderr(std::string_view prefixColor, std::string_view label, std::string_view message,
                          std::size_t line, std::size_t col) {
    std::string out = std::format("{}{}{} (line {}, column {})\n", prefixColor, label, message, line, col);

    // Using .write() bypasses operator<<
    std::cerr.write(out.data(), static_cast<std::streamsize>(out.size()));
}

template <class... Args>
void logInternal(LogLevel level, std::format_string<Args...> fmt, Args&&... args) {
#if MN_DEBUG
    std::string message = std::format(fmt, std::forward<Args>(args)...);
    std::string out;
    switch (level) {
        case LogLevel::Info: out = std::format("{}[Internal Info] {}{}\n", ansi::BLUE, message, ansi::RESET); break;
        case LogLevel::Warning:
            out = std::format("{}[Internal Warning] {}{}\n", ansi::YELLOW, message, ansi::RESET);
            break;
        case LogLevel::Error: out = std::format("{}[Internal Error] {}{}\n", ansi::RED, message, ansi::RESET); break;
        case LogLevel::Critical:
            out = std::format("{}[Internal Critical Error] {}{}\n", ansi::RED, message, ansi::RESET);
            std::cout.write(out.data(), static_cast<std::streamsize>(out.size()));
            std::cout.write("Critical error encountered\n", 27);
            throw std::runtime_error("Critical error");
    }
    std::cout.write(out.data(), static_cast<std::streamsize>(out.size()));
#else
    DISCARD(level);
    DISCARD(fmt);
    ((void)(args), ...);
#endif
}

template <class... Args>
void log(LogLevel level, std::size_t line, std::size_t col, std::format_string<Args...> fmt, Args&&... args) {
    std::string message = std::format(fmt, std::forward<Args>(args)...);

    switch (level) {
        case LogLevel::Info: return;
        case LogLevel::Warning: writeToStderr(ansi::YELLOW, "Warning: ", message, line, col); break;
        case LogLevel::Error: writeToStderr(ansi::RED, "Error: ", message, line, col); break;
        case LogLevel::Critical:
            std::string critMessage
                = std::format("{}Critical error: {} Compilation aborted.{}", ansi::CRITICAL, message, ansi::RESET);
            writeToStderr("", "", critMessage, line, col);
            break;
    }
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