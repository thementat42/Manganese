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

std::string visibilityToString(const Visibility& visibility) noexcept_if_release {
    switch (visibility) {
        case Visibility::Public: return "public ";
        case Visibility::ReadOnly: return "readonly ";
        case Visibility::Private: return "private ";
        default: ASSERT_UNREACHABLE("Invalid visibility");
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

// ===== Operator== overloads for Type nodes =====

auto typePtrsEqual = [](const TypeSPtr_t& lhs, const TypeSPtr_t& rhs) { return lhs.get() == rhs.get(); };

bool ArrayType::operator==(const Type& other) const noexcept {
    if (other.kind() != kind()) { return false; }
    const auto& otherArrayType = static_cast<const ArrayType&>(other);
    return elementType.get() == otherArrayType.elementType.get();
}

bool BundleType::operator==(const Type& other) const noexcept {
    if (other.kind() != kind()) { return false; }
    const auto& otherBundleType = static_cast<const BundleType&>(other);
    if (fieldTypes.size() != otherBundleType.fieldTypes.size()) { return false; }
    return std::equal(fieldTypes.begin(), fieldTypes.end(), otherBundleType.fieldTypes.begin(), typePtrsEqual);
}

bool FunctionType::operator==(const Type& other) const noexcept {
    if (other.kind() != kind()) { return false; }
    const auto& otherFunctionType = static_cast<const FunctionType&>(other);
    if (otherFunctionType.returnType.get() != returnType.get()) { return false; }
    if (otherFunctionType.parameterTypes.size() != parameterTypes.size()) { return false; }

    return std::equal(parameterTypes.begin(), parameterTypes.end(), otherFunctionType.parameterTypes.begin());
}

bool GenericType::operator==(const Type& other) const noexcept {
    if (other.kind() != kind()) { return false; }
    const auto& otherGenericType = static_cast<const GenericType&>(other);
    if (baseType.get() != otherGenericType.baseType.get()) { return false; }
    if (typeParameters.size() != otherGenericType.typeParameters.size()) { return false; }
    return std::equal(typeParameters.begin(), typeParameters.end(), otherGenericType.typeParameters.begin(),
                      typePtrsEqual);
}

bool PointerType::operator==(const Type& other) const noexcept {
    if (other.kind() != kind()) { return false; }
    const auto& otherPointerType = static_cast<const PointerType&>(other);
    return baseType.get() == otherPointerType.baseType.get();
}

bool SymbolType::operator==(const Type& other) const noexcept {
    if (other.kind() != kind()) { return false; }
    const auto& otherSymbolType = static_cast<const SymbolType&>(other);
    return name == otherSymbolType.name;
}
}  // namespace ast
}  // namespace Manganese