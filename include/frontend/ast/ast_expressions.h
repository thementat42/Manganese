/**
 * @file ast_expressions.h
 * @brief Defines AST node classes for various expressions
 *
 * This header declares the core expression node types used in the AST.
 * Each expression type (literals, binary, assignment, function call, etc.) is represented as a class inheriting from
 * Expression.
 *
 * ! The nodes are listed in alphabetical order
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

/**
 * @brief e.g. [1, 2, 3]
 */
class ArrayLiteralExpression : public Expression {
   protected:
    std::vector<ExpressionUPtr_t> elements;
    TypeSPtr_t elementType;  // Optional, can be inferred from the elements
    ExpressionUPtr_t lengthExpression = nullptr;

   public:
    ArrayLiteralExpression(std::vector<ExpressionUPtr_t> elements_, TypeSPtr_t elementType_ = nullptr) :
        elements(std::move(elements_)), elementType(std::move(elementType_)) {}

    AST_STANDARD_INTERFACE;
    ExpressionKind kind() const noexcept override { return ExpressionKind::ArrayLiteralExpression; }
};

/**
 * e.g. `foo = bar`, `baz *= 1`
 */
class AssignmentExpression : public Expression {
   protected:
    ExpressionUPtr_t assignee, value;
    lexer::TokenType op;

   public:
    AssignmentExpression(ExpressionUPtr_t assignee_, lexer::TokenType op_, ExpressionUPtr_t value_) :
        assignee(std::move(assignee_)), value(std::move(value_)), op(op_) {}

    AST_STANDARD_INTERFACE;
    ExpressionKind kind() const noexcept override { return ExpressionKind::AssignmentExpression; }
};

/**
 * @brief e.g. `a + b`, `x * y`
 */
class BinaryExpression : public Expression {
   protected:
    ExpressionUPtr_t left, right;
    lexer::TokenType op;

   public:
    BinaryExpression(ExpressionUPtr_t left_, lexer::TokenType op_, ExpressionUPtr_t right_) :
        left(std::move(left_)), right(std::move(right_)), op(op_) {};

    AST_STANDARD_INTERFACE;
    ExpressionKind kind() const noexcept override { return ExpressionKind::BinaryExpression; }
};

/**
 * @brief e.g. `true`, `false`
 */
class BoolLiteralExpression : public Expression {
   protected:
    bool value;

   public:
    explicit BoolLiteralExpression(const bool value_) : value(value_) {};

    AST_STANDARD_INTERFACE;
    ExpressionKind kind() const noexcept override { return ExpressionKind::BoolLiteralExpression; }
};

struct BundleInstantiationField {
    std::string name;
    ExpressionUPtr_t value;

    BundleInstantiationField(std::string name_, ExpressionUPtr_t value_) :
        name(std::move(name_)), value(std::move(value_)) {}
};

/**
 * @brief e.g. `Point3D{ x = 1, y = 2, z = 3 }`
 */
class BundleInstantiationExpression : public Expression {
   protected:
    std::string name;  // The name of the bundle type being instantiated
    std::vector<TypeSPtr_t> genericTypes;
    std::vector<BundleInstantiationField> fields;

   public:
    BundleInstantiationExpression(std::string name_, std::vector<TypeSPtr_t> genericTypes_,
                                  std::vector<BundleInstantiationField> fields_) :
        name(std::move(name_)), genericTypes(std::move(genericTypes_)), fields(std::move(fields_)) {}

    AST_STANDARD_INTERFACE;
    ExpressionKind kind() const noexcept override { return ExpressionKind::BundleInstantiationExpression; }
};

/**
 * @brief e.g. 'a', '\u1234', '\n'
 */
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
    ExpressionKind kind() const noexcept override { return ExpressionKind::CharLiteralExpression; }
};

/**
 * @brief e.g. `foo()`, `bar(1, 2, 3)`
 */
class FunctionCallExpression : public Expression {
   protected:
    ExpressionUPtr_t callee;
    std::vector<ExpressionUPtr_t> arguments;

   public:
    FunctionCallExpression(ExpressionUPtr_t callee_, std::vector<ExpressionUPtr_t> arguments_) :
        callee(std::move(callee_)), arguments(std::move(arguments_)) {}

    AST_STANDARD_INTERFACE;
    ExpressionKind kind() const noexcept override { return ExpressionKind::FunctionCallExpression; }
};

/**
 * e.g. `foo@[int, string]`
 */
class GenericExpression : public Expression {
   protected:
    ExpressionUPtr_t identifier;
    std::vector<TypeSPtr_t> types;

   public:
    GenericExpression(ExpressionUPtr_t identifier_, std::vector<TypeSPtr_t> types_) :
        identifier(std::move(identifier_)), types(std::move(types_)) {}

