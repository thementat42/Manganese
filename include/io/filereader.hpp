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

#include <global_macros.hpp>

#include <fstream>
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
    std::ifstream fileStream;
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
    ~FileReader() noexcept = default;

    FileReader(const FileReader&) = delete;
    FileReader& operator=(const FileReader&) = delete;

    char peekChar(size_t offset = 0) override;
    [[nodiscard]] char consumeChar() override;

    void setPosition(size_t newPosition) noexcept override;
    size_t getPosition() const noexcept override;
    size_t getLine() const noexcept override;
    size_t getColumn() const noexcept override;

    bool done() const noexcept override;
};
}  // namespace io
}  // namespace Manganese
#endif  // MANGANESE_INCLUDE_IO_FILEREADER_HPP