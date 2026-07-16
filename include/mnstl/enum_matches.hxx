#ifndef MNSTL_ENUM_MATCHES
#define MNSTL_ENUM_MATCHES 1

#include <type_traits>

namespace mnstl {

template <class Enum, class... Args>
    requires(std::is_enum_v<Enum> && (std::is_same_v<Enum, Args> && ...))
constexpr bool enum_matches(Enum value, Args&&... args) noexcept {
    return ((value == args) || ...);
}

}  // namespace mnstl

#endif  // MNSTL_ENUM_MATCHES