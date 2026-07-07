#ifndef MANGANESE_INCLUDE_FRONTEND_AST_AST_TYPES_HPP
#define MANGANESE_INCLUDE_FRONTEND_AST_AST_TYPES_HPP

#include <frontend/ast/ast_base.hpp>
#include <mnstl/number.hxx>
#include <vector>

namespace Manganese {

namespace ast {

enum class TypeKind : uint8_t {
#define STMT(name, str)
#define EXPR(name, str)
#define TYPE(name, str) name,
#include "ast.def"
#undef STMT
#undef EXPR
#undef TYPE
};

/**
 * e.g. aggregate {int, float}
 */
struct AggregateType final : public Type {
    std::vector<Type*> fieldTypes;

    explicit AggregateType(std::vector<Type*>&& fieldTypes_) noexcept :
        Type(TypeKind::AggregateType), fieldTypes(std::move(fieldTypes_)) {}
    AST_STANDARD_INTERFACE;
};

/**
 * e.g. int[], float[][], etc.
 */
struct ArrayType final : public Type {
    Type* elementType;
    Expression* lengthExpression;  // If not given, the length is inferred from the number of elements


    explicit ArrayType(Type* elementType_, Expression* lengthExpr_ = nullptr) noexcept :
        Type(TypeKind::ArrayType), elementType(elementType_), lengthExpression(lengthExpr_) {}

    AST_STANDARD_INTERFACE;
};

struct FunctionParameterType {
    bool isMutable;
    Type* type;

    FunctionParameterType(bool isMutable_, Type* type_) noexcept : isMutable(isMutable_), type(type_) {}
};

/**
 * e.g. func(int, int) -> bool
 */
struct FunctionType final : public Type {
    std::vector<FunctionParameterType> parameterTypes;
    Type* returnType;

    FunctionType(std::vector<FunctionParameterType>&& parameterTypes_, Type* returnType_) noexcept :
        Type(TypeKind::FunctionType), parameterTypes(std::move(parameterTypes_)), returnType(returnType_) {}

    AST_STANDARD_INTERFACE;
};

/**
 * @brief Represents the application of generic arguments to a base type.
 * e.g. some_function@[T, U](args)
 * It does not represent the generic type itself
 */
struct GenericType final : public Type {
    Type* baseType;  // some_function in `some_function@[T,U]`
    std::vector<Type*> typeParameters;  // T and U in `some_function@[T,U]`
    GenericType(Type* baseType_, std::vector<Type*>&& typeParameters_) noexcept :
        Type(TypeKind::GenericType), baseType(baseType_), typeParameters(std::move(typeParameters_)) {}

    AST_STANDARD_INTERFACE;
};

/**
 * ptr + any type
 */
struct PointerType final : public Type {
    Type* baseType;
    bool isMutable;

    PointerType(Type* baseType_, bool isMutable_) noexcept :
        Type(TypeKind::PointerType), baseType(baseType_), isMutable(isMutable_) {}

    AST_STANDARD_INTERFACE;
};

/**
 * e.g. T, int, etc.
 */
struct SymbolType final : public Type {
    std::string name;

    constexpr explicit SymbolType(std::string&& name_, PrimitiveType_t prim = PrimitiveType_t::not_primitive) noexcept :
        Type(TypeKind::SymbolType, prim), name(std::move(name_)) {}
    AST_STANDARD_INTERFACE;
    std::string getName() const noexcept { return name; }
};

}  // namespace ast

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_AST_TYPES_HPP