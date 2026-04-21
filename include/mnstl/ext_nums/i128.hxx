#ifndef MNSTL_EXT_NUMS_I128_HXX
#define MNSTL_EXT_NUMS_I128_HXX 1

#include <bit>
#include <cassert>
#include <compare>
#include <cstdint>
#include <format>
#include <limits>
#include <ostream>
#include <type_traits>

#include "ext_num_config.hxx"


// macros to make definitions of op= (e.g. +=) more concise

#define MNSTL_I128_SELF_IN_PLACE_OP(op, is_noexcept)                                          \
    constexpr _basic_int128& operator op## = (const _basic_int128& r) noexcept(is_noexcept) { \
        *this = *this op r;                                                                   \
        return *this;                                                                         \
    }

#define MNSTL_I128_PRIMITIVE_IN_PLACE_OP(op, is_noexcept)                              \
    constexpr _basic_int128& operator op## = (Integral auto r) noexcept(is_noexcept) { \
        *this = *this op r;                                                            \
        return *this;                                                                  \
    }

#define MNSTL_PRIMITIVE_IN_PLACE_OP(op, is_noexcept)                                        \
    template <Integral I, bool B>                                                           \
    constexpr I& operator op## = (I & l, const _basic_int128<B>& r) noexcept(is_noexcept) { \
        l = l op static_cast<I>(r);                                                         \
        return l;                                                                           \
    }

namespace mnstl {

#if !MNSTL_NATIVE_I128
#define TWO_POW_64 18446744073709551616.0L  // for float conversions

// Some helpers

namespace i128_detail {

template <class T>
using i128_arithmetic_result_t = std::conditional_t<std::is_signed_v<T>, int128_t, uint128_t>;

/**
 * helper for bitwise left shift of a 128-bit int
 */
constexpr int128_t _shl_i128(int128_t, unsigned) noexcept;

/**
 * helper for bitwise right shift of a 128-bit int
 */
constexpr int128_t _shr_i128(int128_t, unsigned) noexcept;

/**
 * helper for bitwise left shift of unsigned 128-bit ints
 */
constexpr uint128_t _shl_u128(uint128_t, unsigned) noexcept;

/**
 * helper for bitwise right shift of unsigned 128-bit ints
 */
constexpr uint128_t _shr_u128(uint128_t, unsigned) noexcept;

/**
 * helper for multiplications of two unsigned 128-bit ints
 */
constexpr uint128_t _mul_u128(uint128_t, uint128_t) noexcept;

/**
 * Multiplication of two 64-bit ints to make a 128-bit int
 */
constexpr uint128_t _mul_64(uint64_t, uint64_t) noexcept;

/**
 * Absolute values for 128-bit ints
 */
constexpr uint128_t _abs_i128(int128_t) noexcept;

}  // namespace i128_detail

template <bool is_signed>
struct _basic_int128 {
    // the upper 64 bits encode sign information
    typedef std::conditional_t<is_signed, int64_t, uint64_t> upper_t;

    upper_t _upper;
    uint64_t _lower;

    constexpr _basic_int128() noexcept {};  // purposely leave members uninitialized
    constexpr _basic_int128(const _basic_int128&) noexcept = default;
    constexpr _basic_int128& operator=(const _basic_int128&) noexcept = default;
    constexpr _basic_int128(_basic_int128&&) noexcept = default;
    constexpr _basic_int128& operator=(_basic_int128&&) noexcept = default;

    constexpr _basic_int128(upper_t upper64, uint64_t lower64) : _upper(upper64), _lower(lower64) {}

    template <Integral T>
        requires(sizeof(T) < 16)
    constexpr _basic_int128(T value) {
        _assign_from_primitive(value);
    }

    template <Integral T>
        requires(sizeof(T) < 16)
    constexpr _basic_int128& operator=(T value) {
        _assign_from_primitive(value);
        return *this;
    }

    template <FloatingPoint T>
        requires(sizeof(T) < 16)
    constexpr _basic_int128(T value) {
        _assign_from_primitive(value);
    }

    template <FloatingPoint T>
        requires(sizeof(T) < 16)
    constexpr _basic_int128& operator=(T value) {
        _assign_from_primitive(value);
        return *this;
    }

    constexpr operator bool() const noexcept { return !(_upper == 0 && _lower == 0); }

    template <Integral I>
        requires(sizeof(I) < 16)  // exclude construction from 128-bit (16 byte) values
    constexpr operator I() const noexcept {
        if constexpr (std::is_signed_v<I>) {
            return static_cast<I>(static_cast<int64_t>(_lower));
        } else {
            return static_cast<I>(_lower);
        }
    }

    template <FloatingPoint T>
        requires(sizeof(T) < 16)  // exclude construction from 128-bit (16 byte) values
    constexpr operator T() const noexcept {
        T result = static_cast<T>(_upper) * T(TWO_POW_64);
        result += static_cast<T>(_lower);
        return result;
    }

    // Casts
    constexpr operator int128_t() const noexcept;
    constexpr operator uint128_t() const noexcept;

    MNSTL_I128_SELF_IN_PLACE_OP(+, true)
    MNSTL_I128_SELF_IN_PLACE_OP(-, true)
    MNSTL_I128_SELF_IN_PLACE_OP(*, true)
    MNSTL_I128_SELF_IN_PLACE_OP(/, false)
    MNSTL_I128_SELF_IN_PLACE_OP(%, false)
    MNSTL_I128_SELF_IN_PLACE_OP(&, true)
    MNSTL_I128_SELF_IN_PLACE_OP(|, true)
    MNSTL_I128_SELF_IN_PLACE_OP(^, true)

