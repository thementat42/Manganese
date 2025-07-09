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
void logInternal(const std::string& message, LogLevel level) {
    switch (level) {
        case LogLevel::Info:
            std::cout << BLUE << "[INFO] " << message << RESET << "\n";
            break;
        case LogLevel::Warning:
            std::cout << YELLOW << "[WARNING] " << message << RESET << "\n";
            break;
        case LogLevel::Error:
            std::cerr << RED << "[ERROR] " << message << RESET << "\n";
            break;
        case LogLevel::Critical:
            std::cerr << RED << "[CRITICAL] " << message << RESET << "\n";

            break;
    }
    if (level == LogLevel::Critical) {
        ASSERT_UNREACHABLE("Critical error encountered");
    }
}

void log(const std::string& message, LogLevel level, size_t line, size_t col) {
    switch (level) {
        case LogLevel::Info:
            return;  // No user info
        case LogLevel::Warning:
            std::cout << "\033[33m Warning: " << message << "\033[0m\n";
            break;
        case LogLevel::Error:
            std::cerr << "\033[31mError: " << message << "\033[0m\n";
            break;
        case LogLevel::Critical:
            std::cerr << "\033[91;1mCritical error: " << message << "\n Compilation aborted." << "\033[0m\n";
            break;
    }
    std::cerr << "(line " << line << ", column " << col << ")\n";
}

void log(std::initializer_list<std::string> messages, LogLevel level, size_t line, size_t col) {
    switch (level) {
        case LogLevel::Info:
            break;  // No user info
        case LogLevel::Warning:
            std::cout << "\033[33mWarning: ";
            break;
        case LogLevel::Error:
            std::cerr << "\033[31mError: ";
            break;
        case LogLevel::Critical:
            std::cerr << "\033[91;1mCritical error: ";
            break;
    }
    for (const auto& message : messages) {
        std::cerr << message << " ";
    }
    std::cerr << "\033[0m\n";
    if (line != 0 && col != 0) {
        std::cerr << "(line " << line << ", column " << col << ")\n";
    }
}
}  // namespace logging
}  // namespace Manganese
