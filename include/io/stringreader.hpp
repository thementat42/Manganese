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
    StringReader(const std::string& source);
    ~StringReader() noexcept = default;

    StringReader(const StringReader&) = delete;
    StringReader& operator=(const StringReader&) = delete;

    char peekChar(size_t offset = 0) noexcept override;
    [[nodiscard]] char consumeChar() noexcept override;

    constexpr void setPosition(size_t newPosition) noexcept override {
        position = newPosition >= source.length() ? source.length() : newPosition;
    }
    constexpr size_t getPosition() const noexcept override { return position; }
    constexpr size_t getLine() const noexcept override { return line; }
    constexpr size_t getColumn() const noexcept override { return column; }

    constexpr bool done() const noexcept override { return position >= source.length(); }
};
}  // namespace io
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_IO_STRING_READER_HPP