#ifndef MANGANESE_INCLUDE_UTILS_NAMED_PARAMETER_HPP
#define MANGANESE_INCLUDE_UTILS_NAMED_PARAMETER_HPP 1

namespace Manganese::utils {

template <class T, class>
struct NamedParameter {
    T value;
    constexpr explicit NamedParameter(T val) : value(val) {}

    constexpr T get() const { return value; }
};

}  // namespace Manganese::utils

#endif  // MANGANESE_INCLUDE_UTILS_NAMED_PARAMETER_HPP