#include <global_macros.h>
#include <io/logging.h>
#include <utils/stox.h>

#include <charconv>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Manganese {

namespace utils {
std::optional<number_t> stringToNumber(std::string_view str, int base, bool isFloat, const std::string& suffix) noexcept_except_catastrophic {
    if (isFloat) {
        if (suffix == "f" || suffix == "F") {
            return stof32(str);
        } else if (suffix == "") {
            auto val = stof32(str);
            if (val) {
                return *val;
            }
            // fallback to f64
        }
        return stof64(str);
    }
    static const std::unordered_map<std::string, std::function<std::optional<number_t>(std::string_view, int)>> suffixMap = {
        {"u8", stoui8},
        {"U8", stoui8},
        {"u16", stoui16},
        {"U16", stoui16},
        {"u32", stoui32},
        {"U32", stoui32},
        {"u64", stoui64},
        {"U64", stoui64},
        {"i8", stoi8},
        {"I8", stoi8},
        {"i16", stoi16},
        {"I16", stoi16},
        {"i32", stoi32},
        {"I32", stoi32},
        {"i64", stoi64},
        {"I64", stoi64},
    };
    auto it = suffixMap.find(suffix);
    if (it != suffixMap.end()) {
        return it->second(str, base);
    } else if (suffix == "") {
        auto i32 = stoi32(str, base);
        if (i32) {
            return *i32;
        }
        // If i32 fails, try i64
        auto i64 = stoi64(str, base);
        if (i64) {
            return *i64;
        }
        // If i64 fails, try ui64
        return stoui64(str, base);
    } else {
        ASSERT_UNREACHABLE("Invalid Number Suffix: " + suffix);
    }
}

template <typename T>
std::optional<T> __stox(std::string_view str, int base = 10) {
    T temp;
    const char* begin = str.data();
    const char* end = str.data() + str.size();
    auto [ptr, errorCode] = std::from_chars(begin, end, temp, base);
    if (errorCode == std::errc() && ptr == end) {
        return temp;
    }
    return std::nullopt;
}

//~ Wrapper methods for convenience

std::optional<int8_t> stoi8(std::string_view str, int base) {
    return __stox<int8_t>(str, base);
}

std::optional<int16_t> stoi16(std::string_view str, int base) {
    return __stox<int16_t>(str, base);
}

std::optional<int32_t> stoi32(std::string_view str, int base) {
    return __stox<int32_t>(str, base);
}

std::optional<int64_t> stoi64(std::string_view str, int base) {
    return __stox<int64_t>(str, base);
}

std::optional<uint8_t> stoui8(std::string_view str, int base) {
    return __stox<uint8_t>(str, base);
}

std::optional<uint16_t> stoui16(std::string_view str, int base) {
    return __stox<uint16_t>(str, base);
}

std::optional<uint32_t> stoui32(std::string_view str, int base) {
    return __stox<uint32_t>(str, base);
}

std::optional<uint64_t> stoui64(std::string_view str, int base) {
    return __stox<uint64_t>(str, base);
}

// from_chars doesn't always support floats so fall back to stl functions
std::optional<float> stof32(std::string_view str) {
    return std::stof(std::string(str));
}

std::optional<double> stof64(std::string_view str) {
    return std::stod(std::string(str));
}
}  // namespace utils
}  // namespace Manganese
