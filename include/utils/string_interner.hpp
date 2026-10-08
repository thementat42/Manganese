#ifndef MANGANESE_INCLUDE_UTILS_STRING_INTERNER_HPP
#define MANGANESE_INCLUDE_UTILS_STRING_INTERNER_HPP 1

#include <cstdint>
#include <deque>
#include <format>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Manganese::utils {

using StringID = std::uint64_t;

struct OptionalStringID {
    StringID id = static_cast<StringID>(-1);

    constexpr bool has_value() const noexcept { return id != static_cast<StringID>(-1); }
    constexpr explicit operator bool() const noexcept { return has_value(); }
    constexpr StringID operator*() const noexcept { return id; }
};

class StringInterner {
    // deque guarantees pointer stability
    std::deque<std::string> _strings;
    std::unordered_map<std::string_view, StringID, std::hash<std::string_view>, std::equal_to<>> _str_to_id;

   public:
    StringID intern(std::string&& str) {
        if (auto it = _str_to_id.find(str); it != _str_to_id.end()) { return it->second; }
        _strings.push_back(std::move(str));
        StringID id = _strings.size() - 1;
        _str_to_id[_strings.back()] = id;

        return id;
    }

    StringID intern(std::string_view str) {
        if (auto it = _str_to_id.find(str); it != _str_to_id.end()) { return it->second; }
        _strings.emplace_back(str);
        StringID id = _strings.size() - 1;
        _str_to_id[_strings.back()] = id;

        return id;
    }

    template <class... Args>
    StringID intern(std::format_string<Args...> str, Args&&... args) {
        return intern(std::format(str, std::forward<Args>(args)...));
    }

    [[nodiscard]] bool has(std::string_view str) const noexcept { return _str_to_id.contains(str); }

    [[nodiscard]] OptionalStringID get_ID(std::string_view str) const noexcept {
        if (auto it = _str_to_id.find(str); it != _str_to_id.end()) { return OptionalStringID{.id = it->second}; }
        return OptionalStringID{};
    }

    [[nodiscard]] std::string_view get_view(StringID id) const noexcept { return _strings[id]; }
    [[nodiscard]] std::string get_copy(StringID id) const { return std::string{_strings[id]}; }
};

}  // namespace Manganese::utils

#endif  // MANGANESE_INCLUDE_UTILS_STRING_INTERNER_HPP