    constexpr _basic_int128& operator<<=(const _basic_int128& r) noexcept {
        if constexpr (is_signed) {
            *this = i128_detail::_shl_i128(*this, r);
        } else {
            *this = i128_detail::_shl_u128(*this, r);
        }
        return *this;
    }

    constexpr _basic_int128& operator>>=(const _basic_int128& r) noexcept {
        if constexpr (is_signed) {
            *this = i128_detail::_shr_i128(*this, r);
        } else {
            *this = i128_detail::_shr_u128(*this, r);
        }
        return *this;
    }

    MNSTL_I128_PRIMITIVE_IN_PLACE_OP(+, true)
    MNSTL_I128_PRIMITIVE_IN_PLACE_OP(-, true)
    MNSTL_I128_PRIMITIVE_IN_PLACE_OP(*, true)
    MNSTL_I128_PRIMITIVE_IN_PLACE_OP(/, false)
    MNSTL_I128_PRIMITIVE_IN_PLACE_OP(%, false)
    MNSTL_I128_PRIMITIVE_IN_PLACE_OP(&, true)
    MNSTL_I128_PRIMITIVE_IN_PLACE_OP(|, true)
    MNSTL_I128_PRIMITIVE_IN_PLACE_OP(^, true)

    constexpr _basic_int128& operator<<=(Integral auto r) noexcept {
        if constexpr (is_signed) {
            *this = i128_detail::_shl_i128(*this, static_cast<unsigned>(r));
        } else {
            *this = i128_detail::_shl_u128(*this, static_cast<unsigned>(r));
        }
        return *this;
    }

    constexpr _basic_int128& operator>>=(Integral auto r) noexcept {
        if constexpr (is_signed) {
            *this = i128_detail::_shr_i128(*this, static_cast<unsigned>(r));
        } else {
            *this = i128_detail::_shr_u128(*this, static_cast<unsigned>(r));
        }
        return *this;
    }

   private:
    template <Integral T>
        requires(sizeof(T) < 16)
    constexpr void _assign_from_primitive(T value) noexcept {
        if constexpr (std::is_signed_v<T>) {
            _upper = value < 0 ? static_cast<upper_t>(-1) : 0;
        } else {
            _upper = 0;
        }
        _lower = static_cast<uint64_t>(value);
    }

