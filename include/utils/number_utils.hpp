/**
 * @file number_utils.hpp
 * @brief Utility functions for converting strings to numbers
 * These functions differ from the standard library's `std::stox` functions in that they convert to the
 * types defined in cstdint.h (int8_t, int16_t, etc.), which are guaranteed to have the same width across different
 * platforms. This makes cross-platform code more reliable since (a) there are no assumptions about the size of types
 * like `int` or `long`. and (b) there are no issues in the number_t type (the std::variant of the different numeric
 * types) where the set of types is valid on one platform but incomplete on another.
 */

#ifndef MANGANESE_INCLUDE_UTILS_NUMBER_UTILS_HPP
#define MANGANESE_INCLUDE_UTILS_NUMBER_UTILS_HPP

#include <cstdint>
#include <global_macros.hpp>
#include <optional>
#include <string_view>
#include <variant>
#if __cplusplus > 202002L
#include <stdfloat>
#endif  // __cplusplus > 202002L


namespace Manganese {

#if __cplusplus <= 202002L
typedef float float32_t;
typedef double float64_t;
#else  // ^^ __cplusplus <= 202002L vv __cplusplus > 202002L
// C++23 added support for fixed-width floating point types
// However, these are not required so may not be available in all standard library implementations
// Use the (required) macros to check if they are available
#if __STDCPP_FLOAT32_T__
typedef std::float32_t float32_t;
#else  // ^^ __STDCPP_FLOAT32_T__ vv !__STDCPP_FLOAT32_T__
typedef float float32_t;
#endif  // __STDCPP_FLOAT32_T__
#if __STDCPP_FLOAT64_T__
typedef std::float64_t float64_t;
#else  // ^^ __STDCPP_FLOAT64_T__ vv !__STDCPP_FLOAT64_T__
typedef double float64_t;
#endif  // __STDCPP_FLOAT64_T__
#endif  // __cplusplus <= 202002L

using number_t = std::variant<int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t, float32_t, float64_t>;

enum class Base: uint8_t {
    Binary = 2,  // 0b prefix
    Octal = 8,  // 0o prefix
    Decimal = 10,  // Default base, no prefix
    Hexadecimal = 16  // 0x prefix
};

constexpr std::string baseToString(Base b) {
    switch (b) {
        case Base::Binary: return "binary";
        case Base::Octal: return "octal";
        case Base::Decimal: return "decimal";
        case Base::Hexadecimal:
            return "hexadecimal";
            default: ASSERT_UNREACHABLE("Invalid base");
    }
}

namespace utils {

constexpr std::optional<int8_t> stoi8(std::string_view str, Base base = Base::Decimal);
constexpr std::optional<int16_t> stoi16(std::string_view str, Base base = Base::Decimal);
constexpr std::optional<int32_t> stoi32(std::string_view str, Base base = Base::Decimal);
constexpr std::optional<int64_t> stoi64(std::string_view str, Base base = Base::Decimal);
constexpr std::optional<uint8_t> stoui8(std::string_view str, Base base = Base::Decimal);
constexpr std::optional<uint16_t> stoui16(std::string_view str, Base base = Base::Decimal);
constexpr std::optional<uint32_t> stoui32(std::string_view str, Base base = Base::Decimal);
constexpr std::optional<uint64_t> stoui64(std::string_view str, Base base = Base::Decimal);
std::optional<float32_t> stof32(std::string_view str);
std::optional<float64_t> stof64(std::string_view str);

/**
 * @brief Converts a string to a number, if possible
 * @param str The string to convert
 * @param base The base (hex/octal/binary/decimal)
 * @param isFloat Whether this is a floating point value
 * @param suffix Whether a type suffix was specified (see the lexer for the acceptable type suffixes)
 * @return The string as a number, or nullopt_t if it failed
 */
    std::optional<number_t>
    stringToNumber(std::string_view str, Base base = Base::Decimal, bool isFloat = false,
                   const std::string& suffix = "") NOEXCEPT_IF_RELEASE;

}  // namespace utils

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_UTILS_NUMBER_UTILS_HPP