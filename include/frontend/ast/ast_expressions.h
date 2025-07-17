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
    std::vector<ExpressionPtr_t> elements;
    TypePtr_t elementType;  // Optional, can be inferred from the elements

   public:
    ArrayLiteralExpression(std::vector<ExpressionPtr_t> elements_, TypePtr_t elementType_ = nullptr)
        : elements(std::move(elements_)), elementType(std::move(elementType_)) {}

    AST_STANDARD_INTERFACE;
    TypePtr_t getType() const override {
        return elementType ? TypePtr_t(elementType.get()) : TypePtr_t(new SymbolType("auto"));
    }
    ExpressionKind kind() const noexcept override { return ExpressionKind::ArrayLiteralExpression; }
};

class AssignmentExpression : public Expression {
   protected:
    ExpressionPtr_t assignee, value;
    lexer::TokenType op;

   public:
    AssignmentExpression(ExpressionPtr_t assignee_, lexer::TokenType op_, ExpressionPtr_t value_)
        : assignee(std::move(assignee_)), value(std::move(value_)), op(op_) {}

    AST_STANDARD_INTERFACE;
    TypePtr_t getType() const override {
        return assignee->getType();  // Assume the assignee's type is the result type
    }
    ExpressionKind kind() const noexcept override { return ExpressionKind::AssignmentExpression; }
};

class BinaryExpression : public Expression {
   protected:
    ExpressionPtr_t left, right;
    lexer::TokenType op;

   public:
    BinaryExpression(ExpressionPtr_t left_, lexer::TokenType op_, ExpressionPtr_t right_)
        : left(std::move(left_)), right(std::move(right_)), op(op_) {};

    AST_STANDARD_INTERFACE;
    TypePtr_t getType() const override {
        // Resolution is in the semantic analysis phase
        return TypePtr_t(new SymbolType("auto"));
    }
    ExpressionKind kind() const noexcept override { return ExpressionKind::BinaryExpression; }
};

class BoolLiteralExpression : public Expression {
   protected:
    bool value;

   public:
    explicit BoolLiteralExpression(const bool value_) : value(value_) {};

    AST_STANDARD_INTERFACE;
    TypePtr_t getType() const override {
        return TypePtr_t(new SymbolType("bool"));
    }
    ExpressionKind kind() const noexcept override { return ExpressionKind::BoolLiteralExpression; }
};

struct BundleInstantiationField {
    std::string name;
    ExpressionPtr_t value;

    BundleInstantiationField(std::string name_, ExpressionPtr_t value_)
        : name(std::move(name_)), value(std::move(value_)) {}
};

class BundleInstantiationExpression : public Expression {
   protected:
    std::string name;
    std::vector<TypePtr_t> genericTypes;
    std::vector<BundleInstantiationField> fields;

   public:
    BundleInstantiationExpression(std::string name_, std::vector<TypePtr_t> genericTypes_, std::vector<BundleInstantiationField> fields_)
        : name(std::move(name_)), genericTypes(std::move(genericTypes_)), fields(std::move(fields_)) {}

    AST_STANDARD_INTERFACE;
    TypePtr_t getType() const override;
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

    AST_STANDARD_INTERFACE;
    TypePtr_t getType() const override {
        return TypePtr_t(new SymbolType("char"));
    }
    ExpressionKind kind() const noexcept override { return ExpressionKind::CharLiteralExpression; }
};

class FunctionCallExpression : public Expression {
   protected:
    ExpressionPtr_t callee;
    std::vector<ExpressionPtr_t> arguments;

   public:
    FunctionCallExpression(ExpressionPtr_t callee_, std::vector<ExpressionPtr_t> arguments_)
        : callee(std::move(callee_)), arguments(std::move(arguments_)) {}

    AST_STANDARD_INTERFACE;
    TypePtr_t getType() const override {
        return TypePtr_t(new SymbolType("auto"));
    }
    ExpressionKind kind() const noexcept override { return ExpressionKind::FunctionCallExpression; }
};

class GenericExpression : public Expression {
   protected:
    ExpressionPtr_t identifier;
    std::vector<TypePtr_t> types;

   public:
    GenericExpression(ExpressionPtr_t identifier_, std::vector<TypePtr_t> types_)
        : identifier(std::move(identifier_)), types(std::move(types_)) {}

    /**
     * @brief Transfer ownership of the type parameters to the caller.
     * @details Used when a GenericExpression is part of a larger expression (e.g. a bundle instantiation)
     */
    std::vector<TypePtr_t> moveTypeParameters() { return std::move(types); }

