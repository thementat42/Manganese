/**
 * @file stringreader.cpp
 * @brief This file contains the implementation of the StringReader class
 */

//! Note: This is a temporary file, mainly for testing purposes.
//! Once the compiler is complete, this will be removed, and only the FileReader will be used.

#include <io/stringreader.h>

#include <global_macros.h>

MANGANESE_BEGIN
namespace io {

StringReader::StringReader(const std::string& _source) : position(0), line(1), column(1), source(_source) {}

char StringReader::peekChar(size_t offset) noexcept {
    if (position + offset >= source.length()) {
        return EOF_CHAR;
    }
    return source[position + offset];
}

char StringReader::consumeChar() noexcept {
    if (position >= source.length()) {
        return EOF_CHAR;
    }
    char c = source[position];
    if (c == '\n') {
        ++line;
        column = 1;
    } else {
        ++column;
    }
    ++position;
    return c;
}

void StringReader::setPosition(size_t newPosition) noexcept {
    if (newPosition >= source.length()) {
        position = source.length();
    } else {
        position = newPosition;
    }
}

inline size_t StringReader::getPosition() const noexcept {
    return position;
}

inline size_t StringReader::getLine() const noexcept {
    return line;
}

inline size_t StringReader::getColumn() const noexcept {
    return column;
}

inline bool StringReader::done() const noexcept {
    return position >= source.length();
}

}  // namespace io
MANGANESE_END