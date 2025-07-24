#ifndef MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SEMANTIC_TYPE_HELPERS_H
#define MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SEMANTIC_TYPE_HELPERS_H

#include <frontend/ast.h>
#include <unordered_map>
#include <utils/number_utils.h>

namespace Manganese {

namespace semantic {

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

}  // namespace semantic

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SEMANTIC_TYPE_HELPERS_H