    AST_STANDARD_INTERFACE;
    TypePtr_t getType() const override {
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

    AST_STANDARD_INTERFACE;
    TypePtr_t getType() const override {
        return TypePtr_t(new SymbolType("auto"));
    }
    ExpressionKind kind() const noexcept override { return ExpressionKind::IdentifierExpression; }
};

class IndexExpression : public Expression {
   protected:
    ExpressionPtr_t variable;
    ExpressionPtr_t index;

   public:
    IndexExpression(ExpressionPtr_t variable_, ExpressionPtr_t index_)
        : variable(std::move(variable_)), index(std::move(index_)) {}

    AST_STANDARD_INTERFACE;
    TypePtr_t getType() const override {
        return TypePtr_t(new SymbolType("auto"));
    }
    ExpressionKind kind() const noexcept override { return ExpressionKind::IndexExpression; }
};

class MemberAccessExpression : public Expression {
   protected:
    ExpressionPtr_t object;
    std::string property;

   public:
    MemberAccessExpression(ExpressionPtr_t object_, std::string property_)
        : object(std::move(object_)), property(std::move(property_)) {}

    AST_STANDARD_INTERFACE;
    TypePtr_t getType() const override {
        return TypePtr_t(new SymbolType("auto"));
    }
    ExpressionKind kind() const noexcept override { return ExpressionKind::MemberAccessExpression; }
};

class NumberLiteralExpression : public Expression {
   protected:
    number_t value;

   public:
    explicit NumberLiteralExpression(number_t value_) : value(value_) {};

    AST_STANDARD_INTERFACE;
    TypePtr_t getType() const override;
    ExpressionKind kind() const noexcept override { return ExpressionKind::NumberLiteralExpression; }
};

class PostfixExpression : public Expression {
   protected:
    ExpressionPtr_t left;
    lexer::TokenType op;

   public:
    PostfixExpression(ExpressionPtr_t left_, lexer::TokenType op_)
        : left(std::move(left_)), op(op_) {}

    AST_STANDARD_INTERFACE;
    TypePtr_t getType() const override {
        return TypePtr_t(new SymbolType("auto"));
    }
    ExpressionKind kind() const noexcept override { return ExpressionKind::PostfixExpression; }
};

class PrefixExpression : public Expression {
   protected:
    lexer::TokenType op;
    ExpressionPtr_t right;

   public:
    PrefixExpression(lexer::TokenType op_, ExpressionPtr_t right_)
        : op(op_), right(std::move(right_)) {}

    AST_STANDARD_INTERFACE;
    TypePtr_t getType() const override {
        // Don't infer the type, leave that to the semantic analysis phase
        return TypePtr_t(new SymbolType("auto"));
    }
    ExpressionKind kind() const noexcept override { return ExpressionKind::PrefixExpression; }
};

class ScopeResolutionExpression : public Expression {
   protected:
    ExpressionPtr_t scope;
    std::string element;

   public:
    ScopeResolutionExpression(ExpressionPtr_t scope_, std::string element_)
        : scope(std::move(scope_)), element(std::move(element_)) {}

    AST_STANDARD_INTERFACE;
    TypePtr_t getType() const override {
        return TypePtr_t(new SymbolType("auto"));
    }
    ExpressionKind kind() const noexcept override { return ExpressionKind::ScopeResolutionExpression; }
};

class StringLiteralExpression : public Expression {
   protected:
    std::string value;

   public:
    explicit StringLiteralExpression(const std::string& value_) : value(std::move(value_)) {};
    explicit StringLiteralExpression(const char* value_) : value(value_) {};

    AST_STANDARD_INTERFACE;
    TypePtr_t getType() const override {
        return TypePtr_t(new SymbolType("string"));
    }
    ExpressionKind kind() const noexcept override { return ExpressionKind::StringLiteralExpression; }
};

class TypeCastExpression : public Expression {
   protected:
    ExpressionPtr_t expression;
    TypePtr_t type;

   public:
    TypeCastExpression(ExpressionPtr_t expression_, TypePtr_t type_)
        : expression(std::move(expression_)), type(std::move(type_)) {}

    AST_STANDARD_INTERFACE;
    TypePtr_t getType() const override;
    ExpressionKind kind() const noexcept override { return ExpressionKind::TypeCastExpression; }
};

}  // namespace ast

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_AST_EXPRESSIONS_H