#ifndef TYPE_NAMES_HPP
#define TYPE_NAMES_HPP

#include <string_view>

namespace Manganese {

constexpr inline std::string_view int8_str = "int8";
constexpr inline std::string_view int16_str = "int16";
constexpr inline std::string_view int32_str = "int32";
constexpr inline std::string_view int64_str = "int64";
constexpr inline std::string_view int128_str = "int128";
constexpr inline std::string_view uint8_str = "uint8";
constexpr inline std::string_view uint16_str = "uint16";
constexpr inline std::string_view uint32_str = "uint32";
constexpr inline std::string_view uint64_str = "uint64";
constexpr inline std::string_view uint128_str = "uint128";
constexpr inline std::string_view float32_str = "float32";
constexpr inline std::string_view float64_str = "float64";
constexpr inline std::string_view bool_str = "bool";
constexpr inline std::string_view string_str = "string";
constexpr inline std::string_view char_str = "char";

}  // namespace Manganese

#endif  // TYPE_NAMES_HPP