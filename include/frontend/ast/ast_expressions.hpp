#ifndef MANGANESE_INCLUDE_FRONTEND_AST_AST_EXPRESSIONS_HPP
#define MANGANESE_INCLUDE_FRONTEND_AST_AST_EXPRESSIONS_HPP

#include <core.hpp>
#include <cstddef>
#include <cstdint>
#include <frontend/ast/ast_base.hpp>
#include <frontend/lexer/token.hpp>
#include <mnstl/fold_result.hxx>
#include <mnstl/number.hxx>
#include <string>
#include <utility>
#include <vector>

namespace Manganese::ast {

enum class ExpressionKind : std::uint8_t {
#define STMT(name)
#define EXPR(name) name,
#define TYPE(name)
#include <frontend/ast/ast.def>
#undef STMT
#undef EXPR
#undef TYPE
};

struct AggregateInstantiationField {
    std::string name;
    Expression* value;
    std::size_t line, column;
};

struct AggregateInstantiationExpression final : public Expression {
    Expression* base;
    std::vector<AggregateInstantiationField> fields;

    AggregateInstantiationExpression(Expression* _base, std::vector<AggregateInstantiationField>&& _fields) noexcept :
        Expression(ExpressionKind::AggregateInstantiationExpression), base(_base), fields(std::move(_fields)) {}

    MN_AST_STANDARD_INTERFACE;
};

struct AggregateLiteralExpression final : public Expression {
    std::vector<Expression*> elements;

    explicit AggregateLiteralExpression(std::vector<Expression*>&& _elements) noexcept :
        Expression(ExpressionKind::AggregateLiteralExpression), elements(std::move(_elements)) {}

    MN_AST_STANDARD_INTERFACE;
};

struct AlignofExpression final : public Expression {
    Type* type;

    AlignofExpression(Type* t) noexcept : Expression(ExpressionKind::AlignofExpression), type(t) {}

    mnstl::fold_result_t fold() const NOEXCEPT_IF_RELEASE override;

    MN_AST_STANDARD_INTERFACE
};

struct ArrayLiteralExpression final : public Expression {
    std::vector<Expression*> elements;

    explicit ArrayLiteralExpression(std::vector<Expression*>&& _elements) noexcept :
        Expression(ExpressionKind::ArrayLiteralExpression), elements(std::move(_elements)) {}

    MN_AST_STANDARD_INTERFACE;
};

struct AssignmentExpression final : public Expression {
    Expression* assignee;  // The thing being assigned to (foo in foo = bar)
    Expression* value;  // The value being assigned (bar in foo = bar)
    lexer::TokenType op;

    AssignmentExpression(Expression* _assignee, lexer::TokenType _op, Expression* _value) noexcept :
        Expression(ExpressionKind::AssignmentExpression), assignee(_assignee), value(_value), op(_op) {}

    MN_AST_STANDARD_INTERFACE;
};

struct BinaryExpression final : public Expression {
    Expression* left;
    Expression* right;
    lexer::TokenType op;

    BinaryExpression(Expression* _left, lexer::TokenType _op, Expression* _right) noexcept :
        Expression(ExpressionKind::BinaryExpression), left(_left), right(_right), op(_op) {}

    MN_AST_STANDARD_INTERFACE;

    mnstl::fold_result_t fold() const NOEXCEPT_IF_RELEASE override;
};

struct BoolLiteralExpression final : public Expression {
    const bool value;

    constexpr explicit BoolLiteralExpression(bool _value) noexcept :
        Expression(ExpressionKind::BoolLiteralExpression), value(_value) {}

    MN_AST_STANDARD_INTERFACE;

    mnstl::fold_result_t fold() const NOEXCEPT_IF_RELEASE override { return mnstl::fold_result_t{value}; }
};

struct CharLiteralExpression final : public Expression {
    const char32_t value;

    constexpr explicit CharLiteralExpression(char32_t _value) noexcept :
        Expression(ExpressionKind::CharLiteralExpression), value(_value) {}
    constexpr explicit CharLiteralExpression(char _value) noexcept :
        Expression(ExpressionKind::CharLiteralExpression), value(static_cast<char32_t>(_value)) {}

    MN_AST_STANDARD_INTERFACE;

