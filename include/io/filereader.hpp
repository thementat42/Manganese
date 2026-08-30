#ifndef MANGANESE_INCLUDE_IO_FILEREADER_HPP
#define MANGANESE_INCLUDE_IO_FILEREADER_HPP

#include <core.hpp>
#include <cstddef>
#include <cstdio>
#include <io/reader.hpp>
#include <memory>
#include <string>

namespace Manganese::io {
/**
 * @brief A reader that extracts characters from a file, stored in memory, with built-in buffering behaviour
 */
class FileReader : public Reader {
   private:
    std::size_t _position, _line, _column;
    std::FILE* _filePtr;
    std::size_t _bufferSize;
    std::size_t _bufferCapacity;
    constexpr static inline int DEFAULT_BUFFER_CAPACITY = 64 * 1024;
    std::unique_ptr<char[]> _buffer;

    void refillBuffer();

   public:
    FileReader() = default;
    explicit FileReader(const std::string& filename, std::size_t bufferCapacity = DEFAULT_BUFFER_CAPACITY);
    ~FileReader() noexcept override {
        if (_filePtr != nullptr) { std::fclose(_filePtr); }
    }

    char peekChar(std::size_t offset = 0) noexcept override;
    [[nodiscard]] char consumeChar() noexcept override;

    void setPosition(std::size_t newPosition) noexcept override {
        while (_position < newPosition && !done()) { DISCARD(consumeChar()); }
    }
    std::size_t getPosition() const noexcept override { return _position; }
    std::size_t getLine() const noexcept override { return _line; }
    std::size_t getColumn() const noexcept override { return _column; }

    bool done() const noexcept override { return (_position >= _bufferSize) && std::feof(_filePtr); }
};
}  // namespace Manganese::io
#endif  // MANGANESE_INCLUDE_IO_FILEREADER_HPP