    template <FloatingPoint T>
        requires(sizeof(T) < 16)
    constexpr void _assign_from_primitive(T value) noexcept {
        if (value < 0) {
            _assign_from_primitive(-value);
            *this = -(*this);
            return;
        }
        uint64_t _upper_part = static_cast<uint64_t>(value / TWO_POW_64);
        T remainder = value - static_cast<T>(_upper_part) * TWO_POW_64;
        _upper = static_cast<upper_t>(_upper_part);
        _lower = static_cast<uint64_t>(remainder);
    }
};

MNSTL_PRIMITIVE_IN_PLACE_OP(+, true)
MNSTL_PRIMITIVE_IN_PLACE_OP(-, true)
MNSTL_PRIMITIVE_IN_PLACE_OP(*, true)
MNSTL_PRIMITIVE_IN_PLACE_OP(/, false)
MNSTL_PRIMITIVE_IN_PLACE_OP(%, false)
MNSTL_PRIMITIVE_IN_PLACE_OP(&, true)
MNSTL_PRIMITIVE_IN_PLACE_OP(|, true)
MNSTL_PRIMITIVE_IN_PLACE_OP(^, true)
MNSTL_PRIMITIVE_IN_PLACE_OP(>>, true)
MNSTL_PRIMITIVE_IN_PLACE_OP(<<, true)

// Division Helpers (forward declarations)

namespace i128_detail {

struct divmod_u128_result {
    uint128_t quotient, remainder;
};
struct divmod_i128_result {
    int128_t quotient, remainder;
};
constexpr divmod_u128_result _divmod_u128(uint128_t, uint128_t);
constexpr divmod_i128_result _divmod_i128(int128_t, int128_t);
}  // namespace i128_detail

// Casts
template <>
constexpr int128_t::operator uint128_t() const noexcept {
    return uint128_t{static_cast<uint64_t>(_upper), _lower};
}
template <>
constexpr uint128_t::operator int128_t() const noexcept {
    return int128_t{static_cast<int64_t>(_upper), _lower};
}

// Comparison operators for uint128_t
constexpr bool operator==(uint128_t l, uint128_t r) noexcept {
    return (l._upper == r._upper) && (l._lower == r._lower);
}
constexpr std::strong_ordering operator<=>(uint128_t l, uint128_t r) noexcept {
    if (l._upper != r._upper) { return l._upper <=> r._upper; }
    return l._lower <=> r._lower;
}

// Bitwise operators for uint128_t
constexpr uint128_t operator&(uint128_t l, uint128_t r) noexcept {
    return uint128_t{l._upper & r._upper, l._lower & r._lower};
}
constexpr uint128_t operator|(uint128_t l, uint128_t r) noexcept {
    return uint128_t{l._upper | r._upper, l._lower | r._lower};
}
constexpr uint128_t operator^(uint128_t l, uint128_t r) noexcept {
    return uint128_t{l._upper ^ r._upper, l._lower ^ r._lower};
}
constexpr uint128_t operator~(uint128_t i) noexcept { return uint128_t{~i._upper, ~i._lower}; }
constexpr uint128_t operator<<(uint128_t l, uint128_t r) noexcept {
    return i128_detail::_shl_u128(l, static_cast<unsigned>(r));
}
constexpr uint128_t operator>>(uint128_t l, uint128_t r) noexcept {
    return i128_detail::_shr_u128(l, static_cast<unsigned>(r));
}

// Unary operators for uint128_t
constexpr uint128_t operator+(uint128_t i) noexcept { return i; }
constexpr uint128_t operator-(uint128_t i) noexcept {
    uint64_t result_lower = ~i._lower + 1;
    uint64_t result_higher = ~i._upper + (result_lower == 0);
    return uint128_t{result_higher, result_lower};
}

constexpr uint128_t& operator++(uint128_t& i) noexcept {
    ++i._lower;
    if (i._lower == 0) { ++i._upper; }
    return i;
}
constexpr uint128_t operator++(uint128_t& i, int) noexcept {
    uint128_t tmp = i;
    ++i;
    return tmp;
}
constexpr uint128_t& operator--(uint128_t& i) noexcept {
    --i._lower;
    // underflow, need to subtract from upper bits
    if (i._lower == std::numeric_limits<uint64_t>::max()) { --i._upper; }
    return i;
}
constexpr uint128_t operator--(uint128_t& i, int) noexcept {
    uint128_t tmp = i;
    --i;
    return tmp;
}

// Arithmetic operators for uint128_t
constexpr uint128_t operator+(uint128_t l, uint128_t r) noexcept {
    uint128_t out;
    out._lower = l._lower + r._lower;
    out._upper = l._upper + r._upper + (out._lower < l._lower);  // carry if overflow occured
    return out;
}

constexpr uint128_t operator-(uint128_t l, uint128_t r) noexcept {
    uint128_t out;
    out._lower = l._lower - r._lower;
    out._upper = l._upper - r._upper - (out._lower > l._lower);  // borrow 1 if underflow ocuured in lower bits
    return out;
}
constexpr uint128_t operator*(uint128_t l, uint128_t r) noexcept { return i128_detail::_mul_u128(l, r); }
constexpr uint128_t operator/(uint128_t l, uint128_t r) { return i128_detail::_divmod_u128(l, r).quotient; }
constexpr uint128_t operator%(uint128_t l, uint128_t r) { return i128_detail::_divmod_u128(l, r).remainder; }

// Comparison operators for int128_t
constexpr bool operator==(int128_t l, int128_t r) noexcept { return (l._upper == r._upper) && (l._lower == r._lower); }
constexpr std::strong_ordering operator<=>(int128_t l, int128_t r) noexcept {
    if (l._upper != r._upper) { return l._upper <=> r._upper; }
    return l._lower <=> r._lower;
}

// Bitwise operators for int128_t
constexpr int128_t operator&(int128_t l, int128_t r) noexcept {
    return int128_t{l._upper & r._upper, l._lower & r._lower};
}
constexpr int128_t operator|(int128_t l, int128_t r) noexcept {
    return int128_t{l._upper | r._upper, l._lower | r._lower};
}
constexpr int128_t operator^(int128_t l, int128_t r) noexcept {
    return int128_t{l._upper ^ r._upper, l._lower ^ r._lower};
}
constexpr int128_t operator~(int128_t i) noexcept { return int128_t{~i._upper, ~i._lower}; }
constexpr int128_t operator<<(int128_t l, int128_t r) noexcept {
    return i128_detail::_shl_i128(l, static_cast<unsigned>(r));
}
constexpr int128_t operator>>(int128_t l, int128_t r) noexcept {
    return i128_detail::_shr_i128(l, static_cast<unsigned>(r));
}

// Unary operators for int128_t
constexpr int128_t operator+(int128_t i) noexcept { return i; }
constexpr int128_t operator-(int128_t i) noexcept {
    uint64_t result_lower = ~i._lower + 1;
    int64_t result_upper = ~i._upper + (result_lower == 0);  // if result low is 0, overflow occured, carry
    return int128_t{result_upper, result_lower};
}
constexpr int128_t& operator++(int128_t& i) noexcept {
    ++i._lower;
    if (i._lower == 0) { ++i._upper; }  // overflow
    return i;
}
constexpr int128_t operator++(int128_t& i, int) noexcept {
    int128_t tmp = i;
    ++i;
    return tmp;  // return un-incremented value
}
constexpr int128_t& operator--(int128_t& i) noexcept {
    --i._lower;
    // underflow, need to subtract from upper bits
    if (i._lower == std::numeric_limits<uint64_t>::max()) { --i._upper; }
    return i;
}
constexpr int128_t operator--(int128_t& i, int) noexcept {
    int128_t tmp = i;
    --i;
    return tmp;  // return un-decremented value
}

// Arithmetic operators for int128_t
constexpr int128_t operator+(int128_t l, int128_t r) noexcept {
    int128_t out;
    out._lower = l._lower + r._lower;
    out._upper = l._upper + r._upper + (out._lower < l._lower);  // carry if overflow occured
    return out;
}
constexpr int128_t operator-(int128_t l, int128_t r) noexcept {
    int128_t out;
    out._lower = l._lower - r._lower;
    out._upper = l._upper - r._upper - (out._lower > l._lower);  // borrow 1 if underflow ocuured in lower bits
    return out;
}

constexpr int128_t operator*(int128_t l, int128_t r) noexcept {
    bool result_negative = (l._upper < 0) ^ (r._upper < 0);
    uint128_t result = i128_detail::_mul_u128(i128_detail::_abs_i128(l), i128_detail::_abs_i128(r));
    return result_negative ? -static_cast<int128_t>(result) : static_cast<int128_t>(result);
}

constexpr int128_t operator/(int128_t l, int128_t r) { return i128_detail::_divmod_i128(l, r).quotient; }
constexpr int128_t operator%(int128_t l, int128_t r) { return i128_detail::_divmod_i128(l, r).remainder; }

// Comparison operators between int128_t and uint128_t
constexpr bool operator==(int128_t l, uint128_t r) noexcept {
    if (l._upper < 0) { return false; }  // a negative value can never equal an unsigned value
    return (static_cast<uint64_t>(l._upper) == r._upper) && (l._lower == r._lower);
}
constexpr std::strong_ordering operator<=>(int128_t l, uint128_t r) noexcept {
    if (l._upper < 0) { return std::strong_ordering::less; }  // a negative value is always less than an unsigned value
    uint64_t l_upper_unsigned = static_cast<uint64_t>(l._upper);
    if (l_upper_unsigned != r._upper) { return l_upper_unsigned <=> r._upper; }
    return l._lower <=> r._lower;
}
constexpr bool operator==(uint128_t l, int128_t r) noexcept { return r == l; }
constexpr std::strong_ordering operator<=>(uint128_t l, int128_t r) noexcept {
    std::strong_ordering cmp_result = r <=> l;  // need to reverse the result of this
    if (cmp_result == std::strong_ordering::less) { return std::strong_ordering::greater; }
    if (cmp_result == std::strong_ordering::greater) { return std::strong_ordering::less; }
    return cmp_result;
}

// Bitwise operators between int128_t and uint128_t

constexpr int128_t operator&(int128_t l, uint128_t r) noexcept {
    int64_t result_upper = static_cast<int64_t>(static_cast<uint64_t>(l._upper) & r._upper);
    return int128_t{result_upper, l._lower & r._lower};
}
constexpr int128_t operator|(int128_t l, uint128_t r) noexcept {
    int64_t result_upper = static_cast<int64_t>(static_cast<uint64_t>(l._upper) | r._upper);
    return int128_t{result_upper, l._lower | r._lower};
}
constexpr int128_t operator^(int128_t l, uint128_t r) noexcept {
    int64_t result_upper = static_cast<int64_t>(static_cast<uint64_t>(l._upper) ^ r._upper);
    return int128_t{result_upper, l._lower ^ r._lower};
}
constexpr int128_t operator<<(int128_t l, uint128_t r) noexcept {
    return i128_detail::_shl_i128(l, static_cast<unsigned>(r));
}
constexpr int128_t operator>>(int128_t l, uint128_t r) noexcept {
    return i128_detail::_shr_i128(l, static_cast<unsigned>(r));
}

constexpr uint128_t operator&(uint128_t l, int128_t r) noexcept {
    uint128_t r_uint = static_cast<uint128_t>(r);
    return uint128_t{l._upper & r_uint._upper, l._lower & r_uint._lower};
}
constexpr uint128_t operator|(uint128_t l, int128_t r) noexcept {
    uint128_t r_uint = static_cast<uint128_t>(r);
    return uint128_t{l._upper | r_uint._upper, l._lower | r_uint._lower};
}
constexpr uint128_t operator^(uint128_t l, int128_t r) noexcept {
    uint128_t r_uint = static_cast<uint128_t>(r);
    return uint128_t{l._upper ^ r_uint._upper, l._lower ^ r_uint._lower};
}
constexpr uint128_t operator<<(uint128_t l, int128_t r) noexcept {
    return i128_detail::_shl_u128(l, static_cast<uint128_t>(r));
}
constexpr uint128_t operator>>(uint128_t l, int128_t r) noexcept {
    return i128_detail::_shr_u128(l, static_cast<uint128_t>(r));
}

// Arithmetic operators between int128_t and uint128_t

constexpr uint128_t operator+(int128_t l, uint128_t r) noexcept { return static_cast<uint128_t>(l) + r; }
constexpr uint128_t operator-(int128_t l, uint128_t r) noexcept { return static_cast<uint128_t>(l) - r; }
constexpr uint128_t operator*(int128_t l, uint128_t r) noexcept {
    if (l._upper < 0) { return -i128_detail::_mul_u128(i128_detail::_abs_i128(l), r); }
    return i128_detail::_mul_u128(i128_detail::_abs_i128(l), r);
}
constexpr uint128_t operator/(int128_t l, uint128_t r) { return static_cast<uint128_t>(l) / r; }
constexpr uint128_t operator%(int128_t l, uint128_t r) { return static_cast<uint128_t>(l) % r; }

constexpr uint128_t operator+(uint128_t l, int128_t r) noexcept { return l + static_cast<uint128_t>(r); }
constexpr uint128_t operator-(uint128_t l, int128_t r) noexcept { return l - static_cast<uint128_t>(r); }
constexpr uint128_t operator*(uint128_t l, int128_t r) noexcept { return r * l; }
constexpr uint128_t operator/(uint128_t l, int128_t r) { return l / static_cast<uint128_t>(r); }
constexpr uint128_t operator%(uint128_t l, int128_t r) { return l % static_cast<uint128_t>(r); }

// Comparison operators between int128_t and primitives
constexpr bool operator==(int128_t l, Integral auto r) noexcept { return l == int128_t{r}; }
constexpr std::strong_ordering operator<=>(int128_t l, Integral auto r) noexcept { return l <=> int128_t{r}; }
constexpr bool operator==(Integral auto l, int128_t r) noexcept { return int128_t{l} == r; }
constexpr std::strong_ordering operator<=>(Integral auto l, int128_t r) noexcept { return int128_t{l} <=> r; }

// Bitwise operators between int128_t and primitives

constexpr int128_t operator&(int128_t l, Integral auto r) noexcept { return l & int128_t{r}; }
constexpr int128_t operator|(int128_t l, Integral auto r) noexcept { return l | int128_t{r}; }
constexpr int128_t operator^(int128_t l, Integral auto r) noexcept { return l ^ int128_t{r}; }
constexpr int128_t operator<<(int128_t l, Integral auto r) noexcept {
    return i128_detail::_shl_i128(l, static_cast<unsigned>(r));
}
constexpr int128_t operator>>(int128_t l, Integral auto r) noexcept {
    return i128_detail::_shr_i128(l, static_cast<unsigned>(r));
}

constexpr int128_t operator&(Integral auto l, int128_t r) noexcept { return int128_t{l} & r; }
constexpr int128_t operator|(Integral auto l, int128_t r) noexcept { return int128_t{l} | r; }
constexpr int128_t operator^(Integral auto l, int128_t r) noexcept { return int128_t{l} ^ r; }
constexpr int128_t operator<<(Integral auto l, int128_t r) noexcept {
    return i128_detail::_shl_i128(int128_t{l}, static_cast<unsigned>(r));
}
constexpr int128_t operator>>(Integral auto l, int128_t r) noexcept {
    return i128_detail::_shr_i128(int128_t{l}, static_cast<unsigned>(r));
}

// Arithmetic operators between int128_t and primitives

template <Integral T>
constexpr i128_detail::i128_arithmetic_result_t<T> operator+(int128_t l, T r) noexcept {
    if constexpr (std::is_signed_v<T>) {
        return l + int128_t{r};
    } else {
        return static_cast<uint128_t>(l) + uint128_t{r};
    }
}
template <Integral T>
constexpr i128_detail::i128_arithmetic_result_t<T> operator-(int128_t l, T r) noexcept {
    if constexpr (std::is_signed_v<T>) {
        return l - int128_t{r};
    } else {
        return static_cast<uint128_t>(l) - uint128_t{r};
    }
}
template <Integral T>
constexpr i128_detail::i128_arithmetic_result_t<T> operator*(int128_t l, T r) noexcept {
    if constexpr (std::is_signed_v<T>) {
        return l * int128_t(r);
    } else {
        return l * uint128_t(r);
    }
}
template <Integral T>
constexpr i128_detail::i128_arithmetic_result_t<T> operator/(int128_t l, T r) {
    if constexpr (std::is_signed_v<T>) {
        return l / int128_t{r};
    } else {
        return static_cast<uint128_t>(l) / uint128_t{r};
    }
}
template <Integral T>
constexpr i128_detail::i128_arithmetic_result_t<T> operator%(int128_t l, T r) {
    if constexpr (std::is_signed_v<T>) {
        return l % int128_t{r};
    } else {
        return static_cast<uint128_t>(l) % uint128_t{r};
    }
}

template <Integral T>
constexpr i128_detail::i128_arithmetic_result_t<T> operator+(T l, int128_t r) noexcept {
    if constexpr (std::is_signed_v<T>) {
        return int128_t{l} + r;
    } else {
        return uint128_t{l} + static_cast<uint128_t>(r);
    }
}
template <Integral T>
constexpr i128_detail::i128_arithmetic_result_t<T> operator-(T l, int128_t r) noexcept {
    if constexpr (std::is_signed_v<T>) {
        return int128_t{l} - r;
    } else {
        return uint128_t{l} - static_cast<uint128_t>(r);
    }
}
template <Integral T>
constexpr i128_detail::i128_arithmetic_result_t<T> operator*(T l, int128_t r) noexcept {
    return r * l;
}
template <Integral T>
constexpr i128_detail::i128_arithmetic_result_t<T> operator/(T l, int128_t r) {
    if constexpr (std::is_signed_v<T>) {
        return int128_t{l} / r;
    } else {
        return uint128_t{l} / static_cast<uint128_t>(r);
    }
}
template <Integral T>
constexpr i128_detail::i128_arithmetic_result_t<T> operator%(T l, int128_t r) {
    if constexpr (std::is_signed_v<T>) {
        return int128_t{l} % r;
    } else {
        return uint128_t{l} % static_cast<uint128_t>(r);
    }
}

// Comparison operators between uint128_t and primitives
constexpr bool operator==(uint128_t l, Integral auto r) noexcept { return l == uint128_t{r}; }
constexpr std::strong_ordering operator<=>(uint128_t l, Integral auto r) noexcept { return l <=> uint128_t{r}; }
constexpr bool operator==(Integral auto l, uint128_t r) noexcept { return uint128_t{l} == r; }
constexpr std::strong_ordering operator<=>(Integral auto l, uint128_t r) noexcept { return uint128_t{l} <=> r; }

// Bitwise operators between uint128_t and primitives

constexpr uint128_t operator&(uint128_t l, Integral auto r) noexcept { return l & uint128_t{r}; }
constexpr uint128_t operator|(uint128_t l, Integral auto r) noexcept { return l | uint128_t{r}; }
constexpr uint128_t operator^(uint128_t l, Integral auto r) noexcept { return l ^ uint128_t{r}; }
constexpr uint128_t operator<<(uint128_t l, Integral auto r) noexcept {
    return i128_detail::_shl_u128(l, static_cast<unsigned>(r));
}
constexpr uint128_t operator>>(uint128_t l, Integral auto r) noexcept {
    return i128_detail::_shr_u128(l, static_cast<unsigned>(r));
}

constexpr uint128_t operator&(Integral auto l, uint128_t r) noexcept { return uint128_t{l} & r; }
constexpr uint128_t operator|(Integral auto l, uint128_t r) noexcept { return uint128_t{l} | r; }
constexpr uint128_t operator^(Integral auto l, uint128_t r) noexcept { return uint128_t{l} ^ r; }
constexpr uint128_t operator<<(Integral auto l, uint128_t r) noexcept {
    return i128_detail::_shl_u128(uint128_t{l}, static_cast<unsigned>(r));
}
constexpr uint128_t operator>>(Integral auto l, uint128_t r) noexcept {
    return i128_detail::_shr_u128(uint128_t{l}, static_cast<unsigned>(r));
}

// Arithmetic operators between uint128_t and primitives

constexpr uint128_t operator+(uint128_t l, Integral auto r) noexcept { return l + uint128_t{r}; }
constexpr uint128_t operator-(uint128_t l, Integral auto r) noexcept { return l - uint128_t{r}; }
constexpr uint128_t operator*(uint128_t l, Integral auto r) noexcept { return l * uint128_t{r}; }
constexpr uint128_t operator/(uint128_t l, Integral auto r) { return l / uint128_t{r}; }
constexpr uint128_t operator%(uint128_t l, Integral auto r) { return l % uint128_t{r}; }

constexpr uint128_t operator+(Integral auto l, uint128_t r) noexcept { return uint128_t{l} + r; }
constexpr uint128_t operator-(Integral auto l, uint128_t r) noexcept { return uint128_t{l} - r; }
constexpr uint128_t operator*(Integral auto l, uint128_t r) noexcept { return uint128_t{l} * r; }
constexpr uint128_t operator/(Integral auto l, uint128_t r) { return uint128_t{l} / r; }
constexpr uint128_t operator%(Integral auto l, uint128_t r) { return uint128_t{l} % r; }

// Helpers

namespace i128_detail {

constexpr int128_t _shl_i128(int128_t value, unsigned shift) noexcept {
    if (shift == 0) { return value; }
    // shifts larger than width are usually UB, but for the sake of performance, just return 0 instead
    if (shift >= 128) { return int128_t{0}; }
    if (shift < 64) {
        uint64_t new_upper = (static_cast<uint64_t>(value._upper) << shift) | (value._lower >> (64 - shift));

        return int128_t{static_cast<int64_t>(new_upper), value._lower << shift};
    }
    return int128_t{static_cast<int64_t>(value._lower << (shift - 64)), 0};
}
constexpr int128_t _shr_i128(int128_t value, unsigned shift) noexcept {
    if (shift == 0) { return value; }
    // shifts larger than the width are usually UB but for the sake of performance return a constant value
    // for negative values, sign extension should keep it negative
    // for positive values, this is just 0
    if (shift >= 128) { return value._upper < 0 ? int128_t{-1, static_cast<uint64_t>(~0)} : int128_t{0}; }
    if (shift < 64) {
        return int128_t{value._upper >> shift,  // propagates sign
                                                // account for upper bits being shifted in: keep any bits that are 1
                        (value._lower >> shift) | (static_cast<uint64_t>(value._upper) << (64 - shift))};
    }
    return {value._upper < 0 ? -1 : 0,  // replicate sign
            static_cast<uint64_t>(value._upper >> (shift - 64))};
}

constexpr uint128_t _shl_u128(uint128_t value, unsigned shift) noexcept {
    if (shift == 0) { return value; }
    if (shift >= 128) { return uint128_t{0}; }
    if (shift < 64) {
        return uint128_t{(value._upper << shift) | (value._lower >> (64 - shift)), value._lower << shift};
    }
    return uint128_t{value._lower << (shift - 64), 0};
}
constexpr uint128_t _shr_u128(uint128_t value, unsigned shift) noexcept {
    if (shift == 0) { return value; }
    if (shift >= 128) { return uint128_t{0}; }
    if (shift < 64) {
        return uint128_t{value._upper >> shift, (value._lower >> shift) | (value._upper << (64 - shift))};
    }
    return uint128_t{0, value._upper >> (shift - 64)};
}

constexpr unsigned _ctz_u128(uint128_t x) noexcept {
    if (x == 0) { return 128; }
    // since the number of trailing 0s >= 0, casting to unsigned is ok
    if (x._lower != 0) { return static_cast<unsigned>(std::countr_zero(x._lower)); }
    return 64 + static_cast<unsigned>(std::countr_zero(x._upper));
}

constexpr unsigned _clz_u128(uint128_t x) noexcept {
    if (x == 0) { return 128; }
    if (x._upper != 0) { return static_cast<unsigned>(std::countl_zero(x._upper)); }
    return 64 + static_cast<unsigned>(std::countl_zero(x._lower));
}

constexpr uint128_t _mul_64(uint64_t a, uint64_t b) noexcept {
    uint64_t a_low = static_cast<uint32_t>(a);
    uint64_t b_low = static_cast<uint32_t>(b);
    uint64_t a_high = a >> 32;
    uint64_t b_high = b >> 32;

    // effectively FOIL-ing (a_high, a_low)*(b_high, b_low)
    uint64_t p0 = a_low * b_low;
    uint64_t p1 = a_low * b_high;
    uint64_t p2 = a_high * b_low;
    uint64_t p3 = a_high * b_high;

    // p1 and p2 are both effectively shifted left by 32 bits in the end product (i.e. the middle 64 bits, with 32 bits
    // on either side)
    // add the upper 32 bits of the low*low term (carry) + the lower 32 bits of the cross terms to get this middle term
    uint64_t mid = (p0 >> 32) + static_cast<uint32_t>(p1) + static_cast<uint32_t>(p2);

    // the lower 64 bits are the lower 32 bits of mid and the lower 32 bits of p0.
    // the upper 64 bits are p3 and the upper 32 bits of p1 and p2
    return uint128_t{p3 + (p1 >> 32) + (p2 >> 32) + (mid >> 32), (mid << 32) | static_cast<uint32_t>(p0)};
}

constexpr uint128_t _mul_u128(uint128_t a, uint128_t b) noexcept {
    if (a == 0 || b == 0) { return 0; }
    if (a == 1) { return b; }
    if (b == 1) { return a; }

    // Fast path for powers of 2 (multiplication by a power of 2 is just a shift)
    // if some number is a power of 2, in memory, it looks like a 1 and a bunch of zeroes
    // some number - 1 will have 1s in all those lower bit positions (above that will all be 1)
    // a bitwise and will then give 0
    // e.g. 4 is 0b0100
    // 4-1 = 3 = 0b0011
    // 4 & 3 is then 0

    if ((b & (b - 1)) == 0) {
        unsigned shift = _ctz_u128(b);
        return _shl_u128(a, shift);
    }
    if ((a & (a - 1)) == 0) {
        unsigned shift = _ctz_u128(a);

        return _shl_u128(b, shift);
    }

    uint64_t a_low = a._lower;
    uint64_t a_high = a._upper;
    uint64_t b_low = b._lower;
    uint64_t b_high = b._upper;

    // effectively FOIL on (upper,lower)*(upper,lower)
    uint128_t ll = _mul_64(a_low, b_low);
    uint128_t lh = _mul_64(a_low, b_high);
    uint128_t hl = _mul_64(a_high, b_low);
    // high*high term goes like 2^128 and can't fit so ignore it

    uint64_t low = ll._lower;
    uint64_t carry = ll._upper;

    uint64_t mid_low = lh._lower + hl._lower;
    uint64_t mid_carry = mid_low < lh._lower;
    carry += mid_low;

    if (carry < mid_low) { ++mid_carry; }
    uint64_t high = lh._upper + hl._upper + mid_carry;
    return uint128_t{high, low};
}

constexpr divmod_u128_result _divmod_u128(uint128_t numerator, uint128_t denominator) {
    if (denominator == 0) {  // trap on division by 0
        if (std::is_constant_evaluated()) {
            throw "Division by zero error";
        } else {
#if defined(__GNUC__) || defined(__clang__) || (defined(__has_builtin) && __has_builtin(__builtin_trap))
            __builtin_trap();
#else
            std::abort();
#endif
        }
    }

    // shortcuts for easy values (0/x = 0 and x/1 = x)
    if (numerator == 0) { return divmod_u128_result{.quotient = 0, .remainder = 0}; }
    if (denominator == 1) { return divmod_u128_result{.quotient = numerator, .remainder = 0}; }

    if (numerator < denominator) { return divmod_u128_result{.quotient = 0, .remainder = numerator}; }

    if (numerator._upper == 0 && denominator._upper == 0) {
        // these are really 64-bit ints, do division normally
        // this should also be easy to optimize since most hardware dividers automatically compute the remainder
        // so it's one instruction + 2 moves
        uint64_t quotient = numerator._lower / denominator._lower;
        uint64_t remainder = numerator._lower % denominator._lower;
        return divmod_u128_result{.quotient = quotient, .remainder = remainder};
    }

    // shortcut for powers of two
    // division by 2^N is the same as a right shift
    if ((denominator & (denominator - 1)) == 0) {
        unsigned shift = static_cast<uint128_t>(_ctz_u128(denominator));
        return divmod_u128_result{.quotient = numerator >> shift, .remainder = numerator & (denominator - 1)};
    }

    // this is a restoring divider
    uint128_t quotient = 0;
    uint128_t remainder = 0;

    int msb_divisor = static_cast<int>(127 - _clz_u128(denominator));
    int msb_dividend = static_cast<int>(127 - _clz_u128(numerator));
    int shift = msb_dividend - msb_divisor;
    denominator <<= shift;

    for (; shift >= 0; --shift) {
        remainder <<= 1;
        remainder |= (numerator >> shift) & 1;  // bring in next bit
        if (remainder >= denominator) {
            remainder -= denominator;
            quotient |= uint128_t{1} << shift;
        }
    }
    return divmod_u128_result{.quotient = quotient, .remainder = remainder};
}

}  // namespace i128_detail

constexpr void swap(int128_t& a, int128_t& b) noexcept {
    std::swap(a._upper, b._upper);
    std::swap(a._lower, b._lower);
}

constexpr void swap(uint128_t& a, uint128_t& b) noexcept {
    std::swap(a._upper, b._upper);
    std::swap(a._lower, b._lower);
}

namespace i128_detail {
constexpr divmod_i128_result _divmod_i128(int128_t numerator, int128_t denominator) {
    bool negative_numerator = numerator < 0;
    bool negative_denominator = denominator < 0;
    const auto divmod_result = _divmod_u128(_abs_i128(numerator), _abs_i128(denominator));
    int q_sign = (negative_numerator ^ negative_denominator ? -1 : 1);
    int r_sign = (negative_numerator ? -1 : 1);

    return divmod_i128_result{.quotient = static_cast<int128_t>(divmod_result.quotient) * q_sign,
                              .remainder = static_cast<int128_t>(divmod_result.remainder) * r_sign};
}
}  // namespace i128_detail

#endif  // !MNSTL_NATIVE_I128

namespace i128_detail {

constexpr uint128_t _abs_i128(int128_t i) noexcept {
    return (i < 0) ? static_cast<uint128_t>(-i) : static_cast<uint128_t>(i);
}

constexpr std::string _tostr_u128(const uint128_t& i) {
    if (i == 0) { return "0"; }
    char buffer[40];  // 128 bits fits in ceil(log10(2^128)) = 39 decimal digits + 1 for null terminator
    char* ptr = buffer + sizeof(buffer) - sizeof(char);  // fill the buffer from the end
    *ptr = '\0';
    uint128_t temp = i;
    while (temp != 0) {
#if MNSTL_NATIVE_I128
        auto quotient = temp / uint128_t{10};
        auto remainder = temp % uint128_t{10};
#else
        const i128_detail::divmod_u128_result divmod = i128_detail::_divmod_u128(temp, uint128_t{10});
        const auto quotient = divmod.quotient;
        const auto remainder = divmod.remainder;
#endif
        --ptr;
        *ptr = static_cast<char>('0' + uint8_t(remainder));  // 0 <= remainder <= 9
        temp = quotient;
    }
    return std::string(ptr);
}

constexpr std::string _tostr_i128(const int128_t& i) {
    if (i < 0) { return "-" + _tostr_u128(_abs_i128(i)); }
    return _tostr_u128(static_cast<uint128_t>(i));
}
}  // namespace i128_detail

constexpr inline std::string to_string_int128(const int128_t& i) { return i128_detail::_tostr_i128(i); }
constexpr inline std::string to_string_uint128(const uint128_t& i) { return i128_detail::_tostr_u128(i); }

}  // namespace mnstl

