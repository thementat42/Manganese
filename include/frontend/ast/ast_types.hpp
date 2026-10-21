#ifndef MANGANESE_INCLUDE_FRONTEND_AST_AST_TYPES_HPP
#define MANGANESE_INCLUDE_FRONTEND_AST_AST_TYPES_HPP

#include <frontend/ast/ast_base.hpp>
#include <frontend/lexer/token.hpp>
#include <utility>
#include <utils/slice.hpp>
#include <utils/string_interner.hpp>

namespace Manganese::ast {
struct AggregateType final : public Type {
    utils::Slice<Type*> fieldTypes;

    explicit AggregateType(utils::Slice<Type*>&& _fieldTypes) noexcept :
        Type(TypeKind::AggregateType), fieldTypes(_fieldTypes) {}
    MN_AST_STANDARD_INTERFACE(AggregateType);
};

struct ArrayType final : public Type {
    Type* elementType;
    Expression* lengthExpression;

    ArrayType(Type* _elementType, Expression* _lengthExpr) noexcept :
        Type(TypeKind::ArrayType), elementType(_elementType), lengthExpression(_lengthExpr) {}

    MN_AST_STANDARD_INTERFACE(ArrayType);
};

struct FunctionParameterType {
    bool isMutable;
    bool isVariadic;
    Type* type;
};

struct FunctionType final : public Type {
    utils::Slice<FunctionParameterType> parameterTypes;
    Type* returnType;

    FunctionType(utils::Slice<FunctionParameterType>&& _parameterTypes, Type* _returnType) noexcept :
        Type(TypeKind::FunctionType), parameterTypes(std::move(_parameterTypes)), returnType(_returnType) {}

    MN_AST_STANDARD_INTERFACE(FunctionType);
};

/**
 * represents the application of generic arguments to a base type, not represent the generic type itself
 */
struct GenericInstantiationType final : public Type {
    Type* baseType;  // some_function in `some_function@[T,U]`
    utils::Slice<Type*> typeParameters;  // T and U in `some_function@[T,U]`

    GenericInstantiationType(Type* _baseType, utils::Slice<Type*>&& _typeParameters) noexcept :
        Type(TypeKind::GenericInstantiationType), baseType(_baseType), typeParameters(_typeParameters) {}

    MN_AST_STANDARD_INTERFACE(GenericInstantiationType);
};

struct IdentifierType final : public Type {
    utils::StringID name;

    explicit IdentifierType(utils::StringID _name, PrimitiveType prim = PrimitiveType::not_primitive) noexcept :
        Type(TypeKind::IdentifierType, prim), name(_name) {}
    MN_AST_STANDARD_INTERFACE(IdentifierType);
};

struct PointerType final : public Type {
    Type* baseType;
    bool isMutable;

    PointerType(Type* _baseType, bool _isMutable) noexcept :
        Type(TypeKind::PointerType), baseType(_baseType), isMutable(_isMutable) {}

    MN_AST_STANDARD_INTERFACE(PointerType);
};

struct ScopedType final : public Type {
    Type* scope;
    Type* type;

    ScopedType(Type* _scope, Type* _type) noexcept : Type(TypeKind::ScopedType), scope(_scope), type(_type) {}

    MN_AST_STANDARD_INTERFACE(ScopedType)
};

struct SliceType final : public Type {
    Type* elementType;
    explicit SliceType(Type* _elementType) noexcept : Type(TypeKind::SliceType), elementType(_elementType) {}

    MN_AST_STANDARD_INTERFACE(SliceType)
};

struct TypeofType final : public Type {
    Expression* expression;

    explicit TypeofType(Expression* expr) noexcept : Type(TypeKind::TypeofType), expression(expr) {}

    MN_AST_STANDARD_INTERFACE(TypeofType);
};

struct PoisonedType final : public Type {
    PoisonedType() noexcept : Type(TypeKind::PoisonedType) {}

    MN_AST_STANDARD_INTERFACE(PoisonedType);
};

}  // namespace Manganese::ast

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_AST_TYPES_HPP