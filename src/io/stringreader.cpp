/**
 * @file stringreader.cpp
 * @brief This file contains the implementation of the StringReader class
 */

#include <global_macros.hpp>
#include <io/stringreader.hpp>

#include <string>

namespace Manganese {
namespace io {

StringReader::StringReader(const std::string& _source) : position(0), line(1), column(1), source(_source) {}

char StringReader::peekChar(size_t offset) noexcept {
    if (position + offset >= source.length()) { return EOF_CHAR; }
    return source[position + offset];
}

char StringReader::consumeChar() noexcept {
    if (position >= source.length()) { return EOF_CHAR; }
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

}  // namespace io
}  // namespace Manganese
