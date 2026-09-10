#ifndef MANGANESE_INCLUDE_UTILS_STR_TO_NUM_HPP
#define MANGANESE_INCLUDE_UTILS_STR_TO_NUM_HPP 1

#include <cmath>
#include <core.hpp>
#include <cstdint>
#include <frontend/ast/ast_base.hpp>
#include <mnstl/ext_num_config.hxx>
#include <mnstl/i128.hxx>

namespace Manganese::utils {

enum class Base : std::uint8_t {
    Binary = 2,
    Octal = 8,
    Decimal = 10,
    Hexadecimal = 16
};

template <class T>
struct string_conversion_result_t {
    T value = 0;
    bool exists = false;
    bool overflowed = false;
};

[[nodiscard]] constexpr int _chtoi(char c) noexcept {
    if (BETWEEN(c, '0', '9')) { return c - '0'; }
    if (BETWEEN(c, 'a', 'f')) { return c - 'a' + 10; }
    if (BETWEEN(c, 'A', 'F')) { return c - 'A' + 10; }
    return -1;
}

[[nodiscard]] constexpr bool isdigit(char c) noexcept { return c >= '0' && c <= '9'; }

[[nodiscard]] constexpr inline double pow10(int exp) noexcept {
    if (std::is_constant_evaluated()) {
        double result = 1.0;
        if (exp > 0) {
            while ((exp--) != 0) { result *= 10; }
        } else {
            while ((exp++) != 0) { result /= 10; }
        }
        return result;
    }
    return static_cast<double>(std::pow(10, exp));
}

template <mnstl::FloatingPoint T>
[[nodiscard]] string_conversion_result_t<T> _stox(const char* ptr, const char* end, Base b) noexcept {
    string_conversion_result_t<T> result;
    if (ptr >= end) [[unlikely]] {
        result.exists = false;
        return result;
    }

    if (b != Base::Decimal) [[unlikely]] {
        result.exists = false;
        return result;
    }

    T integer_part = 0;
    bool hasDigits = false;
    while (ptr != end && isdigit(*ptr)) {
        hasDigits = true;
        const int d = _chtoi(*ptr);
        if (!BETWEEN(d, 0, 9)) {
            result.exists = false;
            return result;
        }
        integer_part = integer_part * 10 + static_cast<T>(d);
        ++ptr;
    }

    T fraction_part = 0;
    T fractionDiv = 1;
    if (ptr != end && *ptr == '.') {
        ++ptr;
        while (ptr != end && isdigit(*ptr)) {
            hasDigits = true;
            const int d = _chtoi(*ptr);
            if (!BETWEEN(d, 0, 9)) {
                result.exists = false;
                return result;
            }
            fraction_part = fraction_part * 10 + static_cast<T>(d);
            fractionDiv *= 10;
            ++ptr;
        }
    }
    if (!hasDigits) [[unlikely]] {
        result.exists = false;
        return result;
    }

    T value = integer_part + fraction_part / fractionDiv;
    if (ptr != end && (*ptr == 'e' || *ptr == 'E')) {
        ++ptr;
        bool expNegative = false;
        if (ptr != end && (*ptr == '+' || *ptr == '-')) {
            expNegative = (*ptr == '-');
            ++ptr;
        }

        if (ptr == end || !isdigit(*ptr)) {
            result.exists = false;
            return result;
        }

        int exponent = 0;
        while (ptr != end && isdigit(*ptr)) {
            exponent = exponent * 10 + (*ptr - '0');
            ++ptr;
        }

        if (expNegative) { exponent = -exponent; }
        if (exponent > std::numeric_limits<T>::max_exponent10) {
            result.overflowed = true;
            result.exists = true;
            result.value = std::numeric_limits<T>::infinity();
            return result;
        }
        if (exponent < std::numeric_limits<T>::min_exponent10) {
            result.overflowed = true;
            result.exists = true;
            result.value = 0;
            return result;
        }
        value *= static_cast<T>(pow10(exponent));
    }

    result.exists = true;
    result.value = value;
    result.overflowed
        = value > std::numeric_limits<T>::max() || value < std::numeric_limits<T>::lowest() || !std::isfinite(value);

    return result;
}

template <mnstl::Integral T>
string_conversion_result_t<T> _stox(const char* ptr, const char* end, Base b) noexcept {
    string_conversion_result_t<T> result;
    using U = mnstl::mnstl_make_unsigned_t<T>;

    if (ptr >= end) [[unlikely]] {
        result.exists = false;
        return result;
    }
    const int radix = static_cast<int>(b);

    U value = 0;
    const U max_before_mul = static_cast<U>(std::numeric_limits<U>::max() / static_cast<unsigned>(radix));

    for (; ptr != end; ++ptr) {
        const int d = _chtoi(*ptr);
        if (d < 0 || d >= radix) {
            result.exists = false;
            return result;
        }
        if (value > max_before_mul) { result.overflowed = true; }

        value *= static_cast<U>(radix);

        if (value > std::numeric_limits<U>::max() - static_cast<U>(d)) { result.overflowed = true; }
        value += static_cast<U>(d);
    }

    if constexpr (std::is_signed_v<T>) {
        // Since input strings are always positive, we only check against positive max limits
        if (value <= static_cast<U>(std::numeric_limits<T>::max())) {
            result.value = static_cast<T>(value);
            result.exists = true;
        } else {
            result.exists = true;
            result.overflowed = true;
        }
    } else {
        result.value = static_cast<T>(value);
        result.exists = true;
    }
    return result;
}

template <class Target, class Source>
string_conversion_result_t<Target> wrap_result(const string_conversion_result_t<Source>& result) noexcept {
    return string_conversion_result_t<Target>{
        .value = static_cast<Target>(result.value), .exists = result.exists, .overflowed = result.overflowed};
}

template <mnstl::Numeric T>
inline string_conversion_result_t<T> stringToNumber(std::string_view str, bool isFloat) {
    if (str.empty()) { return {.exists = false}; }
    const char* const base_ptr = str.data();
    const char* parsing_start = base_ptr;
    const char* parsing_end = base_ptr + str.size();

    Base base;

    if (parsing_start + 1 < parsing_end && *parsing_start == '0') {
        switch (*(parsing_start + 1)) {
            case 'x': [[fallthrough]];
            case 'X': base = Base::Hexadecimal; break;
            case 'b': [[fallthrough]];
            case 'B': base = Base::Binary; break;
            case 'o': [[fallthrough]];
            case 'O': base = Base::Octal; break;
            case 'd': [[fallthrough]];
            case 'D':  // explicit decimal prefix
                base = Base::Decimal;
                parsing_start += 2;
                break;
            default:  // Not a base prefix (just leading zero), assume decimal
                base = Base::Decimal;
                break;
        }
        // Don't want to parse the prefix so move the start to after it
        if (base != Base::Decimal) { parsing_start += 2; }
    } else {
        base = Base::Decimal;  // no prefix means decimal
    }

    using enum ast::PrimitiveType;

    ast::PrimitiveType t = not_primitive;

    if (str.ends_with("i8") || str.ends_with("I8")) {
        parsing_end -= 2;
        t = int8;
    } else if (str.ends_with("i16") || str.ends_with("I16")) {
        parsing_end -= 3;
        t = int16;
    } else if (str.ends_with("i32") || str.ends_with("I32")) {
        parsing_end -= 3;
        t = int32;
    } else if (str.ends_with("i64") || str.ends_with("I64")) {
        parsing_end -= 3;
        t = int64;
    } else if (str.ends_with("i128") || str.ends_with("I128")) {
        parsing_end -= 4;
        t = int128;
    } else if (str.ends_with("u8") || str.ends_with("U8")) {
        parsing_end -= 2;
        t = uint8;
    } else if (str.ends_with("u16") || str.ends_with("U16")) {
        parsing_end -= 3;
        t = uint16;
    } else if (str.ends_with("u32") || str.ends_with("U32")) {
        parsing_end -= 3;
        t = uint32;
    } else if (str.ends_with("u64") || str.ends_with("U64")) {
        parsing_end -= 3;
        t = uint64;
    } else if (str.ends_with("u128") || str.ends_with("U128")) {
        parsing_end -= 4;
        t = uint128;
    } else if ((str.ends_with("f32") || str.ends_with("F32")) && isFloat) {
        parsing_end -= 3;
        t = float32;
    } else if ((str.ends_with("f64") || str.ends_with("F64")) && isFloat) {
        parsing_end -= 3;
        t = float64;
    }

    if (parsing_end <= base_ptr) { return {.exists = false}; }

    if (isFloat) {
        if (t == float32) { return wrap_result<T>(_stox<mnstl::float32_t>(parsing_start, parsing_end, base)); }
        if (t == float64) { return wrap_result<T>(_stox<mnstl::float64_t>(parsing_start, parsing_end, base)); }

        auto result32 = _stox<mnstl::float32_t>(parsing_start, parsing_end, base);
        if (!result32.overflowed) { return wrap_result<T>(result32); }
        // if float32 fails do float64
        return wrap_result<T>(_stox<mnstl::float64_t>(parsing_start, parsing_end, base));

    } else if (t == int8) {
        return wrap_result<T>(_stox<std::int8_t>(parsing_start, parsing_end, base));
    } else if (t == int16) {
        return wrap_result<T>(_stox<std::int16_t>(parsing_start, parsing_end, base));
    } else if (t == int32) {
        return wrap_result<T>(_stox<std::int32_t>(parsing_start, parsing_end, base));
    } else if (t == int64) {
        return wrap_result<T>(_stox<std::int64_t>(parsing_start, parsing_end, base));
    } else if (t == int128) {
        return wrap_result<T>(_stox<mnstl::int128_t>(parsing_start, parsing_end, base));
    } else if (t == uint8) {
        return wrap_result<T>(_stox<std::uint8_t>(parsing_start, parsing_end, base));
    } else if (t == uint16) {
        return wrap_result<T>(_stox<std::uint16_t>(parsing_start, parsing_end, base));
    } else if (t == uint32) {
        return wrap_result<T>(_stox<std::uint32_t>(parsing_start, parsing_end, base));
    } else if (t == uint64) {
        return wrap_result<T>(_stox<std::uint64_t>(parsing_start, parsing_end, base));
    } else if (t == uint128) {
        return wrap_result<T>(_stox<mnstl::uint128_t>(parsing_start, parsing_end, base));
    } else {
        auto result32 = _stox<std::int32_t>(parsing_start, parsing_end, base);
        if (!result32.overflowed) { return wrap_result<T>(result32); }
        // if int32 fails try int64
        auto result64 = _stox<std::int64_t>(parsing_start, parsing_end, base);
        if (!result64.overflowed) { return wrap_result<T>(result64); }
        // if int64 fails try int128
        auto result128 = _stox<mnstl::int128_t>(parsing_start, parsing_end, base);
        if (!result128.overflowed) { return wrap_result<T>(result128); }
        // if int128 fails do uint128
        return wrap_result<T>(_stox<mnstl::uint128_t>(parsing_start, parsing_end, base));
    }
}

}  // namespace Manganese::utils

#endif  // MANGANESE_INCLUDE_UTILS_STR_TO_NUM_HPP