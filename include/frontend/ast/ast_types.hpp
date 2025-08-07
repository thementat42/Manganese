/**
 * @file ast_statements.h
 * @brief Contains AST node definitions for various types annotations in the Manganese frontend.
 *
 * This header declares the core type node types used in the AST.
 * Each type (Symbols, Arrays, etc.) is represented as a class inheriting from Statement.
 *
 * ! The nodes are listed in alphabetical order.
 */

#ifndef MANGANESE_INCLUDE_FRONTEND_AST_AST_TYPES_HPP
#define MANGANESE_INCLUDE_FRONTEND_AST_AST_TYPES_HPP

#include <frontend/ast/ast_base.hpp>
#include <utils/number_utils.hpp>

namespace Manganese {

namespace ast {

enum class TypeKind {
    AggregateType,
    ArrayType,
    FunctionType,
    GenericType,
    PointerType,
    SymbolType
};

/**
 * e.g. int[], float[][], etc.
 */

/**
 * e.g. aggregate {int, float}
 */
class AggregateType : public Type {
   protected:
    std::vector<TypeSPtr_t> fieldTypes;

   public:
    explicit AggregateType(std::vector<TypeSPtr_t> fieldTypes_) : fieldTypes(std::move(fieldTypes_)) {}
    AST_STANDARD_INTERFACE;
    TypeKind kind() const noexcept override { return TypeKind::AggregateType; }

    bool operator==(const Type& other) const noexcept override;
};
class ArrayType : public Type {
   protected:
    TypeSPtr_t elementType;
    ExpressionUPtr_t lengthExpression;  // If not given, the length is inferred from the number of elements

   public:
    /**
     * @param elementType_ The type of the elements in the array
     */
    explicit ArrayType(TypeSPtr_t elementType_, ExpressionUPtr_t lengthExpr_ = nullptr) :
        elementType(std::move(elementType_)), lengthExpression(std::move(lengthExpr_)) {}

    AST_STANDARD_INTERFACE;
    TypeKind kind() const noexcept override { return TypeKind::ArrayType; };

    bool operator==(const Type& other) const noexcept override;
};

struct FunctionParameterType {
    bool isConst;
    TypeSPtr_t type;

    FunctionParameterType(bool isConst_, TypeSPtr_t type_) : isConst(isConst_), type(std::move(type_)) {}

    bool operator==(const FunctionParameterType& other) const noexcept {
        return isConst == other.isConst && type.get() == other.type.get();
    }
};

/**
 * e.g. func(int, int) -> bool
 */
class FunctionType : public Type {
   protected:
    std::vector<FunctionParameterType> parameterTypes;
    TypeSPtr_t returnType;

   public:
    FunctionType(std::vector<FunctionParameterType> parameterTypes_, TypeSPtr_t returnType_) :
        parameterTypes(std::move(parameterTypes_)), returnType(std::move(returnType_)) {}

    AST_STANDARD_INTERFACE;
    TypeKind kind() const noexcept override { return TypeKind::FunctionType; };

    bool operator==(const Type& other) const noexcept override;
};

/**
 * @brief Represents the application of generic arguments to a base type.
 * e.g. some_function@[T, U](args)
 * It does not represent the generic type itself
 */
class GenericType : public Type {
   protected:
    TypeSPtr_t baseType;  // some_function in `some_function@[T,U]`
    std::vector<TypeSPtr_t> typeParameters;  // T and U in `some_function@[T,U]`
   public:
    GenericType(TypeSPtr_t baseType_, std::vector<TypeSPtr_t> typeParameters_) :
        baseType(std::move(baseType_)), typeParameters(std::move(typeParameters_)) {}

    AST_STANDARD_INTERFACE;
    TypeKind kind() const noexcept override { return TypeKind::GenericType; };

    bool operator==(const Type& other) const noexcept override;
};

/**
 * ptr + any type
 */
class PointerType : public Type {
   protected:
    TypeSPtr_t baseType;

   public:
    explicit PointerType(TypeSPtr_t baseType_) : baseType(std::move(baseType_)) {}

    AST_STANDARD_INTERFACE;
    TypeKind kind() const noexcept override { return TypeKind::PointerType; };

    bool operator==(const Type& other) const noexcept override;
};

/**
 * e.g. T, int, etc.
 */
class SymbolType : public Type {
   protected:
    std::string name;

   public:
    explicit SymbolType(std::string name_) : name(std::move(name_)) {}
    AST_STANDARD_INTERFACE;
    std::string getName() const noexcept { return name; }
    TypeKind kind() const noexcept override { return TypeKind::SymbolType; };

    bool operator==(const Type& other) const noexcept override;
};

bool isPrimitiveType(const TypeSPtr_t& type);

bool isPrimitiveType(const Type* type);
}  // namespace ast

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_AST_TYPES_HPP