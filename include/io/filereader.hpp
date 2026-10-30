#ifndef MANGANESE_INCLUDE_IO_FILEREADER_HPP
#define MANGANESE_INCLUDE_IO_FILEREADER_HPP

#include <core.hpp>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <io/logging.hpp>
#include <io/reader.hpp>
#include <mio/mn_mio.hpp>
#include <string_view>
#include <system_error>


namespace Manganese::io {

class FileReader final : public Reader {
    mio::basic_mmap<char> _mmap;
    std::size_t _position = 0, _line = 1, _column = 1;

    char _advance(char c) noexcept {
        if (c == '\n') {
            ++_line;
            _column = 1;
        } else {
            ++_column;
        }
        return c;
    }

   public:
    FileReader(const std::filesystem::path& filename, std::error_code& ec) {
        _mmap = mio::make_mmap<mio::basic_mmap<char>>(filename, 0, mio::map_entire_file, ec);

        if (ec) { return; }

        if (_mmap.size() == 0) { logging::logCritical(0, 0, "File '{}' was empty", filename.string()); }
    }

    [[nodiscard]] char peekChar(std::size_t offset = 0) noexcept override {
        if (_position + offset >= _mmap.size()) { return '\0'; }
        return _mmap[_position + offset];
    }

    [[nodiscard]] char consumeChar() noexcept override {
        if (_position >= _mmap.size()) { return '\0'; }
        return _advance(_mmap[_position++]);
    }

    void setPosition(std::size_t newPosition) noexcept override {
        newPosition = newPosition < _mmap.size() ? newPosition : _mmap.size();
        if (newPosition < _position) {
            _position = 0;
            _line = 1;
            _column = 1;
        }

        while (_position < newPosition) { _advance(_mmap[_position++]); }
    }

    [[nodiscard]] std::size_t getPosition() const noexcept override { return _position; }
    [[nodiscard]] std::size_t getLine() const noexcept override { return _line; }
    [[nodiscard]] std::size_t getColumn() const noexcept override { return _column; }
    [[nodiscard]] bool done() const noexcept override { return _position >= _mmap.size(); }

    [[nodiscard]] std::string_view getView(std::size_t start, std::size_t length) const noexcept override {
        if (start + length > _mmap.size()) { return {}; }
        return {_mmap.data() + start, length};
    }
};

}  // namespace Manganese::io
#endif  // MANGANESE_INCLUDE_IO_FILEREADER_HPP