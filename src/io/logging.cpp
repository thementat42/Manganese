/**
 * @file logging.cpp
 * @brief Implements logging utilities for the Manganese project.
 *
 * This file provides functions for logging messages at various severity levels
 * (Info, Warning, Error, Critical) to the standard output and error streams.
 * It supports logging single messages as well as multiple messages, and includes
 * contextual information such as line and column numbers.
 */

#include <io/logging.hpp>

#include <iostream>
#include <string>

namespace Manganese {
namespace logging {
void logInternal(const std::string& message, LogLevel level, std::ostream& out) NOEXCEPT_IF_RELEASE {
#if DEBUG
    switch (level) {
        case LogLevel::Info: out << BLUE << "[Internal Info] " << message << RESET << "\n"; break;
        case LogLevel::Warning: out << YELLOW << "[Internal Warning] " << message << RESET << "\n"; break;
        case LogLevel::Error: out << RED << "[Internal Error] " << message << RESET << "\n"; break;
        case LogLevel::Critical: out << RED << "[Internal Critical Error] " << message << RESET << "\n"; break;
    }
    if (level == LogLevel::Critical) {
        out << ("Critical error encountered");
        throw std::runtime_error("Critical error");
    }
#else  // ^^ DEBUG vv !DEBUG
    return;  // No internal logging in non-debug builds
#endif  // DEBUG
}

void log(const std::string& message, LogLevel level, size_t line, size_t col, std::ostream& out) noexcept {
    switch (level) {
        case LogLevel::Info: return;  // No user info
        case LogLevel::Warning: out << YELLOW << "Warning: " << message << RESET; break;
        case LogLevel::Error: out << RED << "Error: " << message << RESET; break;
        case LogLevel::Critical:
            out << CRITICAL << "Critical error: " << message << " Compilation aborted." << RESET;
            break;
    }
    out << " (line " << line << ", column " << col << ")\n";
}

void log(std::initializer_list<std::string> messages, LogLevel level, size_t line, size_t col,
         std::ostream& out) noexcept {
    switch (level) {
        case LogLevel::Info: break;  // No user info
        case LogLevel::Warning: out << YELLOW << "Warning: "; break;
        case LogLevel::Error: out << RED << "Error: "; break;
        case LogLevel::Critical: out << CRITICAL << "Critical error: "; break;
    }
    for (const auto& message : messages) { out << message << "\n"; }
    out << RESET;
    if (line != 0 && col != 0) { out << "(line " << line << ", column " << col << ")\n"; }
}
}  // namespace logging
}  // namespace Manganese
