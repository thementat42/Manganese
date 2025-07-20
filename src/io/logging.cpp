/**
 * @file logging.cpp
 * @brief Implements logging utilities for the Manganese project.
 *
 * This file provides functions for logging messages at various severity levels
 * (Info, Warning, Error, Critical) to the standard output and error streams.
 * It supports logging single messages as well as multiple messages, and includes
 * contextual information such as line and column numbers.
 */

#include <io/logging.h>

#include <iostream>
#include <string>

namespace Manganese {
namespace logging {
void logInternal(const std::string& message, LogLevel level, std::ostream& out) {
#if DEBUG
    switch (level) {
        case LogLevel::Info:
            out << BLUE << "[INFO] " << message << RESET << "\n";
            break;
        case LogLevel::Warning:
            out << YELLOW << "[WARNING] " << message << RESET << "\n";
            break;
        case LogLevel::Error:
            out << RED << "[ERROR] " << message << RESET << "\n";
            break;
        case LogLevel::Critical:
            out << RED << "[CRITICAL] " << message << RESET << "\n";

            break;
    }
    if (level == LogLevel::Critical) {
        out << ("Critical error encountered");
        throw std::runtime_error("Critical error");
    }
#else   // ^^ DEBUG vv !DEBUG
    return;  // No internal logging in non-debug builds
#endif  // DEBUG
}

void log(const std::string& message, LogLevel level, size_t line, size_t col, std::ostream& out) {
    switch (level) {
        case LogLevel::Info:
            return;  // No user info
        case LogLevel::Warning:
            out << "\033[33m Warning: " << message << "\033[0m";
            break;
        case LogLevel::Error:
            out << "\033[31mError: " << message << "\033[0m";
            break;
        case LogLevel::Critical:
            out << "\033[91;1mCritical error: " << message << " Compilation aborted." << "\033[0m";
            break;
    }
    out << " (line " << line << ", column " << col << ")\n";
}

void log(std::initializer_list<std::string> messages, LogLevel level, size_t line, size_t col, std::ostream& out) {
    switch (level) {
        case LogLevel::Info:
            break;  // No user info
        case LogLevel::Warning:
            out << "\033[33mWarning: ";
            break;
        case LogLevel::Error:
            out << "\033[31mError: ";
            break;
        case LogLevel::Critical:
            out << "\033[91;1mCritical error: ";
            break;
    }
    for (const auto& message : messages) {
        out << message << "\n";
    }
    out << "\033[0m\n";
    if (line != 0 && col != 0) {
        out << "(line " << line << ", column " << col << ")\n";
    }
}
}  // namespace logging
}  // namespace Manganese
