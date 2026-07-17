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

    AggregateInstantiationExpression(std::string&& _name, std::vector<Type*>&& _genericTypes,
                                     std::vector<AggregateInstantiationField>&& _fields) noexcept :
        Expression(ExpressionKind::AggregateInstantiationExpression),
        name(std::move(_name)),
        genericTypes(std::move(_genericTypes)),
        fields(std::move(_fields)) {}

    MN_AST_STANDARD_INTERFACE;
};

/**
 * @brief represents a sequence of elements of different types
 */
struct AggregateLiteralExpression final : public Expression {
    std::vector<Expression*> elements;

    explicit AggregateLiteralExpression(std::vector<Expression*>&& _elements) noexcept :
        Expression(ExpressionKind::AggregateLiteralExpression), elements(std::move(_elements)) {}

    MN_AST_STANDARD_INTERFACE;
};

struct AlignofExpression final : public Expression {
    Type* type;

    AlignofExpression(Type* t) noexcept : Expression(ExpressionKind::AlignofExpression), type(t) {}

    MN_AST_STANDARD_INTERFACE
};

/**
 * @brief e.g. [1, 2, 3]
 */
struct ArrayLiteralExpression final : public Expression {
    std::vector<Expression*> elements;
    Type* elementType;  // Optional, can be inferred from the elements
    Expression* lengthExpression = nullptr;

    explicit ArrayLiteralExpression(std::vector<Expression*>&& _elements, Type* _elementType = nullptr) noexcept :
        Expression(ExpressionKind::ArrayLiteralExpression), elements(std::move(_elements)), elementType(_elementType) {}

    MN_AST_STANDARD_INTERFACE;
};

/**
 * e.g. `foo = bar`, `baz *= 1`
 */
struct AssignmentExpression final : public Expression {
    Expression* assignee;  // The thing being assigned to (foo in foo = bar)
    Expression* value;  // The value being assigned (bar in foo = bar)
    lexer::TokenType op;

    AssignmentExpression(Expression* _assignee, lexer::TokenType _op, Expression* _value) noexcept :
        Expression(ExpressionKind::AssignmentExpression), assignee(_assignee), value(_value), op(_op) {}

    MN_AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. `a + b`, `x * y`
 */
struct BinaryExpression final : public Expression {
    Expression* left;
    Expression* right;
    lexer::TokenType op;

    BinaryExpression(Expression* _left, lexer::TokenType _op, Expression* _right) noexcept :
        Expression(ExpressionKind::BinaryExpression), left(_left), right(_right), op(_op) {};

    MN_AST_STANDARD_INTERFACE;

    virtual mnstl::fold_result_t fold() const noexcept override;
};

/**
 * @brief e.g. `true`, `false`
 */
struct BoolLiteralExpression final : public Expression {
    const bool value;

    constexpr explicit BoolLiteralExpression(bool _value) noexcept :
        Expression(ExpressionKind::BoolLiteralExpression), value(_value) {};

    MN_AST_STANDARD_INTERFACE;

    virtual mnstl::fold_result_t fold() const noexcept override { return mnstl::fold_result_t{value}; }
};

/**
 * @brief e.g. 'a', '\u1234', '\n'
 */
struct CharLiteralExpression final : public Expression {
    const char32_t value;

    constexpr explicit CharLiteralExpression(char32_t _value) noexcept :
        Expression(ExpressionKind::CharLiteralExpression), value(_value) {};
    constexpr explicit CharLiteralExpression(char _value) noexcept :
        Expression(ExpressionKind::CharLiteralExpression), value(static_cast<char32_t>(_value)) {};

    MN_AST_STANDARD_INTERFACE;

    virtual mnstl::fold_result_t fold() const noexcept override { return mnstl::fold_result_t{value}; }
};

/**
 * @brief e.g. `foo()`, `bar(1, 2, 3)`
 */
struct FunctionCallExpression final : public Expression {
    Expression* callee;
    std::vector<Expression*> arguments;

    FunctionCallExpression(Expression* _callee, std::vector<Expression*>&& _arguments) noexcept :
        Expression(ExpressionKind::FunctionCallExpression), callee(_callee), arguments(std::move(_arguments)) {}

    MN_AST_STANDARD_INTERFACE;
};

/**
 * e.g. `foo@[int, string]`
 */
struct GenericExpression final : public Expression {
    Expression* identifier;
    std::vector<Type*> types;

