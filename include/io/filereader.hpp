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

#include <fstream>
#include <global_macros.hpp>
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
    size_t position, line, column;
    std::string source;
    // std::ifstream fileStream;
    std::FILE* filePtr;
    size_t bufferSize;  // How much data is currently in the buffer
    size_t bufferCapacity;  // How much data the buffer can hold
    static constexpr int DEFAULT_BUFFER_CAPCITY = 2 << 20;  // ~2MB buffer size
    std::unique_ptr<char[]> buffer;  // Buffer for file reading

    /**
     * @brief Reads more data from the file (on disc) into the buffer (in memory)
     * @details Handles cases where a lexeme is split across two buffer reads (mainly for lookaheads via peekChar)
     *
     */
    void refillBuffer();

   public:
    FileReader() = default;
    FileReader(const std::string& filename, size_t bufferCapacity = DEFAULT_BUFFER_CAPCITY);
    ~FileReader() noexcept override {
        if (filePtr) {
            std::fclose(filePtr);
        }
    }

    // To avoid any issues with multiple file handlers being opened on the same file,
    // the copy constructor and assignment operator are deleted.
    FileReader(const FileReader&) = delete;
    FileReader& operator=(const FileReader&) = delete;

    char peekChar(size_t offset = 0) noexcept override;
    [[nodiscard]] char consumeChar() noexcept override;

    constexpr void setPosition(size_t newPosition) noexcept override {
        position = newPosition >= bufferSize ? bufferSize : newPosition;
    }
    constexpr size_t getPosition() const noexcept override { return position; }
    constexpr size_t getLine() const noexcept override { return line; }
    constexpr size_t getColumn() const noexcept override { return column; }

    // constexpr bool done() const noexcept override { return position >= bufferSize && filestream.eof(); }
    constexpr bool done() const noexcept override { return position >= bufferSize && std::feof(filePtr); }
    
};
}  // namespace io
}  // namespace Manganese
#endif  // MANGANESE_INCLUDE_IO_FILEREADER_HPP