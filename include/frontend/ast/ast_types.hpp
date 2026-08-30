#ifndef MANGANESE_INCLUDE_FRONTEND_AST_AST_TYPES_HPP
#define MANGANESE_INCLUDE_FRONTEND_AST_AST_TYPES_HPP

#include <cstdint>
#include <frontend/ast/ast_base.hpp>
#include <string>
#include <utility>
#include <vector>

namespace Manganese::ast {

enum class TypeKind : std::uint8_t {
#define STMT(name)
#define EXPR(name)
#define TYPE(name) name,
#include <frontend/ast/ast.def>
#undef STMT
#undef EXPR
#undef TYPE
};

struct AggregateType final : public Type {
    std::vector<Type*> fieldTypes;

    explicit AggregateType(std::vector<Type*>&& _fieldTypes) noexcept :
        Type(TypeKind::AggregateType), fieldTypes(std::move(_fieldTypes)) {}
    MN_AST_STANDARD_INTERFACE;
};

struct ArrayType final : public Type {
    Type* elementType;
    Expression* lengthExpression;  // If not given, the length is inferred from the number of elements

    explicit ArrayType(Type* _elementType, Expression* _lengthExpr = nullptr) noexcept :
        Type(TypeKind::ArrayType), elementType(_elementType), lengthExpression(_lengthExpr) {}

    MN_AST_STANDARD_INTERFACE;
};

struct FunctionParameterType {
    bool isMutable;
    bool isVariadic;
    Type* type;
};

struct FunctionType final : public Type {
    std::vector<FunctionParameterType> parameterTypes;
    Type* returnType;

    FunctionType(std::vector<FunctionParameterType>&& _parameterTypes, Type* _returnType) noexcept :
        Type(TypeKind::FunctionType), parameterTypes(std::move(_parameterTypes)), returnType(_returnType) {}

    MN_AST_STANDARD_INTERFACE;
};

/**
 * represents the application of generic arguments to a base type, not represent the generic type itself
 */
struct GenericInstantiationType final : public Type {
    Type* baseType;  // some_function in `some_function@[T,U]`
    std::vector<Type*> typeParameters;  // T and U in `some_function@[T,U]`
    GenericInstantiationType(Type* _baseType, std::vector<Type*>&& _typeParameters) noexcept :
        Type(TypeKind::GenericInstantiationType), baseType(_baseType), typeParameters(std::move(_typeParameters)) {}

    MN_AST_STANDARD_INTERFACE;
};

struct PointerType final : public Type {
    Type* baseType;
    bool isMutable;

    PointerType(Type* _baseType, bool _isMutable) noexcept :
        Type(TypeKind::PointerType), baseType(_baseType), isMutable(_isMutable) {}

    MN_AST_STANDARD_INTERFACE;
};

struct ScopedType final : public Type {
    Type* scope;
    Type* type;

    ScopedType(Type* _qualifier, Type* _type) noexcept : Type(TypeKind::ScopedType), scope(_qualifier), type(_type) {}

    MN_AST_STANDARD_INTERFACE
};

struct SymbolType final : public Type {
    std::string name;

    explicit SymbolType(std::string&& _name, PrimitiveType_t prim = PrimitiveType_t::not_primitive) noexcept :
        Type(TypeKind::SymbolType, prim), name(std::move(_name)) {}
    MN_AST_STANDARD_INTERFACE;
};

struct TypeofType final : public Type {
    Expression* expression;

    explicit TypeofType(Expression* expr) noexcept :
        Type(TypeKind::TypeofType, PrimitiveType_t::not_primitive), expression(expr) {}

    MN_AST_STANDARD_INTERFACE;
};

}  // namespace Manganese::ast

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_AST_TYPES_HPP