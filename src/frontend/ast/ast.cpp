

/**
 * @file ast.cpp
 * @brief Implementation of some AST node methods.
 *
 * @note Most of the AST functions are declared inline in the ast header files
 * @see include/frontend/ast/
 * @note This file only implements the functions that are too big/complex to be in a header file
 */
#include <frontend/ast.h>
#include <global_macros.h>

#include <memory>
#include <string>
#include <utility>

namespace Manganese {
namespace ast {

TypePtr BundleInstantiationExpression::getType() const {
    if (genericTypes.empty()) {
        return TypePtr(new SymbolType(name));
    }
    std::string typeName = name + "@[";
    for (size_t i = 0; i < genericTypes.size(); ++i) {
        typeName += genericTypes[i]->toString();
        if (i < genericTypes.size() - 1) [[likely]] {
            typeName += ", ";
        }
    }
    typeName += "]";
    return TypePtr(new SymbolType(typeName));
}

TypePtr NumberLiteralExpression::getType() const {
    if (std::holds_alternative<int8_t>(value)) return std::make_unique<SymbolType>("int8");
    if (std::holds_alternative<uint8_t>(value)) return std::make_unique<SymbolType>("uint8");
    if (std::holds_alternative<int16_t>(value)) return std::make_unique<SymbolType>("int16");
    if (std::holds_alternative<uint16_t>(value)) return std::make_unique<SymbolType>("uint16");
    if (std::holds_alternative<int32_t>(value)) return std::make_unique<SymbolType>("int32");
    if (std::holds_alternative<uint32_t>(value)) return std::make_unique<SymbolType>("uint32");
    if (std::holds_alternative<int64_t>(value)) return std::make_unique<SymbolType>("int64");
    if (std::holds_alternative<uint64_t>(value)) return std::make_unique<SymbolType>("uint64");
    if (std::holds_alternative<float>(value)) return std::make_unique<SymbolType>("float32");
    if (std::holds_alternative<double>(value)) return std::make_unique<SymbolType>("float64");
    ASSERT_UNREACHABLE("Unknown number type in NumberLiteralExpression");
}

TypePtr TypeCastExpression::getType() const {
    // Return a copy of the target type
    if (type) {
        if (auto symbolType = dynamic_cast<const SymbolType*>(type.get())) {
            return std::make_unique<SymbolType>(symbolType->getName());
        } else if (auto arrayType = dynamic_cast<const ArrayType*>(type.get())) {
            // For array types, we need to copy the element type too
            return std::make_unique<ArrayType>(std::make_unique<SymbolType>(arrayType->toString()));
        }
        // Default fallback for other type classes
        return std::make_unique<SymbolType>(type->toString());
    }
    // If no type is specified, return auto
    return std::make_unique<SymbolType>("auto");
}

VariableDeclarationStatement::VariableDeclarationStatement(
    bool isConst_, std::string name_, Visibility visibility_, ExpressionPtr _value, TypePtr _type)
    : isConst(isConst_),
      name(std::move(name_)),
      visibility(visibility_),
      value(std::move(_value)),
      type(std::move(_type)) {
    if (type == nullptr && value) {
        type = value->getType();
    }
}
}  // namespace ast
}  // namespace Manganese
