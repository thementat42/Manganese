/**
 * @file ast_expressions.hpp
 * @brief Defines AST node classes for various expressions
 *
 * This header declares the core expression node types used in the AST.
 * Each expression type (literals, binary, assignment, function call, etc.) is represented as a class inheriting from
 * Expression.
 *
 * ! The nodes are listed in alphabetical order
 */

#ifndef MANGANESE_INCLUDE_FRONTEND_AST_AST_EXPRESSIONS_HPP
#define MANGANESE_INCLUDE_FRONTEND_AST_AST_EXPRESSIONS_HPP

#include <frontend/ast/ast_base.hpp>
#include <frontend/ast/ast_types.hpp>

// TODO? If expressions, Switch Expressions, Variable Declarations as expressions (so let x = 3; evaluates to 3i32)

namespace Manganese {

namespace ast {

enum class ExpressionKind {
    AggregateInstantiationExpression,
    AggregateLiteralExpression,
    ArrayLiteralExpression,
    AssignmentExpression,
    BinaryExpression,
    BoolLiteralExpression,
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

struct AggregateInstantiationField {
    std::string name;
    ExpressionUPtr_t value;

    AggregateInstantiationField(std::string name_, ExpressionUPtr_t value_) :
        name(std::move(name_)), value(std::move(value_)) {}
};

/**
 * @brief e.g. `Point3D{ x = 1, y = 2, z = 3 }`
 */
class AggregateInstantiationExpression final : public Expression {
   public:
    std::string name;  // The name of the aggregate type being instantiated
    std::vector<TypeSPtr_t> genericTypes;
    std::vector<AggregateInstantiationField> fields;

    constexpr AggregateInstantiationExpression(std::string name_, std::vector<TypeSPtr_t> genericTypes_,
                                               std::vector<AggregateInstantiationField> fields_) : Expression(ExpressionKind::AggregateInstantiationExpression),
        name(std::move(name_)), genericTypes(std::move(genericTypes_)), fields(std::move(fields_)) {}

    AST_STANDARD_INTERFACE;
};

/**
 * @brief represents a sequence of elements of different types (like Rust's tuple)
 */
class AggregateLiteralExpression final : public Expression {
   public:
    std::vector<ExpressionUPtr_t> elements;
    explicit AggregateLiteralExpression(std::vector<ExpressionUPtr_t> elements_) : Expression(ExpressionKind::AggregateLiteralExpression), elements(std::move(elements_)) {}
    AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. [1, 2, 3]
 */
class ArrayLiteralExpression final : public Expression {
   public:
    std::vector<ExpressionUPtr_t> elements;
    TypeSPtr_t elementType;  // Optional, can be inferred from the elements
    ExpressionUPtr_t lengthExpression = nullptr;

    ArrayLiteralExpression(std::vector<ExpressionUPtr_t> elements_, TypeSPtr_t elementType_ = nullptr) : Expression(ExpressionKind::ArrayLiteralExpression),
        elements(std::move(elements_)), elementType(std::move(elementType_)) {}

    AST_STANDARD_INTERFACE;
};

/**
 * e.g. `foo = bar`, `baz *= 1`
 */
class AssignmentExpression final : public Expression {
   public:
    ExpressionUPtr_t assignee;  // The thing being assigned to (foo in foo = bar)
    ExpressionUPtr_t value;  // The value being assigned (bar in foo = bar)
    lexer::TokenType op;

    AssignmentExpression(ExpressionUPtr_t assignee_, lexer::TokenType op_, ExpressionUPtr_t value_) : Expression(ExpressionKind::AssignmentExpression),
        assignee(std::move(assignee_)), value(std::move(value_)), op(op_) {}

    AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. `a + b`, `x * y`
 */
class BinaryExpression final : public Expression {
   public:
    ExpressionUPtr_t left, right;
    lexer::TokenType op;

    BinaryExpression(ExpressionUPtr_t left_, lexer::TokenType op_, ExpressionUPtr_t right_) : Expression(ExpressionKind::BinaryExpression),
        left(std::move(left_)), right(std::move(right_)), op(op_) {};

    AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. `true`, `false`
 */
class BoolLiteralExpression final : public Expression {
   public:
    bool value;

    constexpr explicit BoolLiteralExpression(const bool value_) : Expression(ExpressionKind::BoolLiteralExpression), value(value_) {};

    AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. 'a', '\u1234', '\n'
 */
class CharLiteralExpression final : public Expression {
   public:
    char32_t value;

    /**
     * @param value_ The character value of the expression (char32_t)
     */
    constexpr explicit CharLiteralExpression(char32_t value_) : Expression(ExpressionKind::CharLiteralExpression), value(value_) {};
    constexpr explicit CharLiteralExpression(char value_) : Expression(ExpressionKind::CharLiteralExpression), value(static_cast<char32_t>(value_)) {};

    AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. `foo()`, `bar(1, 2, 3)`
 */
class FunctionCallExpression final : public Expression {
   public:
    ExpressionUPtr_t callee;
    std::vector<ExpressionUPtr_t> arguments;

    FunctionCallExpression(ExpressionUPtr_t callee_, std::vector<ExpressionUPtr_t> arguments_) : Expression(ExpressionKind::FunctionCallExpression),
        callee(std::move(callee_)), arguments(std::move(arguments_)) {}

    AST_STANDARD_INTERFACE;
};

/**
 * e.g. `foo@[int, string]`
 */
class GenericExpression final : public Expression {
   public:
    ExpressionUPtr_t identifier;
    std::vector<TypeSPtr_t> types;

    GenericExpression(ExpressionUPtr_t identifier_, std::vector<TypeSPtr_t> types_) :Expression(ExpressionKind::GenericExpression),
        identifier(std::move(identifier_)), types(std::move(types_)) {}

    /**
     * @brief Transfer ownership of the type parameters to the caller.
     * @details Used when a GenericExpression is part of a larger expression (e.g. an aggregate instantiation)
     */
    std::vector<TypeSPtr_t> moveTypeParameters() { return std::move(types); }

    AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. `foo`, `bar`
 */
class IdentifierExpression final : public Expression {
   public:
    std::string value;

    constexpr explicit IdentifierExpression(const std::string& value_) : Expression(ExpressionKind::IdentifierExpression), value(std::move(value_)) {}

    AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. `foo[0]`, `bar[1 + 2]`
 */
class IndexExpression final : public Expression {
   public:
    ExpressionUPtr_t variable;
    ExpressionUPtr_t index;

    IndexExpression(ExpressionUPtr_t variable_, ExpressionUPtr_t index_) :Expression(ExpressionKind::IndexExpression),
        variable(std::move(variable_)), index(std::move(index_)) {}

    AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. `foo.bar`
 */
class MemberAccessExpression final : public Expression {
   public:
    ExpressionUPtr_t object;
    std::string property;

    MemberAccessExpression(ExpressionUPtr_t object_, std::string property_) : Expression(ExpressionKind::MemberAccessExpression),
        object(std::move(object_)), property(std::move(property_)) {}

    AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. `42`, `3.14`, `-1`
 */
class NumberLiteralExpression final : public Expression {
   public:
    number_t value;

    constexpr explicit NumberLiteralExpression(number_t value_) : Expression(ExpressionKind::NumberLiteralExpression), value(value_) {};

    AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. `foo++`, `bar--`
 */
class PostfixExpression final : public Expression {
   public:
    ExpressionUPtr_t left;
    lexer::TokenType op;

    PostfixExpression(ExpressionUPtr_t left_, lexer::TokenType op_) : Expression(ExpressionKind::PostfixExpression), left(std::move(left_)), op(op_) {}

    AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. `++foo`, `--bar`
 */
class PrefixExpression final : public Expression {
   public:
    lexer::TokenType op;
    ExpressionUPtr_t right;

    PrefixExpression(lexer::TokenType op_, ExpressionUPtr_t right_) :Expression(ExpressionKind::PrefixExpression), op(op_), right(std::move(right_)) {}

    AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. `Module::Element`
 */
class ScopeResolutionExpression final : public Expression {
   public:
    ExpressionUPtr_t scope;
    std::string element;

    ScopeResolutionExpression(ExpressionUPtr_t scope_, std::string element_) :Expression(ExpressionKind::ScopeResolutionExpression),
        scope(std::move(scope_)), element(std::move(element_)) {}

    AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. `"Hello, World!"`, `"Line 1\nLine 2"`
 */
class StringLiteralExpression final : public Expression {
   public:
    std::string value;

    constexpr explicit StringLiteralExpression(const std::string& value_) : Expression(ExpressionKind::StringLiteralExpression), value(std::move(value_)) {};
    constexpr explicit StringLiteralExpression(const char* value_) : Expression(ExpressionKind::StringLiteralExpression), value(value_) {};

    AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. `foo as Bar`
 */
class TypeCastExpression final : public Expression {
   public:
    ExpressionUPtr_t originalValue;
    TypeSPtr_t targetType;

    TypeCastExpression(ExpressionUPtr_t originalValue_, TypeSPtr_t targetType_) : Expression(ExpressionKind::TypeCastExpression),
        originalValue(std::move(originalValue_)), targetType(std::move(targetType_)) {}

    AST_STANDARD_INTERFACE;
};

}  // namespace ast

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_AST_EXPRESSIONS_HPP