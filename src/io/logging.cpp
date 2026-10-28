#include <io/logging.hpp>
#include <iostream>
#include <mutex>
#include <stdexcept>

namespace Manganese::logging {

namespace {
// Mutex protects against simultaneous writes to std::cout
std::mutex logMutex;
}  // namespace

void writeToStderr(std::string_view prefixColor, std::string_view label, std::string_view message, std::size_t line,
                   std::size_t col) {
    std::string out
        = std::format("{}{}{} {} (line {}, column {})\n", prefixColor, label, message, ansi::RESET, line, col);

    std::lock_guard<std::mutex> lock(logMutex);
    std::cout.write(out.data(), static_cast<std::streamsize>(out.size()));
}

void logInternalRaw(LogLevel level, std::string_view message) {
    std::string out;
    switch (level) {
        case LogLevel::Info: out = std::format("{}[Internal Info] {}{}\n", ansi::BLUE, message, ansi::RESET); break;
        case LogLevel::Warning:
            out = std::format("{}[Internal Warning] {}{}\n", ansi::YELLOW, message, ansi::RESET);
            break;
        case LogLevel::Error: out = std::format("{}[Internal Error] {}{}\n", ansi::RED, message, ansi::RESET); break;
        case LogLevel::Critical:
            out = std::format("{}[Internal Critical Error] {}{}\n", ansi::RED, message, ansi::RESET);
            break;
    }

    std::lock_guard<std::mutex> lock(logMutex);
    std::cout.write(out.data(), static_cast<std::streamsize>(out.size()));
}

void logRaw(LogLevel level, std::size_t line, std::size_t col, std::string_view message) {
    switch (level) {
        case LogLevel::Info: return;
        case LogLevel::Warning: writeToStderr(ansi::YELLOW, "Warning: ", message, line, col); break;
        case LogLevel::Error: writeToStderr(ansi::RED, "Error: ", message, line, col); break;
        case LogLevel::Critical: {
            const std::string critMessage
                = std::format("{}Critical error: {} Compilation aborted.{}", ansi::CRITICAL, message, ansi::RESET);
            writeToStderr("", "", critMessage, line, col);
            throw std::runtime_error{std::string(message)};
        }
    }
}

}  // namespace Manganese::logging