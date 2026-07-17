#ifndef MNSTL_SAFE_CMP
#define MNSTL_SAFE_CMP 1
#include <mnstl/ext_num_config.hxx>

namespace mnstl {

template <Integral T, Integral U>
constexpr bool safe_equal(T t, U u) noexcept {
    if constexpr (SignedIntegral<T> == SignedIntegral<U>) {
        return t == u;
    } else if constexpr (SignedIntegral<T>) {
        return t >= 0 && mnstl_make_unsigned_t<T>(t) == u;
    } else {
        return u >= 0 && mnstl_make_unsigned_t<U>(u) == t;
    }
}

template <Integral T, Integral U>
constexpr bool safe_not_equal(T t, U u) noexcept {
    return !safe_equal(t, u);
}

template <Integral T, Integral U>
constexpr bool safe_less(T t, U u) noexcept {
    if constexpr (SignedIntegral<T> == SignedIntegral<U>) {
        return t < u;
    } else if constexpr (SignedIntegral<T>) {
        return t < 0 || mnstl_make_unsigned_t<T>(t) < u;
    } else {
        return u < 0 || t < mnstl_make_unsigned_t<U>(u);
    }
}
template <Integral T, Integral U>
constexpr bool safe_greater(T t, U u) noexcept {
    // t > u is the same as u < t
    return safe_less(u, t);
}

template <Integral T, Integral U>
constexpr bool safe_less_equal(T t, U u) noexcept {
    // t <= u is the same as !(u < t)
    return !safe_less(u, t);
}

template <Integral T, Integral U>
constexpr bool safe_greater_equal(T t, U u) noexcept {
    // t >= u is the same as !(t < u)
    return !safe_less(t, u);
}

}  // namespace mnstl

#endif  // MNSTL_SAFE_CMP