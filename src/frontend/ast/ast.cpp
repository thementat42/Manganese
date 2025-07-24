/**
 * @file ast.cpp
 * @brief Implementation of some AST node methods.
 *
 * @note This file is implementations of any AST node methods which are get too complex to be inline
 */
#include <frontend/ast.h>
#include <global_macros.h>

#include <memory>
#include <string>
#include <unordered_set>
#include <utility>

namespace Manganese {
namespace ast {

const std::unordered_set<std::string> primitiveTypeNames = {
    int8_str, int16_str, int32_str, int64_str,
    uint8_str, uint16_str, uint32_str, uint64_str,
    float32_str, float64_str, bool_str, string_str, char_str};

std::string visibilityToString(const Visibility& visibility) noexcept_if_release {
    switch (visibility) {
        case Visibility::Public:
            return "public ";
            break;
        case Visibility::ReadOnly:
            return "readonly ";
            break;
        case Visibility::Private:
            return "private ";
            break;
        default:
            ASSERT_UNREACHABLE("Invalid visibility");
    }
}

bool isPrimitiveType(const TypeSPtr_t& type) {
    if (!type) return false;
    if (type->kind() != TypeKind::SymbolType) return false;
    const auto& symbolType = static_cast<const SymbolType*>(type.get());
    return primitiveTypeNames.contains(symbolType->getName());
}

bool isPrimitiveType(const Type* type) {
    if (!type) return false;
    if (type->kind() != TypeKind::SymbolType) return false;
    const auto& symbolType = static_cast<const SymbolType*>(type);
    return primitiveTypeNames.contains(symbolType->getName());
}
}  // namespace ast
}  // namespace Manganese
