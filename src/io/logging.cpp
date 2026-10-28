#include <io/logging.hpp>
#include <iostream>
#include <mutex>
#include <stdexcept>

namespace Manganese::logging {

namespace {
// Mutex prevents simultaneous writes to std::cout
std::mutex logMutex;

// Thread-local pointer pointing to the active buffer for this thread (if any)
inline thread_local std::vector<LogRecord>* activeThreadBuffer = nullptr;
} // namespace

ThreadLogBufferGuard::ThreadLogBufferGuard() {
    oldBuffer = activeThreadBuffer;
    activeThreadBuffer = &records;
}

ThreadLogBufferGuard::~ThreadLogBufferGuard() {
    activeThreadBuffer = oldBuffer;
}

void writeToStderr(std::string_view prefixColor, std::string_view label, std::string_view message,
                   std::size_t line, std::size_t col) {
    std::string out
        = std::format("{}{}{} {} (line {}, column {})\n", prefixColor, label, message, ansi::RESET, line, col);

    std::lock_guard<std::mutex> lock(logMutex);
    // Using .write() bypasses operator<< safely under lock
    std::cout.write(out.data(), static_cast<std::streamsize>(out.size()));
}

void logInternalRaw(LogLevel level, std::string_view message) {
    std::string out;
    switch (level) {
        case LogLevel::Info: 
            out = std::format("{}[Internal Info] {}{}\n", ansi::BLUE, message, ansi::RESET); 
            break;
        case LogLevel::Warning:
            out = std::format("{}[Internal Warning] {}{}\n", ansi::YELLOW, message, ansi::RESET);
            break;
        case LogLevel::Error: 
            out = std::format("{}[Internal Error] {}{}\n", ansi::RED, message, ansi::RESET); 
            break;
        case LogLevel::Critical:
            out = std::format("{}[Internal Critical Error] {}{}\n", ansi::RED, message, ansi::RESET);
            break;
    }

    std::lock_guard<std::mutex> lock(logMutex);
    std::cout.write(out.data(), static_cast<std::streamsize>(out.size()));
}

void logRaw(LogLevel level, std::size_t line, std::size_t col, std::string_view message) {
    if (level == LogLevel::Info) { return; }

    if (activeThreadBuffer != nullptr) {
        activeThreadBuffer->push_back(LogRecord{.level=level, .line=line, .col=col, .message=std::string(message)});
        if (level == LogLevel::Critical) {
            throw std::runtime_error{std::string(message)};
        }
        return;
    }

    // Otherwise, execute normal immediate printing (thread-safe)
    switch (level) {
        case LogLevel::Info: 
            return;
        case LogLevel::Warning: 
            writeToStderr(ansi::YELLOW, "Warning: ", message, line, col); 
            break;
        case LogLevel::Error: 
            writeToStderr(ansi::RED, "Error: ", message, line, col); 
            break;
        case LogLevel::Critical: {
            const std::string critMessage = std::format(
                "{}Critical error: {} Compilation aborted.{}", ansi::CRITICAL, message, ansi::RESET
            );
            writeToStderr("", "", critMessage, line, col);
            throw std::runtime_error{std::string(message)};
        }
    }
}

}  // namespace Manganese::logging