#ifndef MANGANESE_INCLUDE_FRONTEND_AST_AST_EXPRESSIONS_HPP
#define MANGANESE_INCLUDE_FRONTEND_AST_AST_EXPRESSIONS_HPP

#include <frontend/ast/ast_base.hpp>
#include <frontend/ast/ast_types.hpp>
#include <vector>

namespace Manganese {

namespace ast {

enum class ExpressionKind : uint8_t {
#define STMT(name, str)
#define EXPR(name, str) name,
#define TYPE(name, str)
#include "ast.def"
#undef STMT
#undef EXPR
#undef TYPE
};

struct AggregateInstantiationField {
    std::string name;
    Expression* value;
};

/**
 * @brief e.g. `Point3D{ x = 1, y = 2, z = 3 }`
 */
struct AggregateInstantiationExpression final : public Expression {
    std::string name;
    std::vector<Type*> genericTypes;
    std::vector<AggregateInstantiationField> fields;

    AggregateInstantiationExpression(std::string&& name_, std::vector<Type*>&& genericTypes_,
                                     std::vector<AggregateInstantiationField>&& fields_) :
        Expression(ExpressionKind::AggregateInstantiationExpression),
        name(std::move(name_)),
        genericTypes(std::move(genericTypes_)),
        fields(std::move(fields_)) {}

    AST_STANDARD_INTERFACE;
};

/**
 * @brief represents a sequence of elements of different types
 */
struct AggregateLiteralExpression final : public Expression {
    std::vector<Expression*> elements;

    explicit AggregateLiteralExpression(std::vector<Expression*>&& elements_) :
        Expression(ExpressionKind::AggregateLiteralExpression), elements(std::move(elements_)) {}

    AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. [1, 2, 3]
 */
struct ArrayLiteralExpression final : public Expression {
    std::vector<Expression*> elements;
    Type* elementType;  // Optional, can be inferred from the elements
    Expression* lengthExpression = nullptr;

    explicit ArrayLiteralExpression(std::vector<Expression*>&& elements_, Type* elementType_ = nullptr) :
        Expression(ExpressionKind::ArrayLiteralExpression), elements(std::move(elements_)), elementType(elementType_) {}

    AST_STANDARD_INTERFACE;
};

/**
 * e.g. `foo = bar`, `baz *= 1`
 */
struct AssignmentExpression final : public Expression {
    Expression* assignee;  // The thing being assigned to (foo in foo = bar)
    Expression* value;  // The value being assigned (bar in foo = bar)
    lexer::TokenType op;

    AssignmentExpression(Expression* assignee_, lexer::TokenType op_, Expression* value_) :
        Expression(ExpressionKind::AssignmentExpression), assignee(assignee_), value(value_), op(op_) {}

    AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. `a + b`, `x * y`
 */
struct BinaryExpression final : public Expression {
    Expression* left;
    Expression* right;
    lexer::TokenType op;

    BinaryExpression(Expression* left_, lexer::TokenType op_, Expression* right_) :
        Expression(ExpressionKind::BinaryExpression), left(left_), right(right_), op(op_) {};

    AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. `true`, `false`
 */
struct BoolLiteralExpression final : public Expression {
    const bool value;

    constexpr explicit BoolLiteralExpression(bool value_) :
        Expression(ExpressionKind::BoolLiteralExpression), value(value_) {};

    AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. 'a', '\u1234', '\n'
 */
struct CharLiteralExpression final : public Expression {
    const char32_t value;

    constexpr explicit CharLiteralExpression(char32_t value_) :
        Expression(ExpressionKind::CharLiteralExpression), value(value_) {};
    constexpr explicit CharLiteralExpression(char value_) :
        Expression(ExpressionKind::CharLiteralExpression), value(static_cast<char32_t>(value_)) {};

    AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. `foo()`, `bar(1, 2, 3)`
 */
struct FunctionCallExpression final : public Expression {
    Expression* callee;
    std::vector<Expression*> arguments;

    FunctionCallExpression(Expression* callee_, std::vector<Expression*>&& arguments_) :
        Expression(ExpressionKind::FunctionCallExpression), callee(callee_), arguments(std::move(arguments_)) {}

    AST_STANDARD_INTERFACE;
};

/**
 * e.g. `foo@[int, string]`
 */
struct GenericExpression final : public Expression {
    Expression* identifier;
    std::vector<Type*> types;

    GenericExpression(Expression* identifier_, std::vector<Type*>&& types_) :
        Expression(ExpressionKind::GenericExpression), identifier(identifier_), types(std::move(types_)) {}

    AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. `foo`, `bar`
 */
struct IdentifierExpression final : public Expression {
    const std::string value;

    constexpr explicit IdentifierExpression(std::string&& value_) :
        Expression(ExpressionKind::IdentifierExpression), value(std::move(value_)) {}

    AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. `foo[0]`, `bar[1 + 2]`
 */
struct IndexExpression final : public Expression {
    Expression* variable;
    Expression* index;

    IndexExpression(Expression* variable_, Expression* index_) :
        Expression(ExpressionKind::IndexExpression), variable(variable_), index(index_) {}

    AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. `foo.bar`
 */
struct MemberAccessExpression final : public Expression {
    Expression* object;
    const std::string property;

    MemberAccessExpression(Expression* object_, std::string&& property_) :
        Expression(ExpressionKind::MemberAccessExpression), object(object_), property(std::move(property_)) {}

    AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. `42`, `3.14`, `-1`
 */
struct NumberLiteralExpression final : public Expression {
    const mnstl::number_t value;

    constexpr explicit NumberLiteralExpression(mnstl::number_t value_) :
        Expression(ExpressionKind::NumberLiteralExpression), value(value_) {};

    AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. `foo++`, `bar--`
 */
struct PostfixExpression final : public Expression {
    Expression* left;
    lexer::TokenType op;

    PostfixExpression(Expression* left_, lexer::TokenType op_) :
        Expression(ExpressionKind::PostfixExpression), left(left_), op(op_) {}

    AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. `++foo`, `--bar`
 */
struct PrefixExpression final : public Expression {
    lexer::TokenType op;
    Expression* right;

    PrefixExpression(lexer::TokenType op_, Expression* right_) :
        Expression(ExpressionKind::PrefixExpression), op(op_), right(right_) {}

    AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. `Module::Element`
 */
struct ScopeResolutionExpression final : public Expression {
    Expression* scope;
    const std::string element;

    ScopeResolutionExpression(Expression* scope_, std::string&& element_) :
        Expression(ExpressionKind::ScopeResolutionExpression), scope(scope_), element(std::move(element_)) {}

    AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. `"Hello, World!"`, `"Line 1\nLine 2"`
 */
struct StringLiteralExpression final : public Expression {
    const std::string value;

    constexpr explicit StringLiteralExpression(std::string&& value_) :
        Expression(ExpressionKind::StringLiteralExpression), value(std::move(value_)) {};

    constexpr explicit StringLiteralExpression(const char* value_) :
        Expression(ExpressionKind::StringLiteralExpression), value(value_) {};

    AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. `foo as Bar`
 */
struct TypeCastExpression final : public Expression {
    Expression* originalValue;
    Type* targetType;

    TypeCastExpression(Expression* originalValue_, Type* targetType_) :
        Expression(ExpressionKind::TypeCastExpression), originalValue(originalValue_), targetType(targetType_) {}

    AST_STANDARD_INTERFACE;
};

}  // namespace ast

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_AST_EXPRESSIONS_HPP