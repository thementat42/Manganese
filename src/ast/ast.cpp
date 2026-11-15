#include <ast.hpp>
#include <core.hpp>
#include <lexer/token.hpp>
#include <semantic/type_context.hpp>
#include <utils/meta/target_info.hpp>
#include <utils/meta/type_names.hpp>
#include <utils/str_to_num.hpp>

namespace Manganese::ast {

std::string_view primitiveTypeToString(PrimitiveType prim) {
    using enum PrimitiveType;
    switch (prim) {
#define PRIMITIVE(type, name) \
    case type: return name##_str;
#include <ast/primitives.def>
#undef PRIMITIVE
    }
    ASSERT_UNREACHABLE("Invalid primitive type");
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