/**
 * @file filereader.h
 * @brief This file contains the definition of the FileReader class
 */

#ifndef FILEREADER_H
#define FILEREADER_H

#include "../../global_macros.h"
#include "reader.h"
#include <string>
#include <fstream>
#include <memory>

MANG_BEGIN
namespace io {
class FileReader : public Reader {

    private:
    size_t position, line, column;
    std::string source;
    std::ifstream fileStream;
    size_t bufferSize; // How much data is currently in the buffer 
    size_t bufferCapacity; // How much data the buffer can hold
    static constexpr int DEFAULT_BUFFER_CAPCITY = 2 << 20;  // ~2MB buffer size
    std::unique_ptr<char[]> buffer; // Buffer for file reading


   public:
    FileReader() = default;
    FileReader(const FileReader& other) = delete; // Disable copy constructor
    FileReader(const std::string& filename, size_t bufferCapacity = DEFAULT_BUFFER_CAPCITY);
    ~FileReader() override;

    void refillBuffer(); // Fill the buffer with data from the file
    char peekChar(size_t offset = 0) override;
    [[nodiscard]] char consumeChar() override;
    void setPosition(size_t newPosition) override;
    size_t getPosition() const override;
    size_t getLine() const override;
    size_t getColumn() const override;
    bool done() const override;

    FileReader& operator=(const FileReader& other) = delete; // Disable copy assignment operator
};
} // namespace io
MANG_END
#endif // FILEREADER_H