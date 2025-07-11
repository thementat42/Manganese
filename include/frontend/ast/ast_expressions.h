/**
 * @file ast_expressions.h
 * @brief Defines AST node classes for various expressions
 *
 * This header declares the core expression node types used in the AST.
 * Each expression type (literals, binary, assignment, function call, etc.) is represented as a class inheriting from Expression.
 *
 * ! The nodes are listed in alphabetical order
 *
 * Each expression node provides a getType() method for type inference or annotation
 */

#ifndef MANGANESE_INCLUDE_FRONTEND_AST_AST_EXPRESSIONS_H
#define MANGANESE_INCLUDE_FRONTEND_AST_AST_EXPRESSIONS_H

#include <frontend/ast/ast_base.h>
#include <frontend/ast/ast_types.h>

namespace Manganese {

namespace ast {

enum class ExpressionKind {
    ArrayLiteralExpression,
    AssignmentExpression,
    BinaryExpression,
    BoolLiteralExpression,
    BundleInstantiationExpression,
    CharLiteralExpression,
    FunctionCallExpression,
    GenericExpression,
    IdentifierExpression,
    IndexExpression,
    MemberAccessExpression,
    NumberLiteralExpression,
    PostfixExpression,
    PrefixExpression,
    ScopeResolutionExpression,
    StringLiteralExpression,
    TypeCastExpression
};

class ArrayLiteralExpression : public Expression {
   protected:
    std::vector<ExpressionPtr> elements;
    TypePtr elementType;  // Optional, can be inferred from the elements

   public:
    ArrayLiteralExpression(std::vector<ExpressionPtr> elements_, TypePtr elementType_ = nullptr)
        : elements(std::move(elements_)), elementType(std::move(elementType_)) {}

    NODE_OVERRIDES;
    TypePtr getType() const override {
        return elementType ? TypePtr(elementType.get()) : TypePtr(new SymbolType("auto"));
    }
    ExpressionKind kind() const noexcept override { return ExpressionKind::ArrayLiteralExpression; }
};

class AssignmentExpression : public Expression {
   protected:
    ExpressionPtr assignee, value;
    lexer::TokenType op;

   public:
    AssignmentExpression(ExpressionPtr assignee_, lexer::TokenType op_, ExpressionPtr value_)
        : assignee(std::move(assignee_)), value(std::move(value_)), op(op_) {}

    NODE_OVERRIDES;
    TypePtr getType() const override {
        return assignee->getType();  // Assume the assignee's type is the result type
    }
    ExpressionKind kind() const noexcept override { return ExpressionKind::AssignmentExpression; }
};

class BinaryExpression : public Expression {
   protected:
    ExpressionPtr left, right;
    lexer::TokenType op;

   public:
    BinaryExpression(ExpressionPtr left_, lexer::TokenType op_, ExpressionPtr right_)
        : left(std::move(left_)), right(std::move(right_)), op(op_) {};

    NODE_OVERRIDES;
    TypePtr getType() const override {
        // Resolution is in the semantic analysis phase
        return TypePtr(new SymbolType("auto"));
    }
    ExpressionKind kind() const noexcept override { return ExpressionKind::BinaryExpression; }
};

class BoolLiteralExpression : public Expression {
   protected:
    bool value;

   public:
    explicit BoolLiteralExpression(const bool value_) : value(value_) {};

    NODE_OVERRIDES;
    TypePtr getType() const override {
        return TypePtr(new SymbolType("bool"));
    }
    ExpressionKind kind() const noexcept override { return ExpressionKind::BoolLiteralExpression; }
};

struct BundleInstantiationField {
    std::string name;
    ExpressionPtr value;

    BundleInstantiationField(std::string name_, ExpressionPtr value_)
        : name(std::move(name_)), value(std::move(value_)) {}
};

class BundleInstantiationExpression : public Expression {
   protected:
    std::string name;
    std::vector<TypePtr> genericTypes;
    std::vector<BundleInstantiationField> fields;

   public:
    BundleInstantiationExpression(std::string name_, std::vector<TypePtr> genericTypes_, std::vector<BundleInstantiationField> fields_)
        : name(std::move(name_)), genericTypes(std::move(genericTypes_)), fields(std::move(fields_)) {}

    NODE_OVERRIDES;
    TypePtr getType() const override;
    ExpressionKind kind() const noexcept override { return ExpressionKind::BundleInstantiationExpression; }
};

class CharLiteralExpression : public Expression {
   protected:
    char32_t value;

   public:
    /**
     * @param value_ The character value of the expression (char32_t)
     */
    explicit CharLiteralExpression(char32_t value_) : value(value_) {};
    explicit CharLiteralExpression(char value_) : value(static_cast<char32_t>(value_)) {};

    NODE_OVERRIDES;
    TypePtr getType() const override {
        return TypePtr(new SymbolType("char"));
    }
    ExpressionKind kind() const noexcept override { return ExpressionKind::CharLiteralExpression; }
};

class FunctionCallExpression : public Expression {
   protected:
    ExpressionPtr callee;
    std::vector<ExpressionPtr> arguments;

   public:
    FunctionCallExpression(ExpressionPtr callee_, std::vector<ExpressionPtr> arguments_)
        : callee(std::move(callee_)), arguments(std::move(arguments_)) {}

