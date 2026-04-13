#ifndef MNSTL_NUMBER
#define MNSTL_NUMBER 1

#include <cmath>
#include <global_macros.hpp>
#include <io/logging.hpp>
#include <limits>
#include <mnstl/ext_nums/i128.hxx>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

namespace mnstl {
struct zero_init_t {
    constexpr explicit zero_init_t() noexcept = default;
};
constexpr inline zero_init_t zero_init{};

enum class Base : uint8_t {
    Binary = 2,  // 0b prefix
    Octal = 8,  // 0o prefix
    Decimal = 10,  // Default base, no prefix
    Hexadecimal = 16  // 0x prefix
};

constexpr const char* baseToString(Base b) noexcept {
    switch (b) {
        case Base::Binary: return "binary";
        case Base::Octal: return "octal";
        case Base::Decimal: return "decimal";
        case Base::Hexadecimal: return "hexadecimal";
        default: manganese_unreachable();
    }
}

class number_t {
   public:
    enum class held_type : uint8_t {
        int8,
        int16,
        int32,
        int64,
        uint8,
        uint16,
        uint32,
        uint64,
        int128,
        uint128,
        float32,
        float64,
        none
    };

   private:
    union {
        int128_t _i128;
        uint128_t _u128;
        float64_t _f64;
        int64_t _i64;
        uint64_t _u64;
        float32_t _f32;
        int32_t _i32;
        uint32_t _u32;
        int16_t _i16;
        uint16_t _u16;
        int8_t _i8;
        uint8_t _u8;
    };
    held_type _underlying;

    template <class F>
    constexpr decltype(auto) _visit(F&& f) const NOEXCEPT_IF_RELEASE {
        using enum held_type;
        switch (_underlying) {
            case int8: return std::forward<F>(f)(_i8);
            case int16: return std::forward<F>(f)(_i16);
            case int32: return std::forward<F>(f)(_i32);
            case int64: return std::forward<F>(f)(_i64);
            case uint8: return std::forward<F>(f)(_u8);
            case uint16: return std::forward<F>(f)(_u16);
            case uint32: return std::forward<F>(f)(_u32);
            case uint64: return std::forward<F>(f)(_u64);
            case float32: return std::forward<F>(f)(_f32);
            case float64: return std::forward<F>(f)(_f64);
            case int128: return std::forward<F>(f)(_i128);
            case uint128: return std::forward<F>(f)(_u128);
            case none:
                ASSERT_UNREACHABLE("Attempted to read a number_t with no stored value");
        }
        ASSERT_UNREACHABLE("");
    }

   public:
    constexpr number_t() noexcept : _underlying(held_type::none) {}
    constexpr number_t(int8_t i8) noexcept : _i8(i8), _underlying(held_type::int8) {}
    constexpr number_t(int16_t i16) noexcept : _i16(i16), _underlying(held_type::int16) {}
    constexpr number_t(int32_t i32) noexcept : _i32(i32), _underlying(held_type::int32) {}
    constexpr number_t(int64_t i64) noexcept : _i64(i64), _underlying(held_type::int64) {}
    constexpr number_t(int128_t i128) noexcept : _i128(i128), _underlying(held_type::int128) {}
    constexpr number_t(uint8_t u8) noexcept : _u8(u8), _underlying(held_type::uint8) {}
    constexpr number_t(uint16_t u16) noexcept : _u16(u16), _underlying(held_type::uint16) {}
    constexpr number_t(uint32_t u32) noexcept : _u32(u32), _underlying(held_type::uint32) {}
    constexpr number_t(uint64_t u64) noexcept : _u64(u64), _underlying(held_type::uint64) {}
    constexpr number_t(uint128_t u128) noexcept : _u128(u128), _underlying(held_type::uint128) {}
    constexpr number_t(float32_t f32) noexcept : _f32(f32), _underlying(held_type::float32) {}
    constexpr number_t(float64_t f64) noexcept : _f64(f64), _underlying(held_type::float64) {}
    constexpr number_t(zero_init_t, held_type t) noexcept : _underlying(t) {
        switch (t) {
            case held_type::int8: _i8 = 0; break;
            case held_type::int16: _i16 = 0; break;
            case held_type::int32: _i32 = 0; break;
            case held_type::int64: _i64 = 0; break;
            case held_type::uint8: _u8 = 0; break;
            case held_type::uint16: _u16 = 0; break;
            case held_type::uint32: _u32 = 0; break;
            case held_type::uint64: _u64 = 0; break;
            case held_type::float32: _f32 = 0; break;
            case held_type::float64: _f64 = 0; break;
            case held_type::int128: _i128 = 0; break;
            case held_type::uint128: _u128 = 0; break;
            case held_type::none: break;
        }
    }

