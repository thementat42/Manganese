#ifndef MANGANESE_INCLUDE_UTILS_TARGET_INFO_HPP
#define MANGANESE_INCLUDE_UTILS_TARGET_INFO_HPP 1

#include <cstddef>
#include <string_view>

namespace Manganese {
struct TargetInfo {
    std::size_t pointerSize;
    std::size_t pointerAlignment;

    static TargetInfo fromTriple(std::string_view triple);
    static TargetInfo fromHostTriple();
};
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_UTILS_TARGET_INFO_HPP