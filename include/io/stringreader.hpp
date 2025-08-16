/**
 * @file stringreader.hpp
 * @brief This file contains the definition of the StringReader class
 */

//! Note: This is a temporary file, mainly for testing purposes.
//! Once the compiler is complete, this will be removed, and only the FileReader will be used.

#ifndef MANGANESE_INCLUDE_IO_STRING_READER_HPP
#define MANGANESE_INCLUDE_IO_STRING_READER_HPP
#include <global_macros.hpp>
#include <string>

#include "reader.hpp"

namespace Manganese {
namespace io {

/**
 * @brief A simple reader that extracts characters from a string, stored entirely in memory
 */
class StringReader : public Reader {
   private:
    size_t position, line, column;
    std::string source;

   public:
    StringReader() = default;
    StringReader(const std::string& source_) : position(0), line(1), column(1), source(source_) {}
    ~StringReader() noexcept = default;

    constexpr void setPosition(size_t newPosition) noexcept override {
        position = newPosition >= source.length() ? source.length() : newPosition;
    }
    constexpr size_t getPosition() const noexcept override { return position; }
    constexpr size_t getLine() const noexcept override { return line; }
    constexpr size_t getColumn() const noexcept override { return column; }

    constexpr bool done() const noexcept override { return position >= source.length(); }

    char peekChar(size_t offset = 0) noexcept override {
        return (position + offset >= source.length()) ? Reader::EOF_CHAR : source[position + offset];
    }
    [[nodiscard]] char consumeChar() noexcept override {
        if (position >= source.length()) { return Reader::EOF_CHAR; }
        char c = source[position++];
        line += (c == '\n') ? 1 : 0;
        column = (c == '\n') ? 1 : column + 1;
        return c;
    }
};
}  // namespace io
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_IO_STRING_READER_HPP