    constexpr number_t(const number_t&) noexcept = default;
    constexpr number_t& operator=(const number_t&) noexcept = default;
    constexpr number_t(number_t&&) noexcept = default;
    constexpr number_t& operator=(number_t&&) noexcept = default;
    constexpr ~number_t() noexcept = default;

    constexpr bool has_value() const noexcept { return _underlying != held_type::none; }

    constexpr held_type underlying_type() const noexcept { return _underlying; }
    constexpr bool is_integer() const noexcept {
        using enum held_type;
        return _underlying == int8 || _underlying == int16 || _underlying == int32 || _underlying == int64
            || _underlying == uint8 || _underlying == uint16 || _underlying == uint32 || _underlying == uint64
            || _underlying == int128 || _underlying == uint128;
    }
    constexpr bool is_float() const noexcept {
        using enum held_type;
        return _underlying == float32 || _underlying == float64;
    }

    constexpr bool is_signed() const noexcept {
        using enum held_type;
        return _underlying == int8 || _underlying == int16 || _underlying == int32 || _underlying == int64
            || _underlying == int128 || _underlying == float32 || _underlying == float64;
    }

    constexpr bool is_unsigned() const noexcept {
        using enum held_type;
        return _underlying == uint8 || _underlying == uint16 || _underlying == uint32 || _underlying == uint64
            || _underlying == uint128;
    }

    template <Numeric T>
    constexpr inline T value_as() const NOEXCEPT_IF_RELEASE {
        return _visit([&]<class U>(U v) -> T { return static_cast<T>(v); });
    }

    template <Numeric T>
    constexpr std::optional<T> value() const noexcept {
        return _underlying == held_type::none ? std::nullopt : std::make_optional<T>(value_as<T>());
    }

    constexpr std::string to_string(bool trim_trailing_decimals = false) const noexcept {
        if (_underlying == held_type::none) { return ""; }
        std::string result = _visit([](Numeric auto v) -> std::string {
            using U = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<U, int8_t> || std::is_same_v<U, uint8_t>) {
                // since int8 and uint8 are char-based, force a promotion to an integer here to print out a number
                // without the promotion this results in an ASCII character
                return std::to_string(+v);
            }  // custom handling for 128-bit ints
            else if constexpr (std::is_same_v<U, int128_t>) {
                return to_string_int128(v);
            } else if constexpr (std::is_same_v<U, uint128_t>) {
                return to_string_uint128(v);
            } else {
                return std::to_string(v);  // regular int, defer to the compiler
            }
        });

        if (is_float() && trim_trailing_decimals) {
            size_t dotPos = result.find('.');
            if (dotPos != std::string::npos) [[likely]] {
                while (!result.empty() && result.back() == '0') { result.pop_back(); }

                // Ensure that values always have something after the decimal point
                if (!result.empty() && result.back() == '.') { result += '0'; }
            }
        }

        return result;
    }
};

constexpr FORCE_INLINE std::string to_string(const number_t& x) noexcept { return x.to_string(); }

template <class T>
    requires(std::is_constructible_v<number_t, T>)
struct string_conversion_result_t {
    T value{0};
    bool exists{false};
    bool overflowed{false};
};

