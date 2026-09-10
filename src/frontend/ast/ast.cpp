#include <array>
#include <core.hpp>
#include <cstddef>
#include <frontend/ast/ast_expressions.hpp>
#include <frontend/lexer/token.hpp>
#include <frontend/semantic/type_context.hpp>
#include <string_view>
#include <utils/target_info.hpp>
#include <utils/type_names.hpp>

namespace Manganese::ast {

std::string_view primitiveTypeToString(PrimitiveType prim) {
    constexpr static std::array primitiveNames
        = {int8_str,   uint8_str,   int16_str,   uint16_str,
           int32_str,  uint32_str,  int64_str,   uint64_str,
           int128_str, uint128_str, float32_str, float64_str,
           char_str,   string_str,  bool_str,    decltype(bool_str){"not primitive"}};
    const auto index = static_cast<std::size_t>(prim);
    if (index >= primitiveNames.size()) { ASSERT_UNREACHABLE("Invalid primitive type"); }
    return primitiveNames[index];
}

}  // namespace Manganese::ast