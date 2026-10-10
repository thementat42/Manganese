#ifndef MANGANESE_INCLUDE_FRONTEND_AST_AST_EXPRESSIONS_HPP
#define MANGANESE_INCLUDE_FRONTEND_AST_AST_EXPRESSIONS_HPP

#include <core.hpp>
#include <cstddef>
#include <frontend/ast/ast_base.hpp>
#include <frontend/lexer/token.hpp>
#include <utils/string_interner.hpp>
#include <mnstl/slice.hxx>
#include <utils/target_info.hpp>

namespace Manganese::ast {

struct AggregateInstantiationField {
    utils::StringID name;
    Expression* value;
    std::size_t line, column;
};

struct AggregateInstantiationExpression final : public Expression {
    Expression* base;
    mnstl::Slice<AggregateInstantiationField> fields;

    AggregateInstantiationExpression(Expression* _base, mnstl::Slice<AggregateInstantiationField>_fields) noexcept :
        Expression(ExpressionKind::AggregateInstantiationExpression), base(_base), fields(_fields) {}

    MN_AST_STANDARD_INTERFACE(AggregateInstantiationExpression);
};

struct AggregateLiteralExpression final : public Expression {
    mnstl::Slice<Expression*> elements;

    explicit AggregateLiteralExpression(mnstl::Slice<Expression*>&& _elements) noexcept :
        Expression(ExpressionKind::AggregateLiteralExpression), elements(_elements) {}

    MN_AST_STANDARD_INTERFACE(AggregateLiteralExpression);
    ;
};

struct AlignofExpression final : public Expression {
    Type* type;

    AlignofExpression(Type* t) noexcept : Expression(ExpressionKind::AlignofExpression), type(t) {}
    bool canFold() const noexcept override { return true; }

    MN_AST_STANDARD_INTERFACE(AlignofExpression);
};

struct ArrayLiteralExpression final : public Expression {
    mnstl::Slice<Expression*> elements;

    explicit ArrayLiteralExpression(mnstl::Slice<Expression*> _elements) noexcept :
        Expression(ExpressionKind::ArrayLiteralExpression), elements(_elements) {}

    MN_AST_STANDARD_INTERFACE(ArrayLiteralExpression);
};

struct AssignmentExpression final : public Expression {
    Expression* assignee;  // The thing being assigned to (foo in foo = bar)
    Expression* value;  // The value being assigned (bar in foo = bar)
    lexer::TokenType op;

    AssignmentExpression(Expression* _assignee, lexer::TokenType _op, Expression* _value) noexcept :
        Expression(ExpressionKind::AssignmentExpression), assignee(_assignee), value(_value), op(_op) {}

    MN_AST_STANDARD_INTERFACE(AssignmentExpression);
};

struct BinaryExpression final : public Expression {
    Expression* left;
    Expression* right;
    lexer::TokenType op;

    BinaryExpression(Expression* _left, lexer::TokenType _op, Expression* _right) noexcept :
        Expression(ExpressionKind::BinaryExpression), left(_left), right(_right), op(_op) {}
    bool canFold() const noexcept override { return left->canFold() && right->canFold(); }

    MN_AST_STANDARD_INTERFACE(BinaryExpression);
};

struct BoolLiteralExpression final : public Expression {
    const bool value;

    explicit BoolLiteralExpression(bool _value) noexcept :
        Expression(ExpressionKind::BoolLiteralExpression), value(_value) {}
    bool canFold() const noexcept override { return true; }

    MN_AST_STANDARD_INTERFACE(BoolLiteralExpression);
};

struct CharLiteralExpression final : public Expression {
    const char32_t value;

    explicit CharLiteralExpression(char32_t _value) noexcept :
        Expression(ExpressionKind::CharLiteralExpression), value(_value) {}
    explicit CharLiteralExpression(char _value) noexcept :
        Expression(ExpressionKind::CharLiteralExpression), value(static_cast<char32_t>(_value)) {}

    bool canFold() const noexcept override { return true; }

    MN_AST_STANDARD_INTERFACE(CharLiteralExpression);
};

struct FunctionCallExpression final : public Expression {
    Expression* callee;
    mnstl::Slice<Expression*> arguments;

    FunctionCallExpression(Expression* _callee, mnstl::Slice<Expression*> _arguments) noexcept :
        Expression(ExpressionKind::FunctionCallExpression), callee(_callee), arguments(_arguments) {}

    MN_AST_STANDARD_INTERFACE(FunctionCallExpression);
};

/**
 * e.g. `foo@[int, string]`
 */
struct GenericInstantiationExpression final : public Expression {
    Expression* identifier;
    mnstl::Slice<Type*> types;
    mnstl::Slice<const semantic::SemanticType*> semanticTypes;

    GenericInstantiationExpression(Expression* _identifier, mnstl::Slice<Type*> _types) noexcept :
        Expression(ExpressionKind::GenericInstantiationExpression), identifier(_identifier), types(_types) {}

