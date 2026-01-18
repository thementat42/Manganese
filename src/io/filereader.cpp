/**
 * @file filereader.cpp
 * @brief This file contains the implementation of the FileReader class
 */

#include <global_macros.hpp>
#include <io/filereader.hpp>
#include <io/logging.hpp>

#include <cstring>  // For memmove
#include <format>
#include <memory>
#include <string>
#include <cstring>
#include <cstdio>

namespace Manganese {
namespace io {

FileReader::FileReader(const std::string& filename, size_t bufferCapacity_) :
    position(0), line(1), column(1), filePtr(nullptr), bufferSize(0), bufferCapacity(bufferCapacity_) {
    filePtr = std::fopen(filename.c_str(), "r");
    if (!filePtr) {
        logging::logCritical(0, 0, "Could not open file {}", filename);
        this->hasCriticalError_ = true;
        return;
    }
    // FileReader does its own buffering, with extra stuff to support lookaheads
    // fread buffers by default, which is redundant and wastes memory (since the same data is stored in two places)
    // So, disable fread's buffering
    setvbuf(filePtr, nullptr, _IONBF, 0);

    buffer = std::make_unique<char[]>(bufferCapacity + 1);  // +1 for a null terminator
    bufferSize = std::fread(buffer.get(), sizeof(char), bufferCapacity_, filePtr);  // initial read

    if (bufferSize == 0) {
        logging::logError(0, 0, "File {} is empty or could not be read", filename);
        this->hasCriticalError_ = true;
        return;
    }
    buffer[bufferSize] = Reader::EOF_CHAR;  // Null-terminate the buffer since peekChar will rely on this to determine EOF
}

void FileReader::refillBuffer() {
    const size_t unreadBytes = bufferSize - position;
    if (unreadBytes) {
        // Move any unread data to the beginning of the buffer
        // This way, if we are near the end of a chunk and try to read into the next chunk
        // unread data can still be read later
        memmove(buffer.get(), buffer.get() + position, unreadBytes);
    }
    const size_t remainingCapacity = bufferCapacity - unreadBytes;
    size_t bytesRead = std::fread(buffer.get() + unreadBytes, sizeof(char), remainingCapacity, filePtr);

    bufferSize = unreadBytes + bytesRead;
    position = 0;  // We moved any remaining data to the front, so reset position to 0
    // Always null-terminate since peeking/consuming determines EOF based on null terminator
    buffer[bufferSize] = Reader::EOF_CHAR;
}

char FileReader::peekChar(size_t offset) noexcept {
    if (position + offset >= bufferSize) {
        refillBuffer();

        // If still out of bounds after refill, we're done reading the file
        if (position + offset >= bufferSize) { return Reader::EOF_CHAR; }
    }
    return buffer[position + offset];
}

char FileReader::consumeChar() noexcept {
    // Ensure buffer has data to consume
    if (position >= bufferSize) {
        refillBuffer();
        if (position >= bufferSize) { return Reader::EOF_CHAR; }
    }

    char c = buffer[position++];
    line += (c == '\n') ? 1 : 0;
    column = (c == '\n') ? 1 : column + 1;
    return c;
}

}  // namespace io
}  // namespace Manganese
