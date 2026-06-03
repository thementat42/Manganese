/**
 * @file filereader.cpp
 * @brief This file contains the implementation of the FileReader class
 */

#include <core.hpp>
#include <cstdio>
#include <cstring>  // For memmove
#include <cstring>
#include <format>
#include <io/filereader.hpp>
#include <io/logging.hpp>
#include <memory>
#include <stdexcept>
#include <string>


namespace Manganese {
namespace io {

FileReader::FileReader(const std::string& filename, size_t bufferCapacity) :
    _position(0), _line(1), _column(1), _filePtr(nullptr), _bufferSize(0), _bufferCapacity(bufferCapacity) {
    _filePtr = std::fopen(filename.c_str(), "r");
    if (!_filePtr) {
        logging::logCritical(0, 0, "Could not open file {}", filename);
        throw std::runtime_error("Critical error encountered.");  // Note: exception here means hard error and exit
        return;
    }
    // FileReader does its own buffering, with extra stuff to support lookaheads
    // fread buffers by default, which is redundant and wastes memory (since the same data is stored in two places)
    // So, disable fread's buffering
    setvbuf(_filePtr, nullptr, _IONBF, 0);

    _buffer = std::make_unique<char[]>(bufferCapacity + 1);  // +1 for a null terminator
    _bufferSize = std::fread(_buffer.get(), sizeof(char), bufferCapacity, _filePtr);  // initial read

    if (_bufferSize == 0) {
        logging::logError(0, 0, "File {} is empty or could not be read", filename);
        throw std::runtime_error("Critical error encountered.");  // Note: exception here means hard error and exit
        return;
    }
    _buffer[_bufferSize]
        = Reader::EOF_CHAR;  // Null-terminate the buffer since peekChar will rely on this to determine EOF
}

void FileReader::refillBuffer() {
    const size_t unreadBytes = _bufferSize - _position;
    if (unreadBytes) {
        // Move any unread data to the beginning of the buffer
        // This way, if we are near the end of a chunk and try to read into the next chunk
        // unread data can still be read later
        std::memmove(_buffer.get(), _buffer.get() + _position, unreadBytes);
    }
    const size_t remainingCapacity = _bufferCapacity - unreadBytes;
    size_t bytesRead = std::fread(_buffer.get() + unreadBytes, sizeof(char), remainingCapacity, _filePtr);

    _bufferSize = unreadBytes + bytesRead;
    _position = 0;  // We moved any remaining data to the front, so reset position to 0
    // Always null-terminate since peeking/consuming determines EOF based on null terminator
    _buffer[_bufferSize] = Reader::EOF_CHAR;
}

char FileReader::peekChar(size_t offset) noexcept {
    if (_position + offset >= _bufferSize) { refillBuffer(); }
    // If still out of bounds after refill, we're done reading the file
    return (_position + offset >= _bufferSize) ? Reader::EOF_CHAR : _buffer[_position + offset];
}

char FileReader::consumeChar() noexcept {
    // Ensure buffer has data to consume
    if (_position >= _bufferSize) {
        refillBuffer();
        if (_position >= _bufferSize) { return Reader::EOF_CHAR; }
    }

    const char c = _buffer[_position++];
    _line += (c == '\n') ? 1 : 0;
    _column = (c == '\n') ? 1 : _column + 1;
    return c;
}

}  // namespace io
}  // namespace Manganese
