

/**
 * @file ast_statements.h
 * @brief Contains AST node definitions for various statement types in the Manganese frontend.
 *
 * This header declares the core statement node types used in the AST.
 * Each statement type (conditionals, function declarations, etc.) is represented as a class inheriting from Statement.
 *
 * ! The nodes are listed in alphabetical order.
 */
#ifndef MANGANESE_INCLUDE_FRONTEND_AST_AST_STATEMENTS_HPP
#define MANGANESE_INCLUDE_FRONTEND_AST_AST_STATEMENTS_HPP

#include <frontend/ast/ast_base.hpp>
#include <frontend/ast/ast_expressions.hpp>
#include <frontend/ast/ast_types.hpp>
#include <utility>

namespace Manganese {

namespace ast {

enum class StatementKind {
    AggregateDeclarationStatement,
    AliasStatement,
    BreakStatement,
    ContinueStatement,
    EmptyStatement,
    EnumDeclarationStatement,
    ExpressionStatement,
    FunctionDeclarationStatement,
    IfStatement,
    RepeatLoopStatement,
    ReturnStatement,
    SwitchStatement,
    VariableDeclarationStatement,
    WhileLoopStatement
};

struct AggregateField {
    std::string name;
    TypeSPtr_t type;

    AggregateField(std::string name_, TypeSPtr_t type_) : name(std::move(name_)), type(std::move(type_)) {}
};

class AggregateDeclarationStatement : public Statement {
   public:
    std::string name;
    std::vector<std::string> genericTypes;
    std::vector<AggregateField> fields;
    Visibility visibility = Visibility::Private;

    AggregateDeclarationStatement(std::string name_, std::vector<std::string> genericTypes_,
                                  std::vector<AggregateField> fields_) :
        name(std::move(name_)), genericTypes(std::move(genericTypes_)), fields(std::move(fields_)) {}
    AST_STANDARD_INTERFACE;
    constexpr StatementKind kind() const noexcept override { return StatementKind::AggregateDeclarationStatement; };
};

class AliasStatement : public Statement {
   public:
    TypeSPtr_t baseType;  // The type being aliased (x in alias x as foo)
    std::string alias;  // The name of the alias (foo in alias x as foo)
    Visibility visibility = Visibility::Private;

    AliasStatement(TypeSPtr_t baseType_, std::string alias_) :
        baseType(std::move(baseType_)), alias(std::move(alias_)) {}

    AST_STANDARD_INTERFACE
    constexpr StatementKind kind() const noexcept override { return StatementKind::AliasStatement; };
};

class BreakStatement : public Statement {
   public:
    BreakStatement() = default;

    AST_STANDARD_INTERFACE;
    constexpr StatementKind kind() const noexcept override { return StatementKind::BreakStatement; };
};

class ContinueStatement : public Statement {
   public:
    ContinueStatement() = default;

    AST_STANDARD_INTERFACE;
    constexpr StatementKind kind() const noexcept override { return StatementKind::ContinueStatement; };
};

class EmptyStatement : public Statement {
   public:
    EmptyStatement() = default;
    AST_STANDARD_INTERFACE;
    constexpr StatementKind kind() const noexcept override { return StatementKind::EmptyStatement; };
};

struct EnumValue {
    std::string name;
    ExpressionUPtr_t value;
    explicit EnumValue(std::string name_, ExpressionUPtr_t value_ = nullptr) :
        name(std::move(name_)), value(std::move(value_)) {}
};

class EnumDeclarationStatement : public Statement {
   public:
    std::string name;
    TypeSPtr_t baseType;
    std::vector<EnumValue> values;
    Visibility visibility = Visibility::Private;

    EnumDeclarationStatement(std::string name_, TypeSPtr_t baseType_, std::vector<EnumValue> values_) :
        name(name_), baseType(std::move(baseType_)), values(std::move(values_)) {}

    AST_STANDARD_INTERFACE;
    constexpr StatementKind kind() const noexcept override { return StatementKind::EnumDeclarationStatement; };
};

/**
 * @brief Wrapper class to convert an expression into a statement
 */
class ExpressionStatement : public Statement {
   public:
    ExpressionUPtr_t expression;

    explicit ExpressionStatement(ExpressionUPtr_t expression_) : expression(std::move(expression_)) {};

    AST_STANDARD_INTERFACE;
    constexpr StatementKind kind() const noexcept override { return StatementKind::ExpressionStatement; };
};

struct FunctionParameter {
    std::string name;
    TypeSPtr_t type;
    bool isConst;

