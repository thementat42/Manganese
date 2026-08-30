#ifndef MANGANESE_INCLUDE_UTILS_RESULT_HPP
#define MANGANESE_INCLUDE_UTILS_RESULT_HPP 1

namespace Manganese {

/**
 * A way to consistently indicate success or failure on a return
 * instead of a per-function convention
 */
enum class [[nodiscard]] Result : bool {
    Failure,
    Success
};

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_UTILS_RESULT_HPP