    GenericExpression(Expression* _identifier, std::vector<Type*>&& _types) noexcept :
        Expression(ExpressionKind::GenericExpression), identifier(_identifier), types(std::move(_types)) {}

    MN_AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. `foo`, `bar`
 */
struct IdentifierExpression final : public Expression {
    const std::string value;

    constexpr explicit IdentifierExpression(std::string&& _value) noexcept :
        Expression(ExpressionKind::IdentifierExpression), value(std::move(_value)) {}

    MN_AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. `foo[0]`, `bar[1 + 2]`
 */
struct IndexExpression final : public Expression {
    Expression* variable;
    Expression* index;

    IndexExpression(Expression* _variable, Expression* _index) noexcept :
        Expression(ExpressionKind::IndexExpression), variable(_variable), index(_index) {}

    MN_AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. `foo.bar`
 */
struct MemberAccessExpression final : public Expression {
    Expression* object;
    const std::string property;

    MemberAccessExpression(Expression* _object, std::string&& _property) noexcept :
        Expression(ExpressionKind::MemberAccessExpression), object(_object), property(std::move(_property)) {}

    MN_AST_STANDARD_INTERFACE;
};

/**
 * @brief e.g. `42`, `3.14`, `-1`
 */
struct NumberLiteralExpression final : public Expression {
    const mnstl::number_t value;

    constexpr explicit NumberLiteralExpression(mnstl::number_t _value) noexcept :
        Expression(ExpressionKind::NumberLiteralExpression), value(_value) {};

    MN_AST_STANDARD_INTERFACE;

    virtual mnstl::fold_result_t fold() const noexcept override { return mnstl::fold_result_t{value}; }
};

/**
 * @brief e.g. `foo++`, `bar--`
 */
struct PostfixExpression final : public Expression {
    Expression* left;
    lexer::TokenType op;

    PostfixExpression(Expression* _left, lexer::TokenType _op) noexcept :
        Expression(ExpressionKind::PostfixExpression), left(_left), op(_op) {}

    MN_AST_STANDARD_INTERFACE;
    virtual mnstl::fold_result_t fold() const noexcept override;
};

/**
 * @brief e.g. `++foo`, `--bar`
 */
struct PrefixExpression final : public Expression {
    lexer::TokenType op;
    Expression* right;

    PrefixExpression(lexer::TokenType _op, Expression* _right) noexcept :
        Expression(ExpressionKind::PrefixExpression), op(_op), right(_right) {}

    MN_AST_STANDARD_INTERFACE;

    virtual mnstl::fold_result_t fold() const noexcept override;
};

/**
 * @brief e.g. `Module::Element`
 */
struct ScopeResolutionExpression final : public Expression {
    Expression* scope;
    const std::string element;

    ScopeResolutionExpression(Expression* _scope, std::string&& _element) noexcept :
        Expression(ExpressionKind::ScopeResolutionExpression), scope(_scope), element(std::move(_element)) {}

    MN_AST_STANDARD_INTERFACE;
};

struct SizeofExpression final : public Expression {
    Type* type;

    SizeofExpression(Type* t) noexcept : Expression(ExpressionKind::SizeofExpression), type(t) {}

    MN_AST_STANDARD_INTERFACE
};

/**
 * @brief e.g. `"Hello, World!"`, `"Line 1\nLine 2"`
 */
struct StringLiteralExpression final : public Expression {
    const std::string value;

    constexpr explicit StringLiteralExpression(std::string&& _value) noexcept :
        Expression(ExpressionKind::StringLiteralExpression), value(std::move(_value)) {};

    // constexpr explicit StringLiteralExpression(const char* _value) noexcept :
    //     Expression(ExpressionKind::StringLiteralExpression), value(_value) {};

    MN_AST_STANDARD_INTERFACE;
    virtual mnstl::fold_result_t fold() const noexcept override { return mnstl::fold_result_t{value}; }
};

/**
 * @brief e.g. `foo as Bar`
 */
struct TypeCastExpression final : public Expression {
    Expression* originalValue;
    Type* targetType;

    TypeCastExpression(Expression* _originalValue, Type* _targetType) noexcept :
        Expression(ExpressionKind::TypeCastExpression), originalValue(_originalValue), targetType(_targetType) {}

    MN_AST_STANDARD_INTERFACE;
};

}  // namespace ast

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_AST_EXPRESSIONS_HPP