    FunctionParameter(std::string name_, TypeSPtr_t type_, bool isConst_) :
        name(std::move(name_)), type(std::move(type_)), isConst(isConst_) {}
};

class FunctionDeclarationStatement : public Statement {
   public:
    std::string name;
    std::vector<std::string> genericTypes;
    std::vector<FunctionParameter> parameters;
    TypeSPtr_t returnType;
    Block body;
    Visibility visibility = Visibility::Private;

    FunctionDeclarationStatement(std::string name_, std::vector<std::string> genericTypes_,
                                 std::vector<FunctionParameter> parameters_, TypeSPtr_t returnType_, Block body_) :
        name(std::move(name_)),
        genericTypes(std::move(genericTypes_)),
        parameters(std::move(parameters_)),
        returnType(std::move(returnType_)),
        body(std::move(body_)) {}

    AST_STANDARD_INTERFACE;
    constexpr StatementKind kind() const noexcept override { return StatementKind::FunctionDeclarationStatement; };
};

struct ElifClause {
    ExpressionUPtr_t condition;
    Block body;

    ElifClause(ExpressionUPtr_t condition_, Block body_) : condition(std::move(condition_)), body(std::move(body_)) {}
};

class IfStatement : public Statement {
   public:
    ExpressionUPtr_t condition;
    Block body, elseBody;  // elseBody might be empty
    std::vector<ElifClause> elifs;

    IfStatement(ExpressionUPtr_t condition_, Block body_, std::vector<ElifClause> elifs_, Block elseBody_ = {}) :
        condition(std::move(condition_)),
        body(std::move(body_)),
        elseBody(std::move(elseBody_)),
        elifs(std::move(elifs_)) {}

    AST_STANDARD_INTERFACE;
    constexpr StatementKind kind() const noexcept override { return StatementKind::IfStatement; };
};

class RepeatLoopStatement : public Statement {
   public:
    ExpressionUPtr_t numIterations;
    Block body;

    RepeatLoopStatement(ExpressionUPtr_t numIterations_, Block body_) :
        numIterations(std::move(numIterations_)), body(std::move(body_)) {}

    AST_STANDARD_INTERFACE;
    constexpr StatementKind kind() const noexcept override { return StatementKind::RepeatLoopStatement; };
};

class ReturnStatement : public Statement {
   public:
    ExpressionUPtr_t value;

    explicit ReturnStatement(ExpressionUPtr_t value_ = nullptr) : value(std::move(value_)) {}

    AST_STANDARD_INTERFACE;
    constexpr StatementKind kind() const noexcept override { return StatementKind::ReturnStatement; };
};

struct CaseClause {
    ExpressionUPtr_t literalValue;
    Block body;

    CaseClause(ExpressionUPtr_t literalValue_, Block body_) :
        literalValue(std::move(literalValue_)), body(std::move(body_)) {}
};

class SwitchStatement : public Statement {
   public:
    ExpressionUPtr_t variable;
    std::vector<CaseClause> cases;
    Block defaultBody;

    SwitchStatement(ExpressionUPtr_t variable_, std::vector<CaseClause> cases_, Block defaultBody_ = {}) :
        variable(std::move(variable_)), cases(std::move(cases_)), defaultBody(std::move(defaultBody_)) {}

    AST_STANDARD_INTERFACE;
    constexpr StatementKind kind() const noexcept override { return StatementKind::SwitchStatement; };
};

class VariableDeclarationStatement : public Statement {
   public:
    bool isConst;
    std::string name;
    Visibility visibility;
    ExpressionUPtr_t value;
    TypeSPtr_t type;

    VariableDeclarationStatement(bool isConst_, std::string name_, Visibility visibility_, ExpressionUPtr_t _value,
                                 TypeSPtr_t _type) :
        isConst(isConst_),
        name(std::move(name_)),
        visibility(visibility_),
        value(std::move(_value)),
        type(std::move(_type)) {}

    bool isConstant() const { return isConst; }

    AST_STANDARD_INTERFACE;
    constexpr StatementKind kind() const noexcept override { return StatementKind::VariableDeclarationStatement; };
};

class WhileLoopStatement : public Statement {
    public:

    Block body;
    ExpressionUPtr_t condition;
    bool isDoWhile;

    WhileLoopStatement(Block body_, ExpressionUPtr_t condition_, bool isDoWhile_ = false) :
        body(std::move(body_)), condition(std::move(condition_)), isDoWhile(isDoWhile_) {}

    AST_STANDARD_INTERFACE;
    constexpr StatementKind kind() const noexcept override { return StatementKind::WhileLoopStatement; };
};

}  // namespace ast

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_AST_STATEMENTS_HPP