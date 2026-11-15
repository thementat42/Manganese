#ifndef TYPE_NAMES_HPP
#define TYPE_NAMES_HPP

#include <core.hpp>
#include <string_view>

namespace Manganese {

#define PRIMITIVE(type, name) constexpr inline std::string_view name##_str = MN_STRINGIFY(name);

#include <ast/primitives.def>

#undef PRIMITIVE

constexpr inline std::string_view int_str = "int";
constexpr inline std::string_view uint_str = "uint";
constexpr inline std::string_view float_str = "float";

}  // namespace Manganese

#endif  // TYPE_NAMES_HPP