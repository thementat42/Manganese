#ifndef MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SEMANTIC_TYPE_HELPERS_HPP
#define MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SEMANTIC_TYPE_HELPERS_HPP

#include <frontend/ast.hpp>
#include <frontend/lexer/token.hpp>
#include <utils/number_utils.hpp>

#include <functional>
#include <unordered_map>
#include <utility>
#include <string>
#include <array>

// Custom hashes have to be in namespace std
namespace std {
template <>
struct hash<std::pair<std::string, std::string>> {
    std::size_t operator()(const std::pair<std::string, std::string>& p) const noexcept {
        std::size_t h1 = std::hash<std::string>{}(p.first);
        std::size_t h2 = std::hash<std::string>{}(p.second);
        // Combine the two hashes
        return h1 ^ (h2 << 1);
    }
};
}  // namespace std

namespace Manganese {

namespace semantic {

// Fallback: prefer float64 > float32 > int64 > int32 > int16 > int8 > uint64 > uint32 > uint16 > uint8
const std::array<std::string, 10> fallbackTypeOrder = {float64_str, float32_str, int64_str,  int32_str,  int16_str,
                                                       int8_str,    uint64_str,  uint32_str, uint16_str, uint8_str};

const std::unordered_map<std::string, std::string> validImplicitConversions = {
    // int8 promotions
    {int8_str, int16_str},
    {int8_str, int32_str},
    {int8_str, int64_str},

    // int16 promotions
    {int16_str, int32_str},
    {int16_str, int64_str},

    // int32 promotions
    {int32_str, int64_str},

    // uint8 promotions
    {uint8_str, uint16_str},
    {uint8_str, uint32_str},
    {uint8_str, uint64_str},

    // uint16 promotions
    {uint16_str, uint32_str},
    {uint16_str, uint64_str},

    // uint32 promotions
    {uint32_str, uint64_str},

    // float32 promotions
    {float32_str, float64_str},

    // Unsigned to signed promotions
    // These are only to larger bit widths
    {uint8_str, int16_str},
    {uint8_str, int32_str},
    {uint8_str, int64_str},
    {uint16_str, int32_str},
    {uint16_str, int64_str},
    {uint32_str, int64_str},

};

const std::unordered_map<std::string, std::string> validImplicitConversionsWithWarnings = {
    // These will raise a warning
    // int64 demotions
    {int64_str, int32_str},
    {int64_str, int16_str},
    {int64_str, int8_str},

    // int32 demotions
    {int32_str, int16_str},
    {int32_str, int8_str},
    // int16 demotions
    {int16_str, int8_str},
    {int16_str, uint8_str},

    // uint64 demotions
    {uint64_str, uint32_str},
    {uint64_str, uint16_str},
    {uint64_str, uint8_str},

    // uint32 demotions
    {uint32_str, uint16_str},
    {uint32_str, uint8_str},

    // uint16 demotions
    {uint16_str, uint8_str},

    // float64 demotions
    {float64_str, float32_str},

    // Unsigned to signed (since unsigned types can hold larger values)
    // However, promoting to a larger bit width (e.g. uint8 to int16 is fine)
    {uint64_str, int64_str},
    {uint32_str, int32_str},
    {uint16_str, int16_str},
    {uint8_str, int8_str},

    // Unsigned to signed of lower bit width
    // Loss of negative values + outside of range values
    {uint32_str, int8_str},
    {uint32_str, int16_str},
    {uint64_str, int8_str},
    {uint64_str, int16_str},
    {uint64_str, int32_str},

    // signed to unsigned of lower bit width
    // Loss of negative values + outside of range values
    {int64_str, uint8_str},
    {int64_str, uint16_str},
    {int64_str, uint32_str},
    {int32_str, uint8_str},
    {int32_str, uint16_str},
    {uint16_str, int8_str},

    // Signed to unsigned (loss of any negative values)
    {int8_str, uint8_str},
    {int8_str, uint16_str},
    {int8_str, uint32_str},
    {int8_str, uint64_str},
    {int16_str, uint16_str},
    {int16_str, uint32_str},
    {int16_str, uint64_str},
    {int32_str, uint32_str},
    {int32_str, uint64_str},
    {int64_str, uint64_str},
};

const std::unordered_map<std::pair<std::string, std::string>, std::string> numericTypePromotionTable
    = {{{int8_str, int8_str}, float64_str},       {{int16_str, uint32_str}, float64_str},
       {{int32_str, uint16_str}, float64_str},    {{uint16_str, uint32_str}, uint32_str},
       {{int16_str, int64_str}, int64_str},       {{int8_str, uint32_str}, float64_str},
       {{uint16_str, uint16_str}, uint16_str},    {{uint8_str, uint16_str}, float64_str},
       {{int8_str, int32_str}, float64_str},      {{uint32_str, uint32_str}, float64_str},
       {{int16_str, float32_str}, float32_str},   {{int16_str, float64_str}, float64_str},
       {{int64_str, uint16_str}, int64_str},      {{uint32_str, float32_str}, float64_str},
       {{uint64_str, uint64_str}, float64_str},   {{uint32_str, float64_str}, float64_str},
       {{int64_str, uint8_str}, int64_str},       {{int32_str, uint32_str}, float64_str},
       {{int32_str, int64_str}, int64_str},       {{int16_str, int16_str}, int16_str},
       {{int8_str, uint16_str}, int32_str},       {{uint8_str, uint32_str}, uint32_str},
       {{int64_str, float64_str}, float64_str},   {{uint8_str, float32_str}, float32_str},
       {{int64_str, int64_str}, int64_str},       {{int16_str, uint8_str}, int16_str},
       {{int32_str, int32_str}, int32_str},       {{int16_str, uint64_str}, float64_str},
       {{int8_str, int64_str}, float64_str},      {{int8_str, int64_str}, int64_str},
       {{int16_str, int64_str}, float64_str},     {{int16_str, int32_str}, int32_str},
       {{uint16_str, uint64_str}, float64_str},   {{int8_str, uint64_str}, float64_str},
       {{int8_str, float32_str}, float32_str},    {{int8_str, uint16_str}, float64_str},
       {{uint16_str, float32_str}, float32_str},  {{int32_str, int64_str}, float64_str},
       {{int32_str, uint16_str}, int32_str},      {{int8_str, int16_str}, int16_str},
       {{uint8_str, float64_str}, float64_str},   {{int32_str, uint32_str}, int64_str},
       {{int32_str, int32_str}, float64_str},     {{float64_str, float64_str}, float64_str},
       {{int64_str, uint32_str}, int64_str},      {{int8_str, int8_str}, int8_str},
       {{int16_str, uint16_str}, float64_str},    {{int16_str, int16_str}, float64_str},
       {{int16_str, uint16_str}, int32_str},      {{int64_str, uint16_str}, float64_str},
       {{int16_str, uint32_str}, int64_str},      {{int32_str, uint8_str}, float64_str},
       {{int16_str, int32_str}, float64_str},     {{uint8_str, uint32_str}, float64_str},
       {{int8_str, uint8_str}, float64_str},      {{uint32_str, uint32_str}, uint32_str},
       {{uint8_str, uint64_str}, uint64_str},     {{int64_str, uint32_str}, float64_str},
       {{uint64_str, float64_str}, float64_str},  {{int8_str, int32_str}, int32_str},
       {{float32_str, float64_str}, float64_str}, {{int32_str, float64_str}, float64_str},
       {{int8_str, uint32_str}, int64_str},       {{int8_str, float64_str}, float64_str},
       {{uint64_str, float32_str}, float64_str},  {{uint64_str, uint64_str}, uint64_str},
       {{int64_str, float32_str}, float64_str},   {{int64_str, uint64_str}, float64_str},
       {{uint8_str, uint8_str}, uint8_str},       {{uint32_str, uint64_str}, float64_str},
       {{int64_str, uint8_str}, float64_str},     {{int8_str, int16_str}, float64_str},
       {{uint16_str, uint16_str}, float64_str},   {{uint32_str, uint64_str}, uint64_str},
       {{int16_str, uint8_str}, float64_str},     {{int32_str, uint64_str}, float64_str},
       {{int64_str, int64_str}, float64_str},     {{uint8_str, uint64_str}, float64_str},
       {{int8_str, uint8_str}, int16_str},        {{int32_str, uint8_str}, int32_str},
       {{uint8_str, uint16_str}, uint16_str},     {{uint16_str, uint64_str}, uint64_str},
       {{float32_str, float32_str}, float32_str}, {{int32_str, float32_str}, float64_str},
       {{uint16_str, float64_str}, float64_str},  {{uint16_str, uint32_str}, float64_str},
       {{uint8_str, uint8_str}, float64_str}};

}  // namespace semantic

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SEMANTIC_TYPE_HELPERS_HPP