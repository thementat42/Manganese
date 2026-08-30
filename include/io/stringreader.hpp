#ifndef MANGANESE_INCLUDE_IO_STRING_READER_HPP
#define MANGANESE_INCLUDE_IO_STRING_READER_HPP

#include <core.hpp>
#include <cstddef>
#include <io/reader.hpp>
#include <string_view>

namespace Manganese::io {

class StringReader : public Reader {
   private:
    std::size_t _position, _line, _column;
    std::string_view _source;

   public:
    StringReader() = default;
    explicit StringReader(std::string_view source) : _position(0), _line(1), _column(1), _source(source) {}
    ~StringReader() noexcept override = default;

    void setPosition(std::size_t newPosition) noexcept override {
        while (_position < newPosition && !done()) { DISCARD(consumeChar()); }
    }
    std::size_t getPosition() const noexcept override { return _position; }
    std::size_t getLine() const noexcept override { return _line; }
    std::size_t getColumn() const noexcept override { return _column; }

    bool done() const noexcept override { return _position >= _source.length(); }

    char peekChar(std::size_t offset = 0) noexcept override {
        return (_position + offset >= _source.length()) ? '\0' : _source[_position + offset];
    }
    [[nodiscard]] char consumeChar() noexcept override {
        if (_position >= _source.length()) { return '\0'; }
        const char c = _source[_position++];
        _line += (c == '\n') ? 1 : 0;
        _column = (c == '\n') ? 1 : _column + 1;
        return c;
    }
};
}  // namespace Manganese::io

#endif  // MANGANESE_INCLUDE_IO_STRING_READER_HPP