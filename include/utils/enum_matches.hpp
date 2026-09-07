#ifndef MANGANESE_INCLUDE_UTILS_ENUM_MATCHES
#define MANGANESE_INCLUDE_UTILS_ENUM_MATCHES 1

#include <type_traits>

namespace Manganese::utils {

template <class Enum, class... Args>
    requires(std::is_enum_v<Enum> && (std::is_same_v<Enum, Args> && ...))
constexpr bool enum_matches(Enum value, Args... args) noexcept {
    return ((value == args) || ...);
}

}  // namespace Manganese::utils

#endif  // MANGANESE_INCLUDE_UTILS_ENUM_MATCHES