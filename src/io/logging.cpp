/**
 * @file logging.cpp
 * @brief Implements logging utilities for the Manganese project.
 *
 * This file provides functions for logging messages at various severity levels
 * (Info, Warning, Error, Critical) to the standard output and error streams.
 * It supports logging single messages as well as multiple messages, and includes
 * contextual information such as line and column numbers.
 */

#include <format>
#include <io/logging.hpp>
#include <iostream>

namespace Manganese {
namespace logging {
template <class... Args>
void logInternal(LogLevel level, std::format_string<Args...> fmt, Args&&... args) NOEXCEPT_IF_RELEASE {
#if DEBUG
    auto message = std::format(fmt, std::forward<Args>(args)...);
    switch (level) {
        case LogLevel::Info: std::cerr << BLUE << "[Internal Info] " << message << RESET << "\n"; break;
        case LogLevel::Warning: std::cerr << YELLOW << "[Internal Warning] " << message << RESET << "\n"; break;
        case LogLevel::Error: std::cerr << RED << "[Internal Error] " << message << RESET << "\n"; break;
        case LogLevel::Critical: std::cerr << RED << "[Internal Critical Error] " << message << RESET << "\n"; break;
    }
    if (level == LogLevel::Critical) {
        std::cerr << ("Critical error encountered");
        throw std::runtime_error("Critical error");
    }
#else  // ^^ DEBUG vv !DEBUG
    return;  // No internal logging in non-debug builds
#endif  // DEBUG
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
}  // namespace logging
}  // namespace Manganese