    mnstl::fold_result_t fold() const NOEXCEPT_IF_RELEASE override { return mnstl::fold_result_t{value}; }
};

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
struct GenericInstantiationExpression final : public Expression {
    Expression* identifier;
    std::vector<Type*> types;

    GenericInstantiationExpression(Expression* _identifier, std::vector<Type*>&& _types) noexcept :
        Expression(ExpressionKind::GenericInstantiationExpression), identifier(_identifier), types(std::move(_types)) {}

    MN_AST_STANDARD_INTERFACE;
};

struct IdentifierExpression final : public Expression {
    const std::string name;

    constexpr explicit IdentifierExpression(std::string&& _name) noexcept :
        Expression(ExpressionKind::IdentifierExpression), name(std::move(_name)) {}

    MN_AST_STANDARD_INTERFACE;
};

struct IndexExpression final : public Expression {
    Expression* variable;
    Expression* index;

    IndexExpression(Expression* _variable, Expression* _index) noexcept :
        Expression(ExpressionKind::IndexExpression), variable(_variable), index(_index) {}

    MN_AST_STANDARD_INTERFACE;
};

struct MemberAccessExpression final : public Expression {
    Expression* object;
    const std::string property;

    MemberAccessExpression(Expression* _object, std::string&& _property) noexcept :
        Expression(ExpressionKind::MemberAccessExpression), object(_object), property(std::move(_property)) {}

    MN_AST_STANDARD_INTERFACE;
};

struct NumberLiteralExpression final : public Expression {
    const mnstl::number_t value;

    constexpr explicit NumberLiteralExpression(mnstl::number_t _value) noexcept :
        Expression(ExpressionKind::NumberLiteralExpression), value(_value) {}

    MN_AST_STANDARD_INTERFACE;

    mnstl::fold_result_t fold() const NOEXCEPT_IF_RELEASE override { return mnstl::fold_result_t{value}; }
};

struct PostfixExpression final : public Expression {
    Expression* left;
    lexer::TokenType op;

    PostfixExpression(Expression* _left, lexer::TokenType _op) noexcept :
        Expression(ExpressionKind::PostfixExpression), left(_left), op(_op) {}

    MN_AST_STANDARD_INTERFACE;
    mnstl::fold_result_t fold() const NOEXCEPT_IF_RELEASE override;
};

struct PrefixExpression final : public Expression {
    lexer::TokenType op;
    Expression* right;

    PrefixExpression(lexer::TokenType _op, Expression* _right) noexcept :
        Expression(ExpressionKind::PrefixExpression), op(_op), right(_right) {}

    MN_AST_STANDARD_INTERFACE;

    mnstl::fold_result_t fold() const NOEXCEPT_IF_RELEASE override;
};

struct ScopeResolutionExpression final : public Expression {
    Expression* scope;
    Expression* element;

    ScopeResolutionExpression(Expression* _scope, Expression* _element) noexcept :
        Expression(ExpressionKind::ScopeResolutionExpression), scope(_scope), element(_element) {}

    MN_AST_STANDARD_INTERFACE;
};

struct SizeofExpression final : public Expression {
    Type* type;

    SizeofExpression(Type* t) noexcept : Expression(ExpressionKind::SizeofExpression), type(t) {}
    mnstl::fold_result_t fold() const NOEXCEPT_IF_RELEASE override;

    MN_AST_STANDARD_INTERFACE
};

struct StringLiteralExpression final : public Expression {
    const std::string value;

    constexpr explicit StringLiteralExpression(std::string&& _value) noexcept :
        Expression(ExpressionKind::StringLiteralExpression), value(std::move(_value)) {}

    MN_AST_STANDARD_INTERFACE;
    mnstl::fold_result_t fold() const NOEXCEPT_IF_RELEASE override { return mnstl::fold_result_t{value}; }
};

struct TypeCastExpression final : public Expression {
    Expression* originalValue;
    Type* targetType;

    TypeCastExpression(Expression* _originalValue, Type* _targetType) noexcept :
        Expression(ExpressionKind::TypeCastExpression), originalValue(_originalValue), targetType(_targetType) {}

    MN_AST_STANDARD_INTERFACE;
};

}  // namespace Manganese::ast

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_AST_EXPRESSIONS_HPP