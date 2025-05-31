/**
 * @file stringreader.h
 * @brief This file contains the definition of the StringReader class
 */

//! Note: This is a temporary file, mainly for testing purposes.
//! Once the compiler is complete, this will be removed, and only the FileReader will be used.

#ifndef STRING_READER_H
#define STRING_READER_H
#include <string>

#include "../../global_macros.h"
#include "reader.h"

namespace manganese {
namespace io {

/**
 * @brief A simple reader that extracts characters from a string, stored entirely in memory
 */
class StringReader : public Reader {
   private:
    size_t position, line, column;
    std::string source;

   public:
    StringReader() = default;
    StringReader(const std::string& source);
    StringReader(const StringReader& other) = delete;  // Disable copy constructor
    ~StringReader() = default;

    char peekChar(size_t offset = 0) override;
    [[nodiscard]] char consumeChar() override;

    void setPosition(size_t newPosition) override;
    size_t getPosition() const override;
    size_t getLine() const override;
    size_t getColumn() const override;

    bool done() const override;

    StringReader& operator=(const StringReader& other) = delete;  // Disable copy assignment operator
};
}  // namespace io
}  // namespace manganese

#endif  // STRING_READER_H