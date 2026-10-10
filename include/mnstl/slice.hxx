#ifndef MANGANESE_INCLUDE_UTILS_SLICE_HPP
#define MANGANESE_INCLUDE_UTILS_SLICE_HPP 1

#include <cstddef>

namespace mnstl {

template <class T>
class Slice {
    const T* data = nullptr;
    std::size_t size = 0;

    [[nodiscard]] const T* begin() const noexcept { return data; }
    [[nodiscard]] const T* end() const noexcept { return data + size; }
    [[nodiscard]] const T& operator[](std::size_t index) const noexcept { return data[index]; }
    [[nodiscard]] bool empty() const noexcept { return size == 0; }
};

}  // namespace mnstl

#endif  // MANGANESE_INCLUDE_UTILS_SLICE_HPP