namespace detail {

[[nodiscard]] constexpr int _chtoi(char c) noexcept {
    if (c >= '0' && c <= '9') { return c - '0'; }
    if (c >= 'a' && c <= 'f') { return c - 'a' + 10; }
    if (c >= 'A' && c <= 'F') { return c - 'A' + 10; }
    return -1;
}

[[nodiscard]] constexpr bool isdigit(char c) noexcept { return c >= '0' && c <= '9'; }

template <Numeric T>
[[nodiscard]] constexpr T pow10(int exp) noexcept {
    T result = 1;
    if (exp > 0) {
        while (exp--) { result *= 10; }
    } else {
        while (exp++) { result /= 10; }
    }
    return result;
}

template <FloatingPoint T>
[[nodiscard]] constexpr string_conversion_result_t<T> _stox(const char* ptr, const char* end, Base b,
                                                            bool is_negative) noexcept {
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
        int d = _chtoi(*ptr);
        if (d < 0 || d >= 10) {
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
            int d = _chtoi(*ptr);
            if (d < 0 || d >= 10) {
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
            result.value = (is_negative ? -1 : 1) * std::numeric_limits<T>::infinity();
            return result;
        }
        if (exponent < std::numeric_limits<T>::min_exponent10) {
            result.overflowed = true;
            result.exists = true;
            result.value = 0;
            return result;
        }
        value *= static_cast<T>(pow10<T>(exponent));
    }

    if (is_negative) { value = -value; }
    result.exists = true;
    result.value = value;
    result.overflowed
        = value > std::numeric_limits<T>::max() || value < std::numeric_limits<T>::lowest() || !std::isfinite(value);

    return result;
}

template <Integral T>
struct mn_make_unsigned {
    typedef std::make_unsigned_t<T> type;
};

template <>
struct mn_make_unsigned<int128_t> {
    typedef uint128_t type;
};

template <>
struct mn_make_unsigned<uint128_t> {
    typedef uint128_t type;
};

template <class T>
using mn_make_unsigned_t = typename mn_make_unsigned<T>::type;

template <Integral T>
constexpr string_conversion_result_t<T> _stox(const char* ptr, const char* end, Base b, bool is_negative) noexcept {
    string_conversion_result_t<T> result;
    typedef mn_make_unsigned_t<T> U;

    if (ptr >= end) [[unlikely]] {
        result.exists = false;
        return result;
    }
    const int radix = static_cast<int>(b);

    U value = 0;
    const U max_before_mul = static_cast<U>(std::numeric_limits<U>::max() / static_cast<unsigned>(radix));

    for (; ptr != end; ++ptr) {
        int d = _chtoi(*ptr);
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
        if (is_negative) {
            if (value <= static_cast<U>(std::numeric_limits<T>::max()) + 1) {
                result.value = static_cast<T>(-value);
                result.exists = true;
            } else {
                result.exists = true;
                result.overflowed = true;
            }
        } else {
            if (value <= static_cast<U>(std::numeric_limits<T>::max())) {
                result.value = static_cast<T>(value);
                result.exists = true;
            } else {
                result.exists = true;
                result.overflowed = true;
            }
        }
    } else {
        result.value = static_cast<T>(value);
        result.exists = true;
    }
    return result;
}

}  // namespace detail

template <class T>
constexpr string_conversion_result_t<number_t> wrap_result(const string_conversion_result_t<T>& result) noexcept {
    return string_conversion_result_t<number_t>{
        .value = number_t(result.value), .exists = result.exists, .overflowed = result.overflowed};
}

