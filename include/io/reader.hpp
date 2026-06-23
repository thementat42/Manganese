#ifndef MANGANESE_INCLUDE_IO_READER_HPP
#define MANGANESE_INCLUDE_IO_READER_HPP

#include <cstddef>
#include <core.hpp>


namespace Manganese {
namespace io {

/**
 * @brief An abstract base class for reading characters from a source
 */
class Reader {

   public:
    Reader() = default;
    virtual ~Reader() noexcept = default;

    // Any reader should be the only thing reading its source
    // This is especially important for reading from a file, to prevent double closing or reading from a closed file
    Reader(const Reader&) = delete;
    Reader& operator=(const Reader&) = delete;

    Reader(Reader&&) noexcept = delete;
    Reader& operator=(Reader&&) noexcept = delete;

    /**
     * @brief Look at the next character without consuming it
     * @param offset The number of characters to look ahead (default is 0)
     * @return The character at the specified offset
     */
    virtual char peekChar(size_t offset = 0) noexcept = 0;

    /**
     * @brief Consume the next character
     * @return The character that was consumed
     */
    [[nodiscard]] virtual char consumeChar() noexcept = 0;
    /**
     * @brief Move the reader forward to a new position
     * @param newPosition The new position to move to
     */
    virtual void setPosition(size_t newPosition) noexcept = 0;

    /**
     * @brief Get the current position of the reader
     * @return The current position of the reader
     */
    constexpr virtual size_t getPosition() const noexcept = 0;

    /**
     * @brief Get the current line of the reader
     * @return The current line of the reader
     */
    constexpr virtual size_t getLine() const noexcept = 0;

    /**
     * @brief Get the current column of the reader
     * @return The current column of the reader
     */
    constexpr virtual size_t getColumn() const noexcept = 0;

    /**
     * @brief Returns true if the reader has reached the end of the input
     * @return True if the reader is done, false otherwise
     */
    constexpr virtual bool done() const noexcept = 0;
};
}  // namespace io
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_IO_READER_HPP