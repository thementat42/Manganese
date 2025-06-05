/**
 * @file filereader.h
 * @brief This file contains the definition of the FileReader class
 */

#ifndef FILEREADER_H
#define FILEREADER_H

#include <fstream>
#include <memory>
#include <string>

#include "../../global_macros.h"
#include "reader.h"

namespace manganese {
namespace io {
/**
 * @brief A reader that extracts characters from a file, stored in memory, with built-in buffering behaviour
 */
class FileReader : public Reader {
   private:
    size_t position, line, column;
    std::string source;
    std::ifstream fileStream;
    size_t bufferSize;                                      // How much data is currently in the buffer
    size_t bufferCapacity;                                  // How much data the buffer can hold
    static constexpr int DEFAULT_BUFFER_CAPCITY = 2 << 20;  // ~2MB buffer size
    std::unique_ptr<char[]> buffer;                         // Buffer for file reading

    /**
     * @brief Reads more data from the file (on disc) into the buffer (in memory)
     * @details Handles cases where a lexeme is split across two buffer reads (mainly for lookaheads via peekChar)
     *
     */
    void refillBuffer();

   public:
    FileReader() = default;
    FileReader(const FileReader& other) = delete;  // Disable copy constructor
    FileReader(const std::string& filename, size_t bufferCapacity = DEFAULT_BUFFER_CAPCITY);
    ~FileReader() noexcept = default;

    char peekChar(size_t offset = 0) noexcept override;
    [[nodiscard]] char consumeChar() noexcept override;

    void setPosition(size_t newPosition) noexcept override;
    size_t getPosition() const noexcept override;
    size_t getLine() const noexcept override;
    size_t getColumn() const noexcept override;

    bool done() const override;

    FileReader& operator=(const FileReader& other) = delete;  // Disable copy assignment operator
};
}  // namespace io
}  // namespace manganese
#endif  // FILEREADER_H