#include <array>
#include <core.hpp>
#include <cstddef>
#include <frontend/ast.hpp>
#include <frontend/lexer/token.hpp>
#include <frontend/semantic/type_context.hpp>
#include <utils/str_to_num.hpp>
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

AttributeKind attributeKindFromString(std::string_view name) {
    using enum AttributeKind;
    if (name == "extern") { return Extern; }
    if (name == "likely") { return Likely; }
    if (name == "unlikely") { return Unlikely; }
    if (name == "packed") { return Packed; }
    if (name == "align") { return Align; }
    if (name == "noreturn") { return Noreturn; }
    if (name == "deprecated") { return Deprecated; }

    return Unknown;
}

const Attribute* getAttribute(utils::Slice<Attribute> attributes, AttributeKind kind) {
    for (const auto& attribute : attributes) {
        if (attribute.kind == kind) { return &attribute; }
    }
    return nullptr;
}
const Attribute* getAttribute(const Statement* statement, AttributeKind kind) {
    return getAttribute(statement->attributes, kind);
}

}  // namespace Manganese::ast