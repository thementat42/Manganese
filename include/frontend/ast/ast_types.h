#ifndef MANGANESE_INCLUDE_FRONTEND_AST_AST_TYPES_H
#define MANGANESE_INCLUDE_FRONTEND_AST_AST_TYPES_H




#include <frontend/ast/ast_base.h>

namespace Manganese {

namespace ast {

class SymbolType : public Type {  // e.g. T, int, etc.
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

class ArrayType : public Type {  // e.g. int[]
   protected:
    std::unique_ptr<Type> elementType;
    ExpressionPtr lengthExpression;  // Optional length specification (otherwise, should be inferred based on the number of elements)

   public:
    /**
     * @param elementType_ The type of the elements in the array
     */
    explicit ArrayType(std::unique_ptr<Type> elementType_, ExpressionPtr lengthExpr_ = nullptr)
        : elementType(std::move(elementType_)), lengthExpression(std::move(lengthExpr_)) {}
    const Expression* getLengthExpression() const { return lengthExpression.get(); }
    bool hasFixedLength() const {
        return lengthExpression != nullptr;
    }
    NODE_OVERRIDES;
};

class GenericType : public Type {  // e.g. [T, U]
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

} // namespace ast

} // namespace Manganese


#endif // MANGANESE_INCLUDE_FRONTEND_AST_AST_TYPES_H