constexpr string_conversion_result_t<number_t> str_to_num(std::string_view str, bool isFloat) NOEXCEPT_IF_RELEASE {
    if (str.empty()) { return {.exists = false}; }
    const char* const base_ptr = str.data();
    const char* parsing_start = base_ptr;
    const char* parsing_end = base_ptr + str.size();

    Base base;
    bool is_negative = false;

    if (*parsing_start == '+' || *parsing_start == '-') { is_negative = *(parsing_start++) == '-'; }
    if (parsing_start + 1 < parsing_end && *parsing_start == '0') {
        switch (*(parsing_start + 1)) {
            case 'x':
            case 'X': base = Base::Hexadecimal; break;
            case 'b':
            case 'B': base = Base::Binary; break;
            case 'o':
            case 'O': base = Base::Octal; break;
            default:  // Not a base prefix (just leading zero), assume decimal
                base = Base::Decimal;
                break;
        }
        if (base != Base::Decimal) { parsing_start += 2; }
    } else {
        base = Base::Decimal;
    }

    number_t::held_type t;
    if (str.ends_with("i8") || str.ends_with("I8")) {
        parsing_end -= 2;
        t = number_t::held_type::int8;
    } else if (str.ends_with("i16") || str.ends_with("I16")) {
        parsing_end -= 3;
        t = number_t::held_type::int16;
    } else if (str.ends_with("i32") || str.ends_with("I32")) {
        parsing_end -= 3;
        t = number_t::held_type::int32;
    } else if (str.ends_with("i64") || str.ends_with("I64")) {
        parsing_end -= 3;
        t = number_t::held_type::int64;
    } else if (str.ends_with("i128") || str.ends_with("I128")) {
        parsing_end -= 4;
        t = number_t::held_type::int128;
    } else if (str.ends_with("u8") || str.ends_with("U8")) {
        parsing_end -= 2;
        t = number_t::held_type::uint8;
    } else if (str.ends_with("u16") || str.ends_with("U16")) {
        parsing_end -= 3;
        t = number_t::held_type::uint16;
    } else if (str.ends_with("u32") || str.ends_with("U32")) {
        parsing_end -= 3;
        t = number_t::held_type::uint32;
    } else if (str.ends_with("u64") || str.ends_with("U64")) {
        parsing_end -= 3;
        t = number_t::held_type::uint64;
    } else if (str.ends_with("u128") || str.ends_with("U128")) {
        parsing_end -= 4;
        t = number_t::held_type::uint128;
    } else if ((str.ends_with("f32") || str.ends_with("F32")) && isFloat) {
        parsing_end -= 3;
        t = number_t::held_type::float32;
    } else if ((str.ends_with("f64") || str.ends_with("F64")) && isFloat) {
        parsing_end -= 3;
        t = number_t::held_type::float64;
    } else {
        t = number_t::held_type::none;
    }

    if (parsing_end <= base_ptr) { return {.exists = false}; }

    if (isFloat) {
        if (t == number_t::held_type::float32) {
            return wrap_result(detail::_stox<float32_t>(parsing_start, parsing_end, base, is_negative));
        } else if (t == number_t::held_type::none) {
            auto result32 = detail::_stox<float32_t>(parsing_start, parsing_end, base, is_negative);
            return result32.overflowed
                ? wrap_result(detail::_stox<float64_t>(parsing_start, parsing_end, base, is_negative))
                : wrap_result(result32);
        }
        return wrap_result(detail::_stox<float64_t>(parsing_start, parsing_end, base, is_negative));
    } else if (t == number_t::held_type::int8) {
        return wrap_result(detail::_stox<int8_t>(parsing_start, parsing_end, base, is_negative));
    } else if (t == number_t::held_type::int16) {
        return wrap_result(detail::_stox<int16_t>(parsing_start, parsing_end, base, is_negative));
    } else if (t == number_t::held_type::int32) {
        return wrap_result(detail::_stox<int32_t>(parsing_start, parsing_end, base, is_negative));
    } else if (t == number_t::held_type::int64) {
        return wrap_result(detail::_stox<int64_t>(parsing_start, parsing_end, base, is_negative));
    } else if (t == number_t::held_type::int128) {
        return wrap_result(detail::_stox<int128_t>(parsing_start, parsing_end, base, is_negative));
    } else if (t == number_t::held_type::uint8) {
        return wrap_result(detail::_stox<uint8_t>(parsing_start, parsing_end, base, is_negative));
    } else if (t == number_t::held_type::uint16) {
        return wrap_result(detail::_stox<uint16_t>(parsing_start, parsing_end, base, is_negative));
    } else if (t == number_t::held_type::uint32) {
        return wrap_result(detail::_stox<uint32_t>(parsing_start, parsing_end, base, is_negative));
    } else if (t == number_t::held_type::uint64) {
        return wrap_result(detail::_stox<uint64_t>(parsing_start, parsing_end, base, is_negative));
    } else if (t == number_t::held_type::uint128) {
        return wrap_result(detail::_stox<uint128_t>(parsing_start, parsing_end, base, is_negative));
    } else {
        auto result32 = detail::_stox<int32_t>(parsing_start, parsing_end, base, is_negative);
        if (!result32.overflowed) { return wrap_result(result32); }
        // if i32 fails try i64
        auto result64 = detail::_stox<int64_t>(parsing_start, parsing_end, base, is_negative);
        if (!result64.overflowed) { return wrap_result(result64); }
        // if i64 fails try i128
        auto result128 = detail::_stox<int128_t>(parsing_start, parsing_end, base, is_negative);
        if (!result128.overflowed) { return wrap_result(result128); }
        // if i128 fails do u128
        return wrap_result(detail::_stox<uint128_t>(parsing_start, parsing_end, base, is_negative));
    }
}

}  // namespace mnstl

#endif  // MNSTL_NUMBER