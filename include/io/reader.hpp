/**
 * @file reader.h
 * @brief This file contains the definition of the virtual Reader class
 *
 * Readers are responsible for taking in raw source code and passing individual characters onto the lexer.
 * The reader abstracts away how this is done so the lexer can just call peekChar and getChar
 */

#ifndef MANGANESE_INCLUDE_IO_READER_HPP
#define MANGANESE_INCLUDE_IO_READER_HPP

#include <global_macros.hpp>

#include <cstddef>
#include <string>

namespace Manganese {
namespace io {

/**
 * @brief An abstract base class for reading characters from a source
 */
class Reader {
   protected:
    size_t position, line, column;
    std::string source;
    bool hasCriticalError_ = false;

   public:
    static constexpr char EOF_CHAR = '\0';
    Reader() = default;
    virtual ~Reader() noexcept = default;

    bool hasCriticalError() const noexcept { return hasCriticalError_; }

    /**
     * @brief Look at the next character without consuming it
     * @param offset The number of characters to look ahead (default is 0)
     * @return The character at the specified offset
     */
    virtual char peekChar(size_t offset = 0) = 0;

    /**
     * @brief Consume the next character
     * @return The character that was consumed
     */
    [[nodiscard]] virtual char consumeChar() = 0;
    /**
     * @brief Move the reader forward to a new position
     * @param newPosition The new position to move to
     */
    virtual void setPosition(size_t newPosition) = 0;

    /**
     * @brief Get the current position of the reader
     * @return The current position of the reader
     */
    virtual size_t getPosition() const noexcept = 0;

    /**
     * @brief Get the current line of the reader
     * @return The current line of the reader
     */
    virtual size_t getLine() const noexcept = 0;

    /**
     * @brief Get the current column of the reader
     * @return The current column of the reader
     */
    virtual size_t getColumn() const noexcept = 0;

    /**
     * @brief Returns true if the reader has reached the end of the input
     * @return True if the reader is done, false otherwise
     */
    virtual bool done() const = 0;
};
}  // namespace io
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_IO_READER_HPP