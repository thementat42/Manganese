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
#include "ast.def"
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

    constexpr AggregateDeclarationStatement(std::string&& name_, std::vector<std::string>&& genericTypes_,
                                            std::vector<AggregateField>&& fields_) noexcept :
        Statement(StatementKind::AggregateDeclarationStatement),
        name(std::move(name_)),
        genericTypes(std::move(genericTypes_)),
        fields(std::move(fields_)) {}
    MN_AST_STANDARD_INTERFACE;
};

struct AliasStatement final : public Statement {
    Type* baseType;  // The type being aliased (x in alias x as foo)
    std::string alias;  // The name of the alias (foo in alias x as foo)
    Visibility visibility = Visibility::Private;

    AliasStatement(Type* baseType_, std::string&& alias_) noexcept :
        Statement(StatementKind::AliasStatement), baseType(baseType_), alias(std::move(alias_)) {}

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

    EnumDeclarationStatement(std::string&& name_, Type* baseType_, std::vector<EnumValue> values_) noexcept :
        Statement(StatementKind::EnumDeclarationStatement),
        name(name_),
        baseType(baseType_),
        values(std::move(values_)) {}

    MN_AST_STANDARD_INTERFACE;
};

/**
 * @brief Wrapper struct to final convert an expression into a statement
 */
struct ExpressionStatement final : public Statement {
    Expression* expression;

    explicit ExpressionStatement(Expression* expression_) noexcept :
        Statement(StatementKind::ExpressionStatement), expression(expression_) {};

    MN_AST_STANDARD_INTERFACE;
};

struct ForLoopStatement final : public Statement {
    Statement* initializationStep;
    Expression* stopCondition;
    Expression* postExpression;
    Block body;

    ForLoopStatement(Statement* initializationStep_, Expression* stopCondition_, Expression* postExpression_,
                     Block&& body_) noexcept :
        Statement(StatementKind::ForLoopStatement),
        initializationStep(initializationStep_),
        stopCondition(stopCondition_),
        postExpression(postExpression_),
        body(std::move(body_)) {}

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

    FunctionDeclarationStatement(std::string&& name_, std::vector<std::string>&& genericTypes_,
                                 std::vector<FunctionParameter>&& parameters_, Type* returnType_,
                                 Block&& body_) noexcept :
        Statement(StatementKind::FunctionDeclarationStatement),
        name(std::move(name_)),
        genericTypes(std::move(genericTypes_)),
        parameters(std::move(parameters_)),
        returnType(returnType_),
        body(std::move(body_)) {}

    MN_AST_STANDARD_INTERFACE;
};

struct ElifClause {
    Expression* condition;
    Block body;

    ElifClause(Expression* condition_, Block&& body_) noexcept : condition(condition_), body(std::move(body_)) {}
};

struct IfStatement final : public Statement {
    Expression* condition;
    Block body, elseBody;  // elseBody might be empty
    std::vector<ElifClause> elifs;

    IfStatement(Expression* condition_, Block&& body_, std::vector<ElifClause>&& elifs_,
                Block&& elseBody_ = {}) noexcept :
        Statement(StatementKind::IfStatement),
        condition(condition_),
        body(std::move(body_)),
        elseBody(std::move(elseBody_)),
        elifs(std::move(elifs_)) {}

    MN_AST_STANDARD_INTERFACE;
};

struct ReturnStatement final : public Statement {
    Expression* value;

    explicit ReturnStatement(Expression* value_ = nullptr) noexcept :
        Statement(StatementKind::ReturnStatement), value(value_) {}

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

    SwitchStatement(Expression* variable_, std::vector<CaseClause>&& cases_, Block&& defaultBody_ = {}) noexcept :
        Statement(StatementKind::SwitchStatement),
        variable(variable_),
        cases(std::move(cases_)),
        defaultBody(std::move(defaultBody_)) {}

    MN_AST_STANDARD_INTERFACE;
};

struct VariableDeclarationStatement final : public Statement {
    bool isMutable;
    std::string name;
    Visibility visibility;
    Expression* value;
    Type* type;

    VariableDeclarationStatement(bool isMutable_, std::string&& name_, Visibility visibility_, Expression* _value,
                                 Type* _type) noexcept :
        Statement(StatementKind::VariableDeclarationStatement),
        isMutable(isMutable_),
        name(std::move(name_)),
        visibility(visibility_),
        value(_value),
        type(_type) {}

    MN_AST_STANDARD_INTERFACE;
};

struct WhileLoopStatement final : public Statement {
    Block body;
    Expression* condition;
    bool isDoWhile;

    WhileLoopStatement(Block&& body_, Expression* condition_, bool isDoWhile_ = false) noexcept :
        Statement(StatementKind::WhileLoopStatement),
        body(std::move(body_)),
        condition(condition_),
        isDoWhile(isDoWhile_) {}

    MN_AST_STANDARD_INTERFACE;
};

}  // namespace ast

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_AST_STATEMENTS_HPP