#ifndef MANGANESE_INCLUDE_IO_FILEREADER_HPP
#define MANGANESE_INCLUDE_IO_FILEREADER_HPP

#include <algorithm>
#include <core.hpp>
#include <cstddef>
#include <cstdio>
#include <io/logging.hpp>
#include <io/reader.hpp>
#include <mio/mio.hpp>
#include <string>
#include <system_error>

namespace Manganese::io {

class FileReader final : public Reader {
    mio::mmap_source _mmap;
    const char* _data = nullptr;
    std::size_t _size = 0;
    std::size_t _position = 0, _line = 1, _column = 1;

   public:
    FileReader(const std::string& filename, std::error_code& ec) {
        std::error_code error;
        _mmap = mio::make_mmap_source(filename, error);

        if (error) { return; }

        _data = _mmap.data();
        _size = _mmap.size();
        if (_size == 0) { logging::logCritical(0, 0, "File '{}' was empty", filename); }
        ec = error;
    }

    [[nodiscard]] char peekChar(std::size_t offset = 0) noexcept override {
        if (_position + offset >= _size) { return '\0'; }
        return _data[_position + offset];
    }

    [[nodiscard]] char consumeChar() noexcept override {
        if (_position >= _size) { return '\0'; }
        const char c = _data[_position++];
        if (c == '\n') {
            ++_line;
            _column = 1;
        } else {
            ++_column;
        }
        return c;
    }

    void setPosition(std::size_t newPosition) noexcept override {
        newPosition = std::min(newPosition, _size);
        if (newPosition < _position) {
            _position = 0;
            _line = 1;
            _column = 1;
        }

        while (_position < newPosition) {
            const char c = _data[_position++];
            if (c == '\n') {
                ++_line;
                _column = 1;
            } else {
                ++_column;
            }
        }
    };

    [[nodiscard]] std::size_t getPosition() const noexcept override { return _position; }
    [[nodiscard]] std::size_t getLine() const noexcept override { return _line; }
    [[nodiscard]] std::size_t getColumn() const noexcept override { return _column; }
    [[nodiscard]] bool done() const noexcept override { return _position >= _size; }

    [[nodiscard]] std::string_view getView(std::size_t start, std::size_t length) const noexcept override {
        if (start + length > _size) { return {}; }
        return {_mmap.data() + start, length};
    }
};

}  // namespace Manganese::io
#endif  // MANGANESE_INCLUDE_IO_FILEREADER_HPP