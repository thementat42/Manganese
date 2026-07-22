#ifndef MANGANESE_INCLUDE_FRONTEND_AST_AST_TYPES_HPP
#define MANGANESE_INCLUDE_FRONTEND_AST_AST_TYPES_HPP

#include <frontend/ast/ast_base.hpp>
#include <mnstl/number.hxx>
#include <string>
#include <utility>
#include <vector>

namespace Manganese {

namespace ast {

enum class TypeKind : uint8_t {
#define STMT(name, str)
#define EXPR(name, str)
#define TYPE(name, str) name,
#include <frontend/ast/ast.def>
#undef STMT
#undef EXPR
#undef TYPE
};

/**
 * e.g. aggregate {int, float}
 */
struct AggregateType final : public Type {
    std::vector<Type*> fieldTypes;

    explicit AggregateType(std::vector<Type*>&& _fieldTypes) noexcept :
        Type(TypeKind::AggregateType), fieldTypes(std::move(_fieldTypes)) {}
    MN_AST_STANDARD_INTERFACE;
};

/**
 * e.g. int[], float[][], etc.
 */
struct ArrayType final : public Type {
    Type* elementType;
    Expression* lengthExpression;  // If not given, the length is inferred from the number of elements

    explicit ArrayType(Type* _elementType, Expression* _lengthExpr = nullptr) noexcept :
        Type(TypeKind::ArrayType), elementType(_elementType), lengthExpression(_lengthExpr) {}

    MN_AST_STANDARD_INTERFACE;
};

struct FunctionParameterType {
    bool isMutable;
    Type* type;

    FunctionParameterType(bool _isMutable, Type* _type) noexcept : isMutable(_isMutable), type(_type) {}
};

/**
 * e.g. func(int, int) -> bool
 */
struct FunctionType final : public Type {
    std::vector<FunctionParameterType> parameterTypes;
    Type* returnType;

    FunctionType(std::vector<FunctionParameterType>&& _parameterTypes, Type* _returnType) noexcept :
        Type(TypeKind::FunctionType), parameterTypes(std::move(_parameterTypes)), returnType(_returnType) {}

    MN_AST_STANDARD_INTERFACE;
};

/**
 * @brief Represents the application of generic arguments to a base type.
 * e.g. some_function@[T, U](args)
 * It does not represent the generic type itself
 */
struct GenericType final : public Type {
    Type* baseType;  // some_function in `some_function@[T,U]`
    std::vector<Type*> typeParameters;  // T and U in `some_function@[T,U]`
    GenericType(Type* _baseType, std::vector<Type*>&& _typeParameters) noexcept :
        Type(TypeKind::GenericType), baseType(_baseType), typeParameters(std::move(_typeParameters)) {}

    MN_AST_STANDARD_INTERFACE;
};

/**
 * ptr + any type
 */
struct PointerType final : public Type {
    Type* baseType;
    bool isMutable;

    PointerType(Type* _baseType, bool _isMutable) noexcept :
        Type(TypeKind::PointerType), baseType(_baseType), isMutable(_isMutable) {}

    MN_AST_STANDARD_INTERFACE;
};

/**
 * e.g. T, int, etc.
 */
struct SymbolType final : public Type {
    std::string name;

    constexpr explicit SymbolType(std::string&& _name, PrimitiveType_t prim = PrimitiveType_t::not_primitive) noexcept :
        Type(TypeKind::SymbolType, prim), name(std::move(_name)) {}
    MN_AST_STANDARD_INTERFACE;
};

struct TypeofType final : public Type {
    Expression* expression;

    explicit TypeofType(Expression* expr) noexcept :
        Type(TypeKind::TypeofType, PrimitiveType_t::not_primitive), expression(expr) {}

    MN_AST_STANDARD_INTERFACE;
};

}  // namespace ast

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_AST_TYPES_HPP