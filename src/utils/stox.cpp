#include <global_macros.h>
#include <io/logging.h>
#include <utils/stox.h>

#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>

MANGANESE_BEGIN

/**
 * @brief Range checking for signed numeric types
 * @tparam T The type
 * @param value The value to check
 * @return true if the value is out of range for the type, false otherwise
 */
template <typename T>
inline bool isOutOfRange(const long long int value) {
    return value < std::numeric_limits<T>::min() || value > std::numeric_limits<T>::max();
}

/**
 * @brief Range checking for unsigned numeric types
 * @tparam T The type
 * @param value The value to check
 * @return true if the value is out of range for the type, false otherwise
 */
template <typename T>
inline bool isOutOfRange(const size_t value) {
    return value > std::numeric_limits<T>::max();
}

namespace utils {
std::optional<number_t> stonum(const std::string& str, int base, bool isFloat, const std::string& suffix) {
    if (isFloat) {
        if (suffix == "f" || suffix == "F") {
            try {
                return stof32(str);
            } catch (const std::out_of_range& e) {
                goto f64_conversion;
            }
        }
    f64_conversion:
        return stof64(str);
    }
    if (suffix == "u8" || suffix == "U8") {
        try {
            return stou8(str, base);
        } catch (const std::out_of_range& e) {
            goto ui16_conversion;
        }
    } else if (suffix == "u16" || suffix == "U16") {
    ui16_conversion:
        try {
            return stou16(str, base);
        } catch (const std::out_of_range& e) {
            goto ui32_conversion;
        }
    } else if (suffix == "u32" || suffix == "U32") {
    ui32_conversion:
        try {
            return stou32(str, base);
        } catch (const std::out_of_range& e) {
            goto ui64_conversion;
        }
    } else if (suffix == "u64" || suffix == "U64") {
    ui64_conversion:
        try {
            return stou64(str, base);
        } catch (const std::out_of_range& e) {
            return std::nullopt;  // If all conversions fail, return nullopt
        }
    } else if (suffix == "i8" || suffix == "I8") {
        try {
            return stoi8(str, base);
        } catch (const std::out_of_range& e) {
            goto i16_conversion;
        }
    } else if (suffix == "i16" || suffix == "I16") {
    i16_conversion:
        try {
            return stoi16(str, base);
        } catch (const std::out_of_range& e) {
            goto i32_conversion;
        }
    } else if (suffix == "i32" || suffix == "I32" || suffix.empty()) {  // empty suffix defaults to i32
    i32_conversion:
        try {
            return stoi32(str, base);
        } catch (const std::out_of_range& e) {
            goto i64_conversion;
        }
    } else if (suffix == "i64" || suffix == "I64") {
    i64_conversion:
        try {
            return stoi64(str, base);
        } catch (const std::out_of_range& e) {
            return std::nullopt;  // If all conversions fail, return nullopt
        }
    } else {
        UNREACHABLE("Invalid numeric suffix: " + suffix);
    }
}

inline int8_t stoi8(const std::string& str, int base) {
    long long int temp = std::stoll(str, nullptr, base);
    if (isOutOfRange<int8_t>(temp)) {
        throw std::out_of_range("stoi8: value out of range for int8_t");
    }
    return static_cast<int8_t>(temp);
}
inline int16_t stoi16(const std::string& str, int base) {
    long long int temp = std::stoll(str, nullptr, base);
    if (isOutOfRange<int8_t>(temp)) {
        throw std::out_of_range("stoi16: value out of range for int16_t");
    }
    return static_cast<int16_t>(temp);
}
inline int32_t stoi32(const std::string& str, int base) {
    long long int temp = std::stoll(str, nullptr, base);
    if (isOutOfRange<int8_t>(temp)) {
        throw std::out_of_range("stoi32: value out of range for int32_t");
    }
    return static_cast<int32_t>(temp);
}
inline int64_t stoi64(const std::string& str, int base) {
    long long int temp = std::stoll(str, nullptr, base);
    if (isOutOfRange<int8_t>(temp)) {
        throw std::out_of_range("stoi64: value out of range for int64_t");
    }
    return static_cast<int64_t>(temp);
}
inline uint8_t stou8(const std::string& str, int base) {
    size_t temp = std::stoull(str, nullptr, base);
    if (temp > std::numeric_limits<uint8_t>::max()) {
        throw std::out_of_range("stou8: value out of range for uint8_t");
    }
    return static_cast<uint8_t>(temp);
}
inline uint16_t stou16(const std::string& str, int base) {
    size_t temp = std::stoull(str, nullptr, base);
    if (temp > std::numeric_limits<uint16_t>::max()) {
        throw std::out_of_range("stou16: value out of range for uint16_t");
    }
    return static_cast<uint16_t>(temp);
}
inline uint32_t stou32(const std::string& str, int base) {
    size_t temp = std::stoull(str, nullptr, base);
    if (temp > std::numeric_limits<uint32_t>::max()) {
        throw std::out_of_range("stou32: value out of range for uint32_t");
    }
    return static_cast<uint32_t>(temp);
}
inline uint64_t stou64(const std::string& str, int base) {
    size_t temp = std::stoull(str, nullptr, base);
    if (temp > std::numeric_limits<uint64_t>::max()) {
        throw std::out_of_range("stou64: value out of range for uint64_t");
    }
    return static_cast<uint64_t>(temp);
}
inline float stof32(const std::string& str) {
    try {
        return std::stof(str);
    } catch (const std::out_of_range& e) {
        throw std::out_of_range("stof32: value out of range for float");
    }
}
inline double stof64(const std::string& str) {
    try {
        return std::stod(str);
    } catch (const std::out_of_range& e) {
        throw std::out_of_range("stof64: value out of range for double");
    }
}
}  // namespace utils

MANGANESE_END
