#ifndef MANGANESE_INCLUDE_UTILS_TARGET_INFO_HPP
#define MANGANESE_INCLUDE_UTILS_TARGET_INFO_HPP 1

#include <cstddef>
#include <string_view>

namespace Manganese::utils {
struct TargetInfo {
    std::size_t pointerSize;
    std::size_t pointerAlignment;

    static TargetInfo fromTriple(std::string_view triple) noexcept;
    static TargetInfo fromHostTriple() noexcept;
};
}  // namespace Manganese::utils

#endif  // MANGANESE_INCLUDE_UTILS_TARGET_INFO_HPP