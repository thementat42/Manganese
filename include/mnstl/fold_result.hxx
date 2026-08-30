#ifndef MNSTL_FOLD_RESULT
#define MNSTL_FOLD_RESULT 1

#include <concepts>
#include <core.hpp>
#include <cstdint>
#include <mnstl/number.hxx>
#include <optional>
#include <string>
#include <string_view>

namespace mnstl {

class fold_result_t {
   public:
    enum class held_type : std::uint8_t {
        Boolean = 'b',
        Character = 'c',
        Number = 'n',
        String = 's',
        Void = 'v'
    };

   private:
    held_type _held;
    union {
        bool _bool;
        char32_t _char;
        number_t _number;
        std::string_view _string;
    };

   public:
    constexpr fold_result_t() noexcept : _held(held_type::Void) {}
    explicit fold_result_t(bool boolean) noexcept : _held(held_type::Boolean), _bool(boolean) {}
    explicit fold_result_t(char32_t character) noexcept : _held(held_type::Character), _char(character) {}
    explicit fold_result_t(number_t number) noexcept : _held(held_type::Number), _number(number) {}
    explicit fold_result_t(std::string_view string) noexcept : _held(held_type::String), _string(string) {}

    fold_result_t& operator=(bool boolean) noexcept { return *this = fold_result_t(boolean); }
    fold_result_t& operator=(char32_t character) noexcept { return *this = fold_result_t(character); }
    fold_result_t& operator=(number_t number) noexcept { return *this = fold_result_t(number); }
    fold_result_t& operator=(const std::string& string) noexcept {
        return *this = fold_result_t(std::string_view{string});
    }
    fold_result_t& operator=(std::string&&) = delete;  // can't have a view into a temporary
    fold_result_t& operator=(std::string_view string_view) noexcept { return *this = fold_result_t(string_view); }

    fold_result_t(const fold_result_t&) noexcept = default;
    fold_result_t& operator=(const fold_result_t&) noexcept = default;
    fold_result_t(fold_result_t&&) noexcept = default;
    fold_result_t& operator=(fold_result_t&&) noexcept = default;

    ~fold_result_t() noexcept = default;

    held_type held_type() const noexcept { return _held; }
    bool has_value() const noexcept { return _held != held_type::Void; }
    bool is_bool() const noexcept { return _held == held_type::Boolean; }
    bool is_char() const noexcept { return _held == held_type::Character; }
    bool is_number() const noexcept { return _held == held_type::Number; }
    bool is_string() const noexcept { return _held == held_type::String; }

    std::optional<bool> boolean() const noexcept {
        return _held == held_type::Boolean ? std::make_optional<bool>(_bool) : std::nullopt;
    }

    std::optional<char32_t> character() const noexcept {
        return _held == held_type::Character ? std::make_optional<char32_t>(_char) : std::nullopt;
    }

    std::optional<number_t> number() const noexcept {
        return _held == held_type::Number ? std::make_optional<number_t>(_number) : std::nullopt;
    }

    std::optional<std::string_view> string() const noexcept {
        return _held == held_type::String ? std::make_optional<std::string_view>(_string) : std::nullopt;
    }

    template <class T>
        requires(std::same_as<T, bool> || std::same_as<T, char32_t> || std::same_as<T, number_t>
                 || std::same_as<T, std::string_view>)
    std::optional<T> value() const noexcept {
        if constexpr (std::same_as<T, bool>) {
            return boolean();
        } else if constexpr (std::same_as<T, char32_t>) {
            return character();
        } else if constexpr (std::same_as<T, number_t>) {
            return number();
        } else if constexpr (std::same_as<T, std::string_view>) {
            return string();
        } else {
            manganese_unreachable();
        }
    }

    // unchecked functions are faster (no branching) but can be UB
    bool boolean_unchecked() const noexcept { return _bool; }
    char32_t character_unchecked() const noexcept { return _char; }
    number_t number_unchecked() const noexcept { return _number; }
    std::string_view string_unchecked() const noexcept { return _string; }

    template <class T>
        requires(std::same_as<T, bool> || std::same_as<T, char32_t> || std::same_as<T, number_t>
                 || std::same_as<T, std::string_view>)
    constexpr T value_unchecked() const noexcept {
        if constexpr (std::same_as<T, bool>) {
            return _bool;
        } else if constexpr (std::same_as<T, char32_t>) {
            return _char;
        } else if constexpr (std::same_as<T, number_t>) {
            return _number;
        } else if constexpr (std::same_as<T, std::string_view>) {
            return _string;
        } else {
            manganese_unreachable();
        }
    }
};

}  // namespace mnstl

#endif  // MNSTL_FOLD_RESULT