inline std::ostream& operator<<(std::ostream& os, const mnstl::uint128_t& i) {
    return os << mnstl::i128_detail::_tostr_u128(i);
}
inline std::ostream& operator<<(std::ostream& os, const mnstl::int128_t& i) {
    return os << mnstl::i128_detail::_tostr_i128(i);
}

// STL specializations

#if !MNSTL_NATIVE_I128
namespace std {
// only specialize for custom int128
template <>
struct formatter<mnstl::uint128_t> : public std::formatter<std::string> {
    template <class FormatContext>
    auto format(const mnstl::uint128_t& value, FormatContext& ctx) {
        std::string str = mnstl::to_string_uint128(value);
        return std::formatter<std::string>::format(str, ctx);
    }
};

template <>
struct formatter<mnstl::int128_t> : public std::formatter<std::string> {
    template <class FormatContext>
    auto format(const mnstl::int128_t& value, FormatContext& ctx) {
        std::string str = mnstl::to_string_int128(value);
        return std::formatter<std::string>::format(str, ctx);
    }
};

template <>
class numeric_limits<mnstl::int128_t> {
   public:
    constexpr static bool is_specialized = true;
    constexpr static bool is_signed = true;
    constexpr static bool is_integer = true;
    constexpr static bool is_exact = true;
    constexpr static bool has_infinity = false;
    constexpr static bool has_quiet_NaN = false;
    constexpr static bool has_signaling_NaN = false;
    constexpr static float_denorm_style has_denorm = denorm_absent;
    constexpr static bool has_denorm_loss = false;
    constexpr static float_round_style round_style = round_toward_zero;
    constexpr static bool is_iec559 = false;
    constexpr static bool is_bounded = true;
    constexpr static bool is_modulo = false;
    constexpr static int digits = 127;
    constexpr static int digits10 = 38;
    constexpr static int max_digits10 = 0;
    constexpr static int radix = 2;
    constexpr static int min_exponent = 0;
    constexpr static int min_exponent10 = 0;
    constexpr static int max_exponent = 0;
    constexpr static int max_exponent10 = 0;
    constexpr static bool traps = false;
    constexpr static bool tinyness_before = false;

