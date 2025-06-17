/**
 * @file stox.h
 * @brief This file contains utility functions for converting strings to various numeric types. It is mainly used in the parser to handle numeric literals.
 * These functions differ from the standard library's `std::stox` functions in that they convert to the
 * types defined in cstdint.h (int8_t, int16_t, etc.), which are guaranteed to have the same width across different platforms.
 * This makes cross-platform code more reliable since (a) there are no assumptions about the size of types like `int` or `long`.
 * and (b) there are no issues in the number_t type (the std::variant of the different numeric types) where the set of types is
 * valid on one platform but incomplete on another.
 */

#ifndef UTILS_STOX_H
#define UTILS_STOX_H

#include <global_macros.h>

#include <cstdint>
#include <limits>
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

inline int8_t stoi8(const std::string& str, int base = 10);
inline int16_t stoi16(const std::string& str, int base = 10);
inline int32_t stoi32(const std::string& str, int base = 10);
inline int64_t stoi64(const std::string& str, int base = 10);
inline uint8_t stou8(const std::string& str, int base = 10);
inline uint16_t stou16(const std::string& str, int base = 10);
inline uint32_t stou32(const std::string& str, int base = 10);
inline uint64_t stou64(const std::string& str, int base = 10);
inline float stof32(const std::string& str);
inline double stof64(const std::string& str);

std::optional<number_t> stonum(const std::string& str, int base = 10, bool isFloat = false, const std::string& suffix = "");

}  // namespace utils

}  // namespace Manganese

#endif  // UTILS_STOX_H