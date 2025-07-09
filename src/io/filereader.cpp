/**
 * @file filereader.cpp
 * @brief This file contains the implementation of the FileReader class
 */

#include <global_macros.h>
#include <io/filereader.h>
#include <io/logging.h>

#include <algorithm>
#include <cstring>  // For memmove
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace Manganese {
namespace io {
FileReader::FileReader(const std::string& filename, size_t bufferCapacity_)
    : position(0), line(1), column(1), bufferCapacity(bufferCapacity_) {
    fileStream.open(filename, std::ios::in);
    if (!fileStream.is_open()) {
        logging::logCritical(std::format("Could not open file {}", filename));
        this->hasCriticalError_ = true;
        return;
    }
    buffer = std::make_unique<char[]>(bufferCapacity_ + 1);                               // +1 for null terminator
    fileStream.read(buffer.get(), static_cast<std::streamsize>(bufferCapacity_));
    bufferSize = static_cast<size_t>(std::max<std::streamsize>(0, fileStream.gcount()));

    if (bufferSize == 0) {
        logging::logError(
            std::format("File {} is empty or could not be read", filename));
        this->hasCriticalError_ = true;
        return;
    }
    buffer[bufferSize] = '\0';  // Null-terminate the buffer since peekChar will rely on this to determine EOF
}

void FileReader::refillBuffer() {
    // Save any remaining data that hasn't been processed yet
    const size_t unreadBytes = bufferSize - position;
    if (unreadBytes > 0) {
        // Move remaining data to beginning of buffer, handling overlap
        memmove(buffer.get(), buffer.get() + position, unreadBytes);
    }

    const size_t remainingCapacity = bufferCapacity - unreadBytes;
    // Read more data into the buffer
    fileStream.read(buffer.get() + unreadBytes, static_cast<std::streamsize>(remainingCapacity));
    const size_t bytesRead = static_cast<size_t>(fileStream.gcount());

    bufferSize = unreadBytes + bytesRead;  // Update buffer size to include new data
    position = 0;                          // We moved any remaining data to the front, so reset position to 0
    // Always null-terminate since peeking/consuming determines EOF based on null terminator
    buffer[bufferSize] = '\0';
}

char FileReader::peekChar(size_t offset) {
    // Check if we need more data
    if (position + offset >= bufferSize) {
        refillBuffer();

        // If still out of bounds after refill, return EOF
        if (position + offset >= bufferSize) {
            return EOF_CHAR;
        }
    }
    return buffer[position + offset];
}

char FileReader::consumeChar() {
    // Ensure buffer has data to consume
    if (position >= bufferSize) {
        refillBuffer();
        if (position >= bufferSize) {
            return EOF_CHAR;
        }
    }

    char c = buffer[position++];
    if (c == '\n') {
        ++line;
        column = 1;
    } else {
        ++column;
    }
    return c;
}

void FileReader::setPosition(size_t newPosition) noexcept {
    if (newPosition >= bufferSize) {
        position = bufferSize;
    } else {
        position = newPosition;
    }
}

size_t FileReader::getPosition() const noexcept {
    return position;
}

size_t FileReader::getLine() const noexcept {
    return line;
}

size_t FileReader::getColumn() const noexcept {
    return column;
}

bool FileReader::done() const noexcept {
    return position >= bufferSize && fileStream.eof();
}

}  // namespace io
}  // namespace Manganese
