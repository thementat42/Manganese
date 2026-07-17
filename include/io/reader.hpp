#ifndef MANGANESE_INCLUDE_IO_READER_HPP
#define MANGANESE_INCLUDE_IO_READER_HPP

#include <core.hpp>
#include <cstddef>

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

    virtual char peekChar(size_t offset = 0) noexcept = 0;
    [[nodiscard]] virtual char consumeChar() noexcept = 0;
    virtual void setPosition(size_t newPosition) noexcept = 0;
    constexpr virtual size_t getPosition() const noexcept = 0;
    constexpr virtual size_t getLine() const noexcept = 0;
    constexpr virtual size_t getColumn() const noexcept = 0;
    constexpr virtual bool done() const noexcept = 0;
};
}  // namespace io
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_IO_READER_HPP