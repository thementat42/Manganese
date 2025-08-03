/**
 * @file number_utils.h
 * @brief Utility functions for converting strings to numbers
 * These functions differ from the standard library's `std::stox` functions in that they convert to the
 * types defined in cstdint.h (int8_t, int16_t, etc.), which are guaranteed to have the same width across different
 * platforms. This makes cross-platform code more reliable since (a) there are no assumptions about the size of types
 * like `int` or `long`. and (b) there are no issues in the number_t type (the std::variant of the different numeric
 * types) where the set of types is valid on one platform but incomplete on another.
 */

#ifndef MANGANESE_INCLUDE_UTILS_NUMBER_UTILS_H
#define MANGANESE_INCLUDE_UTILS_NUMBER_UTILS_H

#include <global_macros.hpp>

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <variant>

namespace Manganese {

using number_t = std::variant<int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t, float, double>;

enum class Base {
    Binary = 2,  // 0b prefix
    Octal = 8,  // 0o prefix
    Decimal = 10,  // Default base, no prefix
    Hexadecimal = 16  // 0x prefix
};

constexpr std::string int8_str = "int8";
constexpr std::string int16_str = "int16";
constexpr std::string int32_str = "int32";
constexpr std::string int64_str = "int64";
constexpr std::string uint8_str = "uint8";
constexpr std::string uint16_str = "uint16";
constexpr std::string uint32_str = "uint32";
constexpr std::string uint64_str = "uint64";
constexpr std::string float32_str = "float32";
constexpr std::string float64_str = "float64";
constexpr std::string bool_str = "bool";
constexpr std::string string_str = "string";
constexpr std::string char_str = "char";

namespace utils {

std::optional<int8_t> stoi8(std::string_view str, int base = 10);
std::optional<int16_t> stoi16(std::string_view str, int base = 10);
std::optional<int32_t> stoi32(std::string_view str, int base = 10);
std::optional<int64_t> stoi64(std::string_view str, int base = 10);
std::optional<uint8_t> stoui8(std::string_view str, int base = 10);
std::optional<uint16_t> stoui16(std::string_view str, int base = 10);
std::optional<uint32_t> stoui32(std::string_view str, int base = 10);
std::optional<uint64_t> stoui64(std::string_view str, int base = 10);
std::optional<float> stof32(std::string_view str);
std::optional<double> stof64(std::string_view str);

/**
 * @brief Converts a string to a number, if possible
 * @param str The string to convert
 * @param base The base (hex/octal/binary/decimal)
 * @param isFloat Whether this is a floating point value
 * @param suffix Whether a type suffix was specified (see the lexer for the acceptable type suffixes)
 * @return The string as a number, or nullopt_t if it failed
 */
std::optional<number_t> stringToNumber(std::string_view str, Base base = Base::Decimal, bool isFloat = false,
                                       const std::string& suffix = "") noexcept_if_release;

}  // namespace utils

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_UTILS_NUMBER_UTILS_H