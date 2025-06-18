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
    // TODO: Add call location
    if (level == LogLevel::Critical) {
        UNREACHABLE("Critical error encountered");
    }
}

void logUser(const std::string& message, LogLevel level, size_t line, size_t col) {
    // TODO: Add file name
    switch (level) {
        case LogLevel::Info:
            break;  // No user info
        case LogLevel::Warning:
            std::cout << "\033[33mWarning: " << message << "\033[0m\n";
            break;
        case LogLevel::Error:
            std::cerr << "\033[31mError: " << message << "\033[0m\n";
            break;
        case LogLevel::Critical:
            std::cerr << "\033[91;1mCritical error: " << message << "\n Compilation aborted." << "\033[0m\n";
            break;
    }
    std::cerr << "(line " << line << ", column " << col << ")\n";
    if (level == LogLevel::Critical) {
        throw std::runtime_error(message);
    }
}

void logUser(std::initializer_list<std::string> messages, LogLevel level, size_t line, size_t col) {
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
    if (level == LogLevel::Critical) {
        std::cerr << "Compilation aborted.\n";
        throw std::runtime_error("Critical Error encountered");
    }
}
}  // namespace logging
}  // namespace Manganese