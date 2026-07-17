#ifndef MANGANESE_INCLUDE_FRONTEND_AST_AST_STATEMENTS_HPP
#define MANGANESE_INCLUDE_FRONTEND_AST_AST_STATEMENTS_HPP

#include <frontend/ast/ast_base.hpp>
#include <frontend/ast/ast_expressions.hpp>
#include <frontend/ast/ast_types.hpp>
#include <utility>
#include <vector>

namespace Manganese {

namespace ast {

enum class StatementKind : uint8_t {
#define STMT(name, str) name,
#define EXPR(name, str)
#define TYPE(name, str)
#include <frontend/ast/ast.def>
#undef STMT
#undef EXPR
#undef TYPE
};

struct AggregateField {
    std::string name;
    Type* type;
    bool isMutable;
    size_t line, column;
};

struct AggregateDeclarationStatement final : public Statement {
    std::string name;
    std::vector<std::string> genericTypes;
    std::vector<AggregateField> fields;
    Visibility visibility = Visibility::Private;

    constexpr AggregateDeclarationStatement(std::string&& _name, std::vector<std::string>&& _genericTypes,
                                            std::vector<AggregateField>&& _fields) noexcept :
        Statement(StatementKind::AggregateDeclarationStatement),
        name(std::move(_name)),
        genericTypes(std::move(_genericTypes)),
        fields(std::move(_fields)) {}
    MN_AST_STANDARD_INTERFACE;
};

struct AliasStatement final : public Statement {
    Type* baseType;  // The type being aliased (x in alias x as foo)
    std::string alias;  // The name of the alias (foo in alias x as foo)
    Visibility visibility = Visibility::Private;

    AliasStatement(Type* _baseType, std::string&& _alias) noexcept :
        Statement(StatementKind::AliasStatement), baseType(_baseType), alias(std::move(_alias)) {}

    MN_AST_STANDARD_INTERFACE
};

struct BreakStatement final : public Statement {
    constexpr explicit BreakStatement() noexcept : Statement(StatementKind::BreakStatement) {}

    MN_AST_STANDARD_INTERFACE;
};

struct ContinueStatement final : public Statement {
    constexpr explicit ContinueStatement() noexcept : Statement(StatementKind::ContinueStatement) {}

    MN_AST_STANDARD_INTERFACE;
};

struct EmptyStatement final : public Statement {
    constexpr explicit EmptyStatement() noexcept : Statement(StatementKind::EmptyStatement) {}
    MN_AST_STANDARD_INTERFACE;
};

struct EnumValue {
    std::string name;
    Expression* value;
    size_t line, column;
};

struct EnumDeclarationStatement final : public Statement {
    std::string name;
    Type* baseType;
    std::vector<EnumValue> values;
    Visibility visibility = Visibility::Private;

    EnumDeclarationStatement(std::string&& _name, Type* _baseType, std::vector<EnumValue> _values) noexcept :
        Statement(StatementKind::EnumDeclarationStatement),
        name(_name),
        baseType(_baseType),
        values(std::move(_values)) {}

    MN_AST_STANDARD_INTERFACE;
};

/**
 * @brief Wrapper struct to final convert an expression into a statement
 */
struct ExpressionStatement final : public Statement {
    Expression* expression;

    explicit ExpressionStatement(Expression* _expression) noexcept :
        Statement(StatementKind::ExpressionStatement), expression(_expression) {};

    MN_AST_STANDARD_INTERFACE;
};

struct ForLoopStatement final : public Statement {
    Statement* initializationStep;
    Expression* stopCondition;
    Expression* postExpression;
    Block body;

    ForLoopStatement(Statement* _initializationStep, Expression* _stopCondition, Expression* _postExpression,
                     Block&& _body) noexcept :
        Statement(StatementKind::ForLoopStatement),
        initializationStep(_initializationStep),
        stopCondition(_stopCondition),
        postExpression(_postExpression),
        body(std::move(_body)) {}

    MN_AST_STANDARD_INTERFACE;
};

struct FunctionParameter {
    std::string name;
    Type* type;
    bool isMutable;
};

struct FunctionDeclarationStatement final : public Statement {
    std::string name;
    std::vector<std::string> genericTypes;
    std::vector<FunctionParameter> parameters;
    Type* returnType;
    Block body;
    Visibility visibility = Visibility::Private;

    FunctionDeclarationStatement(std::string&& _name, std::vector<std::string>&& _genericTypes,
                                 std::vector<FunctionParameter>&& _parameters, Type* _returnType,
                                 Block&& _body) noexcept :
        Statement(StatementKind::FunctionDeclarationStatement),
        name(std::move(_name)),
        genericTypes(std::move(_genericTypes)),
        parameters(std::move(_parameters)),
        returnType(_returnType),
        body(std::move(_body)) {}

    MN_AST_STANDARD_INTERFACE;
};

struct ElifClause {
    Expression* condition;
    Block body;

    ElifClause(Expression* _condition, Block&& _body) noexcept : condition(_condition), body(std::move(_body)) {}
};

struct IfStatement final : public Statement {
    Expression* condition;
    Block body, elseBody;  // elseBody might be empty
    std::vector<ElifClause> elifs;

    IfStatement(Expression* _condition, Block&& _body, std::vector<ElifClause>&& _elifs,
                Block&& _elseBody = {}) noexcept :
        Statement(StatementKind::IfStatement),
        condition(_condition),
        body(std::move(_body)),
        elseBody(std::move(_elseBody)),
        elifs(std::move(_elifs)) {}

    MN_AST_STANDARD_INTERFACE;
};

struct NestedBlockStatement final : public Statement {
    Block block;

    NestedBlockStatement(Block&& _block) : Statement(StatementKind::NestedBlockStatement), block(std::move(_block)) {}
    MN_AST_STANDARD_INTERFACE;
};

struct ReturnStatement final : public Statement {
    Expression* value;

    explicit ReturnStatement(Expression* _value = nullptr) noexcept :
        Statement(StatementKind::ReturnStatement), value(_value) {}

    MN_AST_STANDARD_INTERFACE;
};

struct CaseClause {
    Expression* literalValue;
    Block body;
};

struct SwitchStatement final : public Statement {
    Expression* variable;
    std::vector<CaseClause> cases;
    Block defaultBody;

    SwitchStatement(Expression* _variable, std::vector<CaseClause>&& _cases, Block&& _defaultBody = {}) noexcept :
        Statement(StatementKind::SwitchStatement),
        variable(_variable),
        cases(std::move(_cases)),
        defaultBody(std::move(_defaultBody)) {}

    MN_AST_STANDARD_INTERFACE;
};

struct VariableDeclarationStatement final : public Statement {
    bool isMutable;
    std::string name;
    Visibility visibility;
    Expression* value;
    Type* type;

    VariableDeclarationStatement(bool _isMutable, std::string&& _name, Visibility _visibility, Expression* _value,
                                 Type* _type) noexcept :
        Statement(StatementKind::VariableDeclarationStatement),
        isMutable(_isMutable),
        name(std::move(_name)),
        visibility(_visibility),
        value(_value),
        type(_type) {}

    MN_AST_STANDARD_INTERFACE;
};

struct WhileLoopStatement final : public Statement {
    Block body;
    Expression* condition;
    bool isDoWhile;

    WhileLoopStatement(Block&& _body, Expression* _condition, bool _isDoWhile = false) noexcept :
        Statement(StatementKind::WhileLoopStatement),
        body(std::move(_body)),
        condition(_condition),
        isDoWhile(_isDoWhile) {}

    MN_AST_STANDARD_INTERFACE;
};

}  // namespace ast

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_AST_STATEMENTS_HPP