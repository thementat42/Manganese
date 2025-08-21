/**
 * @file ast.cpp
 * @brief Implementation of some AST node methods.
 *
 * @note This file is implementations of any AST node methods which are get too complex to be inline
 */
#include <frontend/ast.hpp>
#include <global_macros.hpp>
#include <string>
#include <unordered_set>

namespace Manganese {
namespace ast {

const std::unordered_set<std::string> primitiveTypeNames
    = {int8_str,   int16_str,   int32_str,   int64_str, uint8_str,  uint16_str, uint32_str,
       uint64_str, float32_str, float64_str, bool_str,  string_str, char_str};

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

// ===== Operator== overloads for Type nodes =====

auto typePtrsEqual = [](const TypeSPtr_t& lhs, const TypeSPtr_t& rhs) {
    if (!lhs || !rhs) { return lhs.get() == rhs.get(); }
    return *lhs == *rhs;
};

bool AggregateType::operator==(const Type& other) const noexcept {
    if (other.kind() != kind()) { return false; }
    const auto& otherAggregateType = static_cast<const AggregateType&>(other);
    if (fieldTypes.size() != otherAggregateType.fieldTypes.size()) { return false; }
    return std::equal(fieldTypes.begin(), fieldTypes.end(), otherAggregateType.fieldTypes.begin(), typePtrsEqual);
}

bool ArrayType::operator==(const Type& other) const noexcept {
    if (other.kind() != kind()) { return false; }
    const auto& otherArrayType = static_cast<const ArrayType&>(other);
    return typePtrsEqual(elementType, otherArrayType.elementType);
}

bool FunctionType::operator==(const Type& other) const noexcept {
    if (other.kind() != kind()) { return false; }
    const auto& otherFunctionType = static_cast<const FunctionType&>(other);
    if (!typePtrsEqual(returnType, otherFunctionType.returnType)) { return false; }
    if (parameterTypes.size() != otherFunctionType.parameterTypes.size()) { return false; }

    return std::equal(parameterTypes.begin(), parameterTypes.end(), otherFunctionType.parameterTypes.begin(),
                      [](const auto& lhs, const auto& rhs) { return typePtrsEqual(lhs.type, rhs.type); });
}

bool GenericType::operator==(const Type& other) const noexcept {
    if (other.kind() != kind()) { return false; }
    const auto& otherGenericType = static_cast<const GenericType&>(other);
    if (!typePtrsEqual(baseType, otherGenericType.baseType)) { return false; }
    if (typeParameters.size() != otherGenericType.typeParameters.size()) { return false; }
    return std::equal(typeParameters.begin(), typeParameters.end(), otherGenericType.typeParameters.begin(),
                      typePtrsEqual);
}

bool PointerType::operator==(const Type& other) const noexcept {
    if (other.kind() != kind()) { return false; }
    const auto& otherPointerType = static_cast<const PointerType&>(other);
    return typePtrsEqual(baseType, otherPointerType.baseType) && isMutable == otherPointerType.isMutable;
}

bool SymbolType::operator==(const Type& other) const noexcept {
    if (other.kind() != kind()) { return false; }
    const auto& otherSymbolType = static_cast<const SymbolType&>(other);
    return name == otherSymbolType.name;
}
}  // namespace ast
}  // namespace Manganese