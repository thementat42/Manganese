/**
 * @file filereader.hpp
 * @brief Defines the FileReader class for buffered file I/O operations.
 *
 * This file contains the FileReader class which implements the Reader interface
 * for buffered file reading operations. A FileReader maintains an internal
 * buffer to minimize disk access, tracks cursor position including line and column
 * information, and provides methods for peeking ahead in the file without advancing
 * the read position.
 *
 * @note This class is not copyable to prevent unintended duplication of file handles and buffer state.
 *
 * @see reader.hpp - The base interface class that FileReader implements
 */

#ifndef MANGANESE_INCLUDE_IO_FILEREADER_HPP
#define MANGANESE_INCLUDE_IO_FILEREADER_HPP

#include <core.hpp>
#include <memory>
#include <string>

#include "reader.hpp"

namespace Manganese {
namespace io {
/**
 * @brief A reader that extracts characters from a file, stored in memory, with built-in buffering behaviour
 */
class FileReader : public Reader {
   private:
    size_t _position, _line, _column;
    std::string _source;
    std::FILE* _filePtr;
    size_t _bufferSize;  // How much data is currently in the buffer
    size_t _bufferCapacity;  // How much data the buffer can hold
    constexpr static inline int DEFAULT_BUFFER_CAPCITY = 64 * 1024;  // 64 KiB buffer size
    std::unique_ptr<char[]> _buffer;  // Buffer for file reading

    void refillBuffer();

   public:
    FileReader() = default;
    FileReader(const std::string& filename, size_t bufferCapacity = DEFAULT_BUFFER_CAPCITY);
    ~FileReader() noexcept override {
        if (_filePtr) { std::fclose(_filePtr); }
    }

    char peekChar(size_t offset = 0) noexcept override;
    [[nodiscard]] char consumeChar() noexcept override;

    void setPosition(size_t newPosition) noexcept override {
        while (_position < newPosition && !done()) { DISCARD(consumeChar()); }
    }
    constexpr size_t getPosition() const noexcept override { return _position; }
    constexpr size_t getLine() const noexcept override { return _line; }
    constexpr size_t getColumn() const noexcept override { return _column; }

    constexpr bool done() const noexcept override { return _position >= _bufferSize && std::feof(_filePtr); }
};
}  // namespace io
}  // namespace Manganese
#endif  // MANGANESE_INCLUDE_IO_FILEREADER_HPP