    NODE_OVERRIDES;
    TypePtr getType() const override {
        return TypePtr(new SymbolType("auto"));
    }
    ExpressionKind kind() const noexcept override { return ExpressionKind::FunctionCallExpression; }
};

class GenericExpression : public Expression {
   protected:
    ExpressionPtr identifier;
    std::vector<TypePtr> types;

   public:
    GenericExpression(ExpressionPtr identifier_, std::vector<TypePtr> types_)
        : identifier(std::move(identifier_)), types(std::move(types_)) {}

    /**
     * @brief Transfer ownership of the type parameters to the caller.
     * @details Used when a GenericExpression is part of a larger expression (e.g. a bundle instantiation)
     */
    std::vector<TypePtr> moveTypeParameters() { return std::move(types); }

    NODE_OVERRIDES;
    TypePtr getType() const override {
        // The type of a generic expression is the type of the identifier expression
        return identifier->getType();
    }
    ExpressionKind kind() const noexcept override { return ExpressionKind::GenericExpression; }
};

class IdentifierExpression : public Expression {
   protected:
    std::string value;

   public:
    explicit IdentifierExpression(const std::string& value_) : value(std::move(value_)) {}

    NODE_OVERRIDES;
    TypePtr getType() const override {
        return TypePtr(new SymbolType("auto"));
    }
    ExpressionKind kind() const noexcept override { return ExpressionKind::IdentifierExpression; }
};

class IndexExpression : public Expression {
   protected:
    ExpressionPtr variable;
    ExpressionPtr index;

   public:
    IndexExpression(ExpressionPtr variable_, ExpressionPtr index_)
        : variable(std::move(variable_)), index(std::move(index_)) {}

    NODE_OVERRIDES;
    TypePtr getType() const override {
        return TypePtr(new SymbolType("auto"));
    }
    ExpressionKind kind() const noexcept override { return ExpressionKind::IndexExpression; }
};

class MemberAccessExpression : public Expression {
   protected:
    ExpressionPtr object;
    std::string property;

   public:
    MemberAccessExpression(ExpressionPtr object_, std::string property_)
        : object(std::move(object_)), property(std::move(property_)) {}

    NODE_OVERRIDES;
    TypePtr getType() const override {
        return TypePtr(new SymbolType("auto"));
    }
    ExpressionKind kind() const noexcept override { return ExpressionKind::MemberAccessExpression; }
};

class NumberLiteralExpression : public Expression {
   protected:
    number_t value;

   public:
    explicit NumberLiteralExpression(number_t value_) : value(value_) {};

    NODE_OVERRIDES;
    TypePtr getType() const override;
    ExpressionKind kind() const noexcept override { return ExpressionKind::NumberLiteralExpression; }
};

class PostfixExpression : public Expression {
   protected:
    ExpressionPtr left;
    lexer::TokenType op;

   public:
    PostfixExpression(ExpressionPtr left_, lexer::TokenType op_)
        : left(std::move(left_)), op(op_) {}

    NODE_OVERRIDES;
    TypePtr getType() const override {
        return TypePtr(new SymbolType("auto"));
    }
    ExpressionKind kind() const noexcept override { return ExpressionKind::PostfixExpression; }
};

class PrefixExpression : public Expression {
   protected:
    lexer::TokenType op;
    ExpressionPtr right;

   public:
    PrefixExpression(lexer::TokenType op_, ExpressionPtr right_)
        : op(op_), right(std::move(right_)) {}

    NODE_OVERRIDES;
    TypePtr getType() const override {
        // Don't infer the type, leave that to the semantic analysis phase
        return TypePtr(new SymbolType("auto"));
    }
    ExpressionKind kind() const noexcept override { return ExpressionKind::PrefixExpression; }
};

class ScopeResolutionExpression : public Expression {
   protected:
    ExpressionPtr scope;
    std::string element;

   public:
    ScopeResolutionExpression(ExpressionPtr scope_, std::string element_)
        : scope(std::move(scope_)), element(std::move(element_)) {}

    NODE_OVERRIDES;
    TypePtr getType() const override {
        return TypePtr(new SymbolType("auto"));
    }
    ExpressionKind kind() const noexcept override { return ExpressionKind::ScopeResolutionExpression; }
};

class StringLiteralExpression : public Expression {
   protected:
    std::string value;

   public:
    explicit StringLiteralExpression(const std::string& value_) : value(std::move(value_)) {};
    explicit StringLiteralExpression(const char* value_) : value(value_) {};

    NODE_OVERRIDES;
    TypePtr getType() const override {
        return TypePtr(new SymbolType("string"));
    }
    ExpressionKind kind() const noexcept override { return ExpressionKind::StringLiteralExpression; }
};

class TypeCastExpression : public Expression {
   protected:
    ExpressionPtr expression;
    TypePtr type;

   public:
    TypeCastExpression(ExpressionPtr expression_, TypePtr type_)
        : expression(std::move(expression_)), type(std::move(type_)) {}

    NODE_OVERRIDES;
    TypePtr getType() const override;
    ExpressionKind kind() const noexcept override { return ExpressionKind::TypeCastExpression; }
};

}  // namespace ast

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_AST_EXPRESSIONS_H