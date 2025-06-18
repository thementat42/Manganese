#include <global_macros.h>
#include <io/logging.h>
#include <utils/stox.h>

#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>

namespace Manganese {

/**
 * @brief Range checking for signed numeric types
 * @tparam T The type
 * @param value The value to check
 * @return true if the value is out of range for the type, false otherwise
 */
template <typename T>
bool isOutOfRange(const int64_t value) {
    return value < std::numeric_limits<T>::min() || value > std::numeric_limits<T>::max();
}

/**
 * @brief Range checking for unsigned numeric types
 * @tparam T The type
 * @param value The value to check
 * @return true if the value is out of range for the type, false otherwise
 */
template <typename T>
bool isOutOfRange(const uint64_t value) {
    return value > std::numeric_limits<T>::max();
}

namespace utils {
std::optional<number_t> stringToNumber(const std::string& str, int base, bool isFloat, const std::string& suffix) {
    if (isFloat) {
        if (suffix == "f" || suffix == "F") {
            return stof32_(str);
        } else if (suffix == "") {
            auto val = stof32_(str);
            if (val) {
                return *val;
            }
            // fallback to f64
        }
        return stof64_(str);
    }
    if (suffix == "u8" || suffix == "U8") {
        return stoui8_(str, base);
    } else if (suffix == "u16" || suffix == "U16") {
        return stoui16_(str, base);
    } else if (suffix == "u32" || suffix == "U32") {
        return stoui32_(str, base);
    } else if (suffix == "u64" || suffix == "U64") {
        return stoui64_(str, base);
    } else if (suffix == "i8" || suffix == "I8") {
        return stoi8_(str, base);
    } else if (suffix == "i16" || suffix == "I16") {
        return stoi16_(str, base);
    } else if (suffix == "i32" || suffix == "I32") {
        return stoi32_(str, base);
    } else if (suffix == "i64" || suffix == "I64") {
        return stoi64_(str, base);
    } else if (suffix == "") {
        auto i32 = stoi32_(str, base);
        if (i32) {
            return *i32;
        }
        // If i32 fails, try i64
        auto i64 = stoi64_(str, base);
        if (i64) {
            return *i64;
        }
        // If i64 fails, try ui64
        return stoui64_(str, base);
    } else {
        UNREACHABLE("Invalid Number Suffix: " + suffix);
    }
}

std::optional<int8_t> stoi8_(const std::string& str, int base) {
    int64_t temp = std::stoll(str, nullptr, base);
    if (isOutOfRange<int8_t>(temp)) {
        return std::nullopt;
    }
    return static_cast<int8_t>(temp);
}

std::optional<int16_t> stoi16_(const std::string& str, int base) {
    int64_t temp = std::stoll(str, nullptr, base);
    if (isOutOfRange<int16_t>(temp)) {
        return std::nullopt;
    }
    return static_cast<int16_t>(temp);
}

std::optional<int32_t> stoi32_(const std::string& str, int base) {
    int64_t temp = std::stoll(str, nullptr, base);
    if (isOutOfRange<int32_t>(temp)) {
        return std::nullopt;
    }
    return static_cast<int32_t>(temp);
}

std::optional<int64_t> stoi64_(const std::string& str, int base) {
    int64_t temp = std::stoll(str, nullptr, base);
    if (isOutOfRange<int64_t>(temp)) {
        return std::nullopt;
    }
    return static_cast<int64_t>(temp);
}

std::optional<uint8_t> stoui8_(const std::string& str, int base) {
    uint64_t temp = std::stoull(str, nullptr, base);
    if (isOutOfRange<uint8_t>(temp)) {
        return std::nullopt;
    }
    return static_cast<uint8_t>(temp);
}

std::optional<uint16_t> stoui16_(const std::string& str, int base) {
    uint64_t temp = std::stoull(str, nullptr, base);
    if (isOutOfRange<uint16_t>(temp)) {
        return std::nullopt;
    }
    return static_cast<uint16_t>(temp);
}

std::optional<uint32_t> stoui32_(const std::string& str, int base) {
    uint64_t temp = std::stoull(str, nullptr, base);
    if (isOutOfRange<uint32_t>(temp)) {
        return std::nullopt;
    }
    return static_cast<uint32_t>(temp);
}

std::optional<uint64_t> stoui64_(const std::string& str, int base) {
    uint64_t temp = std::stoull(str, nullptr, base);
    if (isOutOfRange<uint64_t>(temp)) {
        return std::nullopt;
    }
    return static_cast<uint64_t>(temp);
}

std::optional<float> stof32_(const std::string& str) {
    try {
        return std::stof(str);
    } catch (const std::out_of_range&) {
        return std::nullopt;
    }
}

std::optional<double> stof64_(const std::string& str) {
    try {
        return std::stod(str);
    } catch (const std::out_of_range&) {
        return std::nullopt;
    }
}
}  // namespace utils
}  // namespace Manganese