    constexpr static mnstl::int128_t min() noexcept { return -(mnstl::int128_t{1} << 127); }
    constexpr static mnstl::int128_t lowest() noexcept { return min(); }
    constexpr static mnstl::int128_t max() noexcept {
        return mnstl::int128_t{std::numeric_limits<int64_t>::max(), std::numeric_limits<uint64_t>::max()};
    }
    constexpr static mnstl::int128_t epsilon() noexcept { return 0; }
    constexpr static mnstl::int128_t round_error() noexcept { return 0; }
    constexpr static mnstl::int128_t infinity() noexcept { return 0; }
    constexpr static mnstl::int128_t quiet_NaN() noexcept { return 0; }
    constexpr static mnstl::int128_t signaling_NaN() noexcept { return 0; }
    constexpr static mnstl::int128_t denorm_min() noexcept { return 0; }
};

template <>
class numeric_limits<mnstl::uint128_t> {
   public:
    constexpr static bool is_specialized = true;
    constexpr static bool is_signed = false;
    constexpr static bool is_integer = true;
    constexpr static bool is_exact = true;
    constexpr static bool has_infinity = false;
    constexpr static bool has_quiet_NaN = false;
    constexpr static bool has_signaling_NaN = false;
    constexpr static float_denorm_style has_denorm = denorm_absent;
    constexpr static bool has_denorm_loss = false;
    constexpr static float_round_style round_style = round_toward_zero;
    constexpr static bool is_iec559 = false;
    constexpr static bool is_bounded = true;
    constexpr static bool is_modulo = true;
    constexpr static int digits = 128;
    constexpr static int digits10 = 38;
    constexpr static int max_digits10 = 0;
    constexpr static int radix = 2;
    constexpr static int min_exponent = 0;
    constexpr static int min_exponent10 = 0;
    constexpr static int max_exponent = 0;
    constexpr static int max_exponent10 = 0;
    constexpr static bool traps = false;
    constexpr static bool tinyness_before = false;

    constexpr static mnstl::uint128_t min() noexcept { return 0; }
    constexpr static mnstl::uint128_t lowest() noexcept { return min(); }
    constexpr static mnstl::uint128_t max() noexcept {
        return mnstl::uint128_t{std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max()};
    }
    constexpr static mnstl::uint128_t epsilon() noexcept { return 0; }
    constexpr static mnstl::uint128_t round_error() noexcept { return 0; }
    constexpr static mnstl::uint128_t infinity() noexcept { return 0; }
    constexpr static mnstl::uint128_t quiet_NaN() noexcept { return 0; }
    constexpr static mnstl::uint128_t signaling_NaN() noexcept { return 0; }
    constexpr static mnstl::uint128_t denorm_min() noexcept { return 0; }
};
}  // namespace std
#endif  // !MNSTL_NATIVE_I128

#undef MNSTL_I128_SELF_IN_PLACE_OP
#undef MNSTL_I128_PRIMITIVE_IN_PLACE_OP

#endif  // MNSTL_EXT_NUMS_I128_HXX