    MN_AST_STANDARD_INTERFACE(GenericInstantiationExpression);
};

struct IdentifierExpression final : public Expression {
    const utils::StringID name;
    const Declaration* resolvedDeclaration = nullptr;

    explicit IdentifierExpression(utils::StringID _name) noexcept :
        Expression(ExpressionKind::IdentifierExpression), name(_name) {}

    MN_AST_STANDARD_INTERFACE(IdentifierExpression);
};

struct IndexExpression final : public Expression {
    Expression* variable;
    Expression* index;

    IndexExpression(Expression* _variable, Expression* _index) noexcept :
        Expression(ExpressionKind::IndexExpression), variable(_variable), index(_index) {}

    MN_AST_STANDARD_INTERFACE(IndexExpression);
};

struct MemberAccessExpression final : public Expression {
    Expression* object;
    const utils::StringID field;
    std::size_t fieldIndex = static_cast<std::size_t>(-1);

    MemberAccessExpression(Expression* _object, utils::StringID _field) noexcept :
        Expression(ExpressionKind::MemberAccessExpression), object(_object), field(_field) {}

    MN_AST_STANDARD_INTERFACE(MemberAccessExpression);
};

struct NumberLiteralExpression final : public Expression {
    const utils::StringID value;
    const bool isFloat;

    explicit NumberLiteralExpression(utils::StringID _value, bool _isFloat) noexcept :
        Expression(ExpressionKind::NumberLiteralExpression), value(_value), isFloat(_isFloat) {}

    bool canFold() const noexcept override { return true; }

    MN_AST_STANDARD_INTERFACE(NumberLiteralExpression);
};

struct PostfixExpression final : public Expression {
    Expression* left;
    lexer::TokenType op;

    PostfixExpression(Expression* _left, lexer::TokenType _op) noexcept :
        Expression(ExpressionKind::PostfixExpression), left(_left), op(_op) {}

    bool canFold() const noexcept override { return left->canFold(); }

    MN_AST_STANDARD_INTERFACE(PostfixExpression);
};

struct PrefixExpression final : public Expression {
    lexer::TokenType op;
    Expression* right;

    PrefixExpression(lexer::TokenType _op, Expression* _right) noexcept :
        Expression(ExpressionKind::PrefixExpression), op(_op), right(_right) {}

    bool canFold() const noexcept override { return right->canFold(); }

    MN_AST_STANDARD_INTERFACE(PrefixExpression);
};

struct ScopeResolutionExpression final : public Expression {
    Expression* scope;
    Expression* element;
    utils::StringID mangledName;

    ScopeResolutionExpression(Expression* _scope, Expression* _element) noexcept :
        Expression(ExpressionKind::ScopeResolutionExpression), scope(_scope), element(_element) {}

    MN_AST_STANDARD_INTERFACE(ScopeResolutionExpression);
};

struct SizeofExpression final : public Expression {
    Type* type;

    SizeofExpression(Type* t) noexcept : Expression(ExpressionKind::SizeofExpression), type(t) {}
    bool canFold() const noexcept override { return true; }

    MN_AST_STANDARD_INTERFACE(SizeofExpression)
};

struct StringLiteralExpression final : public Expression {
    const utils::StringID value;

    explicit StringLiteralExpression(utils::StringID _value) noexcept :
        Expression(ExpressionKind::StringLiteralExpression), value(_value) {}
    bool canFold() const noexcept override { return true; }

    MN_AST_STANDARD_INTERFACE(StringLiteralExpression);
};

struct TernaryExpression final : public Expression {
    Expression* condition;
    Expression* ifTrue;
    Expression* ifFalse;

    TernaryExpression(Expression* _condition, Expression* _ifTrue, Expression* _ifFalse) noexcept :
        Expression(ExpressionKind::TernaryExpression), condition(_condition), ifTrue(_ifTrue), ifFalse(_ifFalse) {}

    MN_AST_STANDARD_INTERFACE(TernaryExpression);
};

struct TypeCastExpression final : public Expression {
    Expression* originalValue;
    Type* targetType;

    TypeCastExpression(Expression* _originalValue, Type* _targetType) noexcept :
        Expression(ExpressionKind::TypeCastExpression), originalValue(_originalValue), targetType(_targetType) {}
    bool canFold() const noexcept override { return originalValue->canFold(); }

    MN_AST_STANDARD_INTERFACE(TypeCastExpression);
};

struct UninitializedExpression final : public Expression {
    UninitializedExpression() noexcept : Expression(ExpressionKind::UninitializedExpression) {}

    MN_AST_STANDARD_INTERFACE(UninitializedExpression);
};

struct PoisonedExpression final : public Expression {
    PoisonedExpression() noexcept : Expression(ExpressionKind::PoisonedExpression) {}

    MN_AST_STANDARD_INTERFACE(PoisonedExpression);
};

}  // namespace Manganese::ast

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_AST_EXPRESSIONS_HPP