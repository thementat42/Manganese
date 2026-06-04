/**
 * @file stringreader.hpp
 * @brief This file contains the definition of the StringReader class
 */

//! Note: This is a temporary file, mainly for testing purposes.
//! Once the compiler is complete, this will be removed, and only the FileReader will be used.

#ifndef MANGANESE_INCLUDE_IO_STRING_READER_HPP
#define MANGANESE_INCLUDE_IO_STRING_READER_HPP
#include <core.hpp>
#include <string>

#include "reader.hpp"

namespace Manganese {
namespace io {

/**
 * @brief A simple reader that extracts characters from a string, stored entirely in memory
 */
class StringReader : public Reader {
   private:
    size_t _position, _line, _column;
    std::string _source;

   public:
    StringReader() = default;
    StringReader(const std::string& source) : _position(0), _line(1), _column(1), _source(source) {}
    ~StringReader() noexcept = default;

    void setPosition(size_t newPosition) noexcept override {
        while (_position < newPosition && !done()) { DISCARD(consumeChar()); }
    }
    constexpr size_t getPosition() const noexcept override { return _position; }
    constexpr size_t getLine() const noexcept override { return _line; }
    constexpr size_t getColumn() const noexcept override { return _column; }

    constexpr bool done() const noexcept override { return _position >= _source.length(); }

    char peekChar(size_t offset = 0) noexcept override {
        return (_position + offset >= _source.length()) ? Reader::EOF_CHAR : _source[_position + offset];
    }
    [[nodiscard]] char consumeChar() noexcept override {
        if (_position >= _source.length()) { return Reader::EOF_CHAR; }
        const char c = _source[_position++];
        _line += (c == '\n') ? 1 : 0;
        _column = (c == '\n') ? 1 : _column + 1;
        return c;
    }
};
}  // namespace io
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_IO_STRING_READER_HPP