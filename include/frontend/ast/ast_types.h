/**
 * @file ast_statements.h
 * @brief Contains AST node definitions for various types annotations in the Manganese frontend.
 *
 * This header declares the core type node types used in the AST.
 * Each type (Symbols, Arrays, etc.) is represented as a class inheriting from Statement.
 *
 * ! The nodes are listed in alphabetical order.
 */

#ifndef MANGANESE_INCLUDE_FRONTEND_AST_AST_TYPES_H
#define MANGANESE_INCLUDE_FRONTEND_AST_AST_TYPES_H

#include <frontend/ast/ast_base.h>

namespace Manganese {

namespace ast {

enum class TypeKind {
    ArrayType,
    FunctionType,
    GenericType,
    PointerType,
    SymbolType
};

/**
 * e.g. int[], float[][], etc.
 */
class ArrayType : public Type {
   protected:
    std::unique_ptr<Type> elementType;
    ExpressionPtr_t lengthExpression;  // Optional length specification (otherwise, should be inferred based on the number of elements)

   public:
    /**
     * @param elementType_ The type of the elements in the array
     */
    explicit ArrayType(std::unique_ptr<Type> elementType_, ExpressionPtr_t lengthExpr_ = nullptr)
        : elementType(std::move(elementType_)), lengthExpression(std::move(lengthExpr_)) {}

    AST_STANDARD_INTERFACE;
    TypeKind kind() const noexcept override { return TypeKind::ArrayType; };
};

struct FunctionParameterType {
    bool isConst;
    TypePtr_t type;

    FunctionParameterType(bool isConst_, TypePtr_t type_) : isConst(isConst_), type(std::move(type_)) {}
};

/**
 * e.g. func(int, int) -> bool
 */
class FunctionType : public Type {
   protected:
    std::vector<FunctionParameterType> parameterTypes;
    TypePtr_t returnType;

   public:
    FunctionType(std::vector<FunctionParameterType> parameterTypes_, TypePtr_t returnType_)
        : parameterTypes(std::move(parameterTypes_)), returnType(std::move(returnType_)) {}

    AST_STANDARD_INTERFACE;
    TypeKind kind() const noexcept override { return TypeKind::FunctionType; };
};

/**
 * @brief Represents the application of generic arguments to a base type.
 * e.g. some_function@[T, U](); some_bundle@[T, U]
 * It does not represent the generic type itself
 */
class GenericType : public Type {
   protected:
    TypePtr_t baseType;       // The base type to which the generics are applied
    std::vector<TypePtr_t> typeParameters;  // The generic type parameters
   public:
    /**
     * @param baseType_ The base type to which the generics are applied
     * @param typeParameters_ The generic type parameters
     */
    GenericType(std::unique_ptr<Type> baseType_, std::vector<TypePtr_t> typeParameters_)
        : baseType(std::move(baseType_)), typeParameters(std::move(typeParameters_)) {}

    AST_STANDARD_INTERFACE;
    TypeKind kind() const noexcept override { return TypeKind::GenericType; };
};

/**
 * ptr + any type
 */
class PointerType : public Type {
   protected:
    TypePtr_t baseType;

   public:
    explicit PointerType(TypePtr_t baseType_) : baseType(std::move(baseType_)) {}

    AST_STANDARD_INTERFACE;
    TypeKind kind() const noexcept override { return TypeKind::PointerType; };
};

/**
 * e.g. T, int, etc.
 */
class SymbolType : public Type {
   protected:
    std::string name;

   public:
    /**
     * @param name_ The name of the type
     */
    explicit SymbolType(std::string name_) : name(std::move(name_)) {}
    AST_STANDARD_INTERFACE;
    std::string getName() const noexcept { return name; }
    TypeKind kind() const noexcept override { return TypeKind::SymbolType; };
};

}  // namespace ast

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_AST_TYPES_H