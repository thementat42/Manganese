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

/**
 * e.g. int[], float[][], etc.
 */
class ArrayType : public Type {
   protected:
    std::unique_ptr<Type> elementType;
    ExpressionPtr lengthExpression;  // Optional length specification (otherwise, should be inferred based on the number of elements)

   public:
    /**
     * @param elementType_ The type of the elements in the array
     */
    explicit ArrayType(std::unique_ptr<Type> elementType_, ExpressionPtr lengthExpr_ = nullptr)
        : elementType(std::move(elementType_)), lengthExpression(std::move(lengthExpr_)) {}

    NODE_OVERRIDES;
};

struct FunctionParameterType {
    bool isConst;
    TypePtr type;

    FunctionParameterType(bool isConst_, TypePtr type_) : isConst(isConst_), type(std::move(type_)) {}
};

/**
 * e.g. func(int, int) -> bool
 */
class FunctionType : public Type {
    protected:
     std::vector<FunctionParameterType> parameterTypes;
     TypePtr returnType;
     public:
     FunctionType(std::vector<FunctionParameterType> parameterTypes_, TypePtr returnType_)
     : parameterTypes(std::move(parameterTypes_)), returnType(std::move(returnType_)) {}

     NODE_OVERRIDES;
};

/**
 * e.g. [T, U]
 */
class GenericType : public Type {
    protected:
    std::unique_ptr<Type> baseType;  // The base type to which the generics are applied
    std::vector<TypePtr> typeParameters;  // The generic type parameters
    public:
    /**
     * @param baseType_ The base type to which the generics are applied
     * @param typeParameters_ The generic type parameters
     */
    GenericType(std::unique_ptr<Type> baseType_, std::vector<TypePtr> typeParameters_)
        : baseType(std::move(baseType_)), typeParameters(std::move(typeParameters_)) {}

    NODE_OVERRIDES;
};

/**
 * ptr + any type
 */
class PointerType: public Type {
    protected:
    TypePtr baseType;
    public:
    explicit PointerType(TypePtr baseType_): baseType(std::move(baseType_)) {}

    NODE_OVERRIDES;
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

    const std::string& getName() const { return name; }
    NODE_OVERRIDES;
};

} // namespace ast

} // namespace Manganese


#endif // MANGANESE_INCLUDE_FRONTEND_AST_AST_TYPES_H