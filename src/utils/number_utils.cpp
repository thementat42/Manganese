/**
 * @file number_utils.cpp
 * @brief Utility functions for converting strings to numeric types with optional suffix and base handling.
 *
 * This file provides a set of functions to convert string representations of numbers
 * into the standard intx_t types (int8_t, uint32_t, etc.), for better cross-platform compatibility
 *
 *
 * Internal:
 * - _stox: Template function for integer conversion using std::from_chars.
 */

#include <charconv>
#include <cstdint>
#include <functional>
#include <global_macros.hpp>
#include <io/logging.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utils/number_utils.hpp>

namespace Manganese {

namespace utils {
using suffixMap_t = std::unordered_map<std::string, std::function<std::optional<number_t>(std::string_view, int)>>;

static const inline suffixMap_t suffixMap = {
    {"u8", stoui8},   {"U8", stoui8},   {"u16", stoui16}, {"U16", stoui16}, {"u32", stoui32}, {"U32", stoui32},
    {"u64", stoui64}, {"U64", stoui64}, {"i8", stoi8},    {"I8", stoi8},    {"i16", stoi16},  {"I16", stoi16},
    {"i32", stoi32},  {"I32", stoi32},  {"i64", stoi64},  {"I64", stoi64},
};

std::optional<number_t> stringToNumber(std::string_view str, Base base, bool isFloat,
                                       const std::string& suffix) noexcept_if_release {
    if (isFloat) {
        if (suffix == "f32" || suffix == "F32") {
            return stof32(str);
        } else if (suffix == "") {
            auto val = stof32(str);
            if (val) { return *val; }
            // fallback to f64
        }
        return stof64(str);
    }
    auto it = suffixMap.find(suffix);
    if (it != suffixMap.end()) {
        return it->second(str, static_cast<int>(base));
    } else if (suffix == "") {
        auto i32 = stoi32(str, static_cast<int>(base));
        if (i32) { return *i32; }
        // If i32 fails, try i64
        auto i64 = stoi64(str, static_cast<int>(base));
        if (i64) { return *i64; }
        // If i64 fails, try ui64
        return stoui64(str, static_cast<int>(base));
    } else {
        ASSERT_UNREACHABLE("Invalid Number Suffix: " + suffix);
    }
}

template <typename T>
constexpr std::optional<T> _stox(std::string_view str, int base = 10)
    requires(std::is_integral_v<T> || std::is_floating_point_v<T>)
{
    T temp;
    const char* begin = str.data();
    const char* end = str.data() + str.size();
    auto [ptr, errorCode] = std::from_chars(begin, end, temp, base);
    if (errorCode == std::errc() && ptr == end) { return temp; }
    return std::nullopt;
}

//~ Wrapper methods for convenience

constexpr FORCE_INLINE std::optional<int8_t> stoi8(std::string_view str, int base) { return _stox<int8_t>(str, base); }

constexpr FORCE_INLINE std::optional<int16_t> stoi16(std::string_view str, int base) { return _stox<int16_t>(str, base); }

constexpr FORCE_INLINE std::optional<int32_t> stoi32(std::string_view str, int base) { return _stox<int32_t>(str, base); }

constexpr FORCE_INLINE std::optional<int64_t> stoi64(std::string_view str, int base) { return _stox<int64_t>(str, base); }

constexpr FORCE_INLINE std::optional<uint8_t> stoui8(std::string_view str, int base) { return _stox<uint8_t>(str, base); }

constexpr FORCE_INLINE std::optional<uint16_t> stoui16(std::string_view str, int base) { return _stox<uint16_t>(str, base); }

constexpr FORCE_INLINE std::optional<uint32_t> stoui32(std::string_view str, int base) { return _stox<uint32_t>(str, base); }

constexpr FORCE_INLINE std::optional<uint64_t> stoui64(std::string_view str, int base) { return _stox<uint64_t>(str, base); }

// from_chars doesn't always support floats so fall back to stl functions
FORCE_INLINE std::optional<float32_t> stof32(std::string_view str) { return std::stof(std::string(str)); }

FORCE_INLINE std::optional<float64_t> stof64(std::string_view str) { return std::stod(std::string(str)); }
}  // namespace utils
}  // namespace Manganese