    /**
     * @brief Transfer ownership of the type parameters to the caller.
     * @details Used when a GenericExpression is part of a larger expression (e.g. a bundle instantiation)
     */
    std::vector<TypeSPtr_t> moveTypeParameters() { return std::move(types); }

    AST_STANDARD_INTERFACE;
    ExpressionKind kind() const noexcept override { return ExpressionKind::GenericExpression; }
};

/**
 * @brief e.g. `foo`, `bar`
 */
class IdentifierExpression : public Expression {
   protected:
    std::string value;

   public:
    explicit IdentifierExpression(const std::string& value_) : value(std::move(value_)) {}

    AST_STANDARD_INTERFACE;
    ExpressionKind kind() const noexcept override { return ExpressionKind::IdentifierExpression; }
};

/**
 * @brief e.g. `foo[0]`, `bar[1 + 2]`
 */
class IndexExpression : public Expression {
   protected:
    ExpressionUPtr_t variable;
    ExpressionUPtr_t index;

   public:
    IndexExpression(ExpressionUPtr_t variable_, ExpressionUPtr_t index_) :
        variable(std::move(variable_)), index(std::move(index_)) {}

    AST_STANDARD_INTERFACE;
    ExpressionKind kind() const noexcept override { return ExpressionKind::IndexExpression; }
};

/**
 * @brief e.g. `foo.bar`
 */
class MemberAccessExpression : public Expression {
   protected:
    ExpressionUPtr_t object;
    std::string property;

   public:
    MemberAccessExpression(ExpressionUPtr_t object_, std::string property_) :
        object(std::move(object_)), property(std::move(property_)) {}

    AST_STANDARD_INTERFACE;
    ExpressionKind kind() const noexcept override { return ExpressionKind::MemberAccessExpression; }
};

/**
 * @brief e.g. `42`, `3.14`, `-1`
 */
class NumberLiteralExpression : public Expression {
   protected:
    number_t value;

   public:
    explicit NumberLiteralExpression(number_t value_) : value(value_) {};

    AST_STANDARD_INTERFACE;
    ExpressionKind kind() const noexcept override { return ExpressionKind::NumberLiteralExpression; }
};

/**
 * @brief e.g. `foo++`, `bar--`
 */
class PostfixExpression : public Expression {
   protected:
    ExpressionUPtr_t left;
    lexer::TokenType op;

   public:
    PostfixExpression(ExpressionUPtr_t left_, lexer::TokenType op_) : left(std::move(left_)), op(op_) {}

    AST_STANDARD_INTERFACE;
    ExpressionKind kind() const noexcept override { return ExpressionKind::PostfixExpression; }
};

/**
 * @brief e.g. `++foo`, `--bar`
 */
class PrefixExpression : public Expression {
   protected:
    lexer::TokenType op;
    ExpressionUPtr_t right;

   public:
    PrefixExpression(lexer::TokenType op_, ExpressionUPtr_t right_) : op(op_), right(std::move(right_)) {}

    AST_STANDARD_INTERFACE;
    ExpressionKind kind() const noexcept override { return ExpressionKind::PrefixExpression; }
};

/**
 * @brief e.g. `Module::Element`
 */
class ScopeResolutionExpression : public Expression {
   protected:
    ExpressionUPtr_t scope;
    std::string element;

   public:
    ScopeResolutionExpression(ExpressionUPtr_t scope_, std::string element_) :
        scope(std::move(scope_)), element(std::move(element_)) {}

    AST_STANDARD_INTERFACE;
    ExpressionKind kind() const noexcept override { return ExpressionKind::ScopeResolutionExpression; }
};

/**
 * @brief e.g. `"Hello, World!"`, `"Line 1\nLine 2"`
 */
class StringLiteralExpression : public Expression {
   protected:
    std::string value;

   public:
    explicit StringLiteralExpression(const std::string& value_) : value(std::move(value_)) {};
    explicit StringLiteralExpression(const char* value_) : value(value_) {};

    AST_STANDARD_INTERFACE;
    ExpressionKind kind() const noexcept override { return ExpressionKind::StringLiteralExpression; }
};

/**
 * @brief e.g. `foo as Bar`
 */
class TypeCastExpression : public Expression {
   protected:
    ExpressionUPtr_t originalValue;
    TypeSPtr_t targetType;

   public:
    TypeCastExpression(ExpressionUPtr_t originalValue_, TypeSPtr_t targetType_) :
        originalValue(std::move(originalValue_)), targetType(std::move(targetType_)) {}

    AST_STANDARD_INTERFACE;
    ExpressionKind kind() const noexcept override { return ExpressionKind::TypeCastExpression; }
};

}  // namespace ast

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_AST_EXPRESSIONS_H