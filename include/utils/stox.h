/**
 * @file stox.h
 * @brief Utility functions for converting strings to numbers
 * These functions differ from the standard library's `std::stox` functions in that they convert to the
 * types defined in cstdint.h (int8_t, int16_t, etc.), which are guaranteed to have the same width across different platforms.
 * This makes cross-platform code more reliable since (a) there are no assumptions about the size of types like `int` or `long`.
 * and (b) there are no issues in the number_t type (the std::variant of the different numeric types) where the set of types is
 * valid on one platform but incomplete on another.
 */

#ifndef UTILS_STOX_H
#define UTILS_STOX_H

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>

namespace Manganese {

using number_t = std::variant<
    int8_t,
    uint8_t,
    int16_t,
    uint16_t,
    int32_t,
    uint32_t,
    int64_t,
    uint64_t,
    float,
    double>;

namespace utils {

std::optional<int8_t> stoi8_(const std::string& str, int base = 10);
std::optional<int16_t> stoi16_(const std::string& str, int base = 10);
std::optional<int32_t> stoi32_(const std::string& str, int base = 10);
std::optional<int64_t> stoi64_(const std::string& str, int base = 10);
std::optional<uint8_t> stoui8_(const std::string& str, int base = 10);
std::optional<uint16_t> stoui16_(const std::string& str, int base = 10);
std::optional<uint32_t> stoui32_(const std::string& str, int base = 10);
std::optional<uint64_t> stoui64_(const std::string& str, int base = 10);
std::optional<float> stof32_(const std::string& str);
std::optional<double> stof64_(const std::string& str);


/**
 * @brief Converts a string to a number, if possible
 * @param str The string to convert
 * @param base The base (hex/octal/binary/decimal)
 * @param isFloat Whether this is a floating point value 
 * @param suffix Whether a type suffix was specified (see the lexer for the acceptable type suffixes)
 * @return The string as a number, or nullopt_t if it failed
 */
std::optional<number_t> stringToNumber(const std::string& str, int base = 10, bool isFloat = false, const std::string& suffix = "") noexcept_except_catastrophic;

}  // namespace utils

}  // namespace Manganese

#endif  // UTILS_STOX_H