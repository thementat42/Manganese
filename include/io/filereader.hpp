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
    FileReader(const std::string& filename) {
        std::error_code error;
        _mmap = mio::make_mmap_source(filename, error);

        if (error) { logging::logCritical(0, 0, "Could not memory-map file '{}' ({})", filename, error.message()); }

        _data = _mmap.data();
        _size = _mmap.size();
        if (_size == 0) { logging::logCritical(0, 0, "File '{}' was empty", filename); }
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
};

}  // namespace Manganese::io
#endif  // MANGANESE_INCLUDE_IO_FILEREADER_HPP

/*
class FileReader : public Reader {
   private:
    struct FileDeleter {
        void operator()(std::FILE* fp) const {
            if (fp != nullptr) { std::fclose(fp); }
        }
    };

    std::size_t _position = 0, _line = 1, _column = 1;
    std::unique_ptr<std::FILE, FileDeleter> _filePtr;
    std::size_t _bufferSize;
    std::size_t _bufferCapacity;
    constexpr static inline std::size_t DEFAULT_BUFFER_CAPACITY = 64ULL * 1024;
    std::unique_ptr<char[]> _buffer;

    void refillBuffer();

   public:
    FileReader() = default;
    explicit FileReader(const std::string& filename, std::size_t bufferCapacity = DEFAULT_BUFFER_CAPACITY);
    ~FileReader() noexcept override = default;

    char peekChar(std::size_t offset = 0) noexcept override;
    [[nodiscard]] char consumeChar() noexcept override;

    void setPosition(std::size_t newPosition) noexcept override {
        while (_position < newPosition && !done()) { DISCARD(consumeChar()); }
    }
    [[nodiscard]] std::size_t getPosition() const noexcept override { return _position; }
    [[nodiscard]] std::size_t getLine() const noexcept override { return _line; }
    [[nodiscard]] std::size_t getColumn() const noexcept override { return _column; }

    [[nodiscard]] bool done() const noexcept override {
        return (_position >= _bufferSize) && (std::feof(_filePtr.get()) != 0);
    }
};
*/