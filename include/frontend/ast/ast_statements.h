

/**
 * @file ast_statements.h
 * @brief Contains AST node definitions for various statement types in the Manganese frontend.
 *
 * This header declares the core statement node types used in the AST.
 * Each statement type (conditionals, function declarations, etc.) is represented as a class inheriting from Statement.
 *
 * ! The nodes are listed in alphabetical order.
 */
#ifndef MANGANESE_INCLUDE_FRONTEND_AST_AST_STATEMENTS_H
#define MANGANESE_INCLUDE_FRONTEND_AST_AST_STATEMENTS_H

#include <frontend/ast/ast_base.h>
#include <frontend/ast/ast_expressions.h>
#include <frontend/ast/ast_types.h>

#include <utility>

namespace Manganese {

namespace ast {

enum class StatementKind {
    AliasStatement,
    BreakStatement,
    BundleDeclarationStatement,
    ContinueStatement,
    EnumDeclarationStatement,
    ExpressionStatement,
    FunctionDeclarationStatement,
    IfStatement,
    ImportStatement,
    ModuleDeclarationStatement,
    RepeatLoopStatement,
    ReturnStatement,
    SwitchStatement,
    VariableDeclarationStatement,
    WhileLoopStatement
};

class AliasStatement : public Statement {
   protected:
    TypePtr baseType;
    std::string alias;

   public:
    AliasStatement(TypePtr baseType_, std::string alias_)
        : baseType(std::move(baseType_)), alias(std::move(alias_)) {}

    NODE_OVERRIDES
    StatementKind kind() const noexcept override { return StatementKind::AliasStatement; };
};

class BreakStatement : public Statement {
   public:
    BreakStatement() = default;

    NODE_OVERRIDES;
    StatementKind kind() const noexcept override { return StatementKind::BreakStatement; };
};

struct BundleField {
    std::string name;
    TypePtr type;

    BundleField(std::string name_, TypePtr type_)
        : name(std::move(name_)), type(std::move(type_)) {}
};

class BundleDeclarationStatement : public Statement {
   protected:
    std::string name;
    std::vector<std::string> genericTypes;
    std::vector<BundleField> fields;

   public:
    BundleDeclarationStatement(std::string name_, std::vector<std::string> genericTypes_, std::vector<BundleField> fields_)
        : name(std::move(name_)), genericTypes(std::move(genericTypes_)), fields(std::move(fields_)) {}
    NODE_OVERRIDES;
    StatementKind kind() const noexcept override { return StatementKind::BundleDeclarationStatement; };
};

class ContinueStatement : public Statement {
   public:
    ContinueStatement() = default;

    NODE_OVERRIDES;
    StatementKind kind() const noexcept override { return StatementKind::ContinueStatement; };
};

struct EnumValue {
    std::string name;
    ExpressionPtr value;
    explicit EnumValue(std::string name_, ExpressionPtr value_ = nullptr)
        : name(std::move(name_)), value(std::move(value_)) {}
};

class EnumDeclarationStatement : public Statement {
   protected:
    std::string name;
    TypePtr baseType;
    std::vector<EnumValue> values;

   public:
    EnumDeclarationStatement(std::string name_, TypePtr baseType_, std::vector<EnumValue> values_)
        : name(name_), baseType(std::move(baseType_)), values(std::move(values_)) {}

    NODE_OVERRIDES;
    StatementKind kind() const noexcept override { return StatementKind::EnumDeclarationStatement; };
};

/**
 * @brief Wrapper class to convert an expression into a statement
 */
class ExpressionStatement : public Statement {
   protected:
    ExpressionPtr expression;

   public:
    explicit ExpressionStatement(ExpressionPtr expression_) : expression(std::move(expression_)) {};

    NODE_OVERRIDES;
    StatementKind kind() const noexcept override { return StatementKind::ExpressionStatement; };
};

struct FunctionParameter {
    std::string name;
    TypePtr type;
    bool isConst;

    FunctionParameter(std::string name_, TypePtr type_, bool isConst_) : name(std::move(name_)), type(std::move(type_)), isConst(isConst_) {}
};

class FunctionDeclarationStatement : public Statement {
   protected:
    std::string name;
    std::vector<std::string> genericTypes;
    std::vector<FunctionParameter> parameters;
    TypePtr returnType;
    Block body;

   public:
    FunctionDeclarationStatement(std::string name_, std::vector<std::string> genericTypes_, std::vector<FunctionParameter> parameters_, TypePtr returnType_, Block body_)
        : name(std::move(name_)), genericTypes(std::move(genericTypes_)), parameters(std::move(parameters_)), returnType(std::move(returnType_)), body(std::move(body_)) {}

    NODE_OVERRIDES;
    StatementKind kind() const noexcept override { return StatementKind::FunctionDeclarationStatement; };
};

struct ElifClause {
    ExpressionPtr condition;
    Block body;

    ElifClause(ExpressionPtr condition_, Block body_) : condition(std::move(condition_)), body(std::move(body_)) {}
};

class IfStatement : public Statement {
   protected:
    ExpressionPtr condition;
    Block body, elseBody;  // elseBody might be empty
    std::vector<ElifClause> elifs;

   public:
    IfStatement(ExpressionPtr condition_, Block body_, std::vector<ElifClause> elifs_, Block elseBody_ = {})
        : condition(std::move(condition_)), body(std::move(body_)), elseBody(std::move(elseBody_)), elifs(std::move(elifs_)) {}

    NODE_OVERRIDES;
    StatementKind kind() const noexcept override { return StatementKind::IfStatement; };
};

/**
 * @note Imports are stored separately in the parser, so this holds no data -- it's just here for compatibility with the other statements
 */
class ImportStatement : public Statement {
   public:
    ImportStatement() = default;
    NODE_OVERRIDES;
    StatementKind kind() const noexcept override { return StatementKind::ImportStatement; };
};

/**
 * @note The module name is stored separately in the parser, so this holds no data -- it's just here for compatibility with the other statements
 */
class ModuleDeclarationStatement : public Statement {
   public:
    ModuleDeclarationStatement() = default;
    NODE_OVERRIDES;
    StatementKind kind() const noexcept override { return StatementKind::ModuleDeclarationStatement; };
};

class RepeatLoopStatement : public Statement {
   protected:
    ExpressionPtr numIterations;
    Block body;

   public:
    RepeatLoopStatement(ExpressionPtr numIterations_, Block body_)
        : numIterations(std::move(numIterations_)), body(std::move(body_)) {}

    NODE_OVERRIDES;
    StatementKind kind() const noexcept override { return StatementKind::RepeatLoopStatement; };
};

class ReturnStatement : public Statement {
   protected:
    ExpressionPtr value;

   public:
    explicit ReturnStatement(ExpressionPtr value_ = nullptr) : value(std::move(value_)) {}

    NODE_OVERRIDES;
    StatementKind kind() const noexcept override { return StatementKind::ReturnStatement; };
};

struct CaseClause {
    ExpressionPtr literalValue;
    Block body;

    CaseClause(ExpressionPtr literalValue_, Block body_) : literalValue(std::move(literalValue_)), body(std::move(body_)) {}
};

class SwitchStatement : public Statement {
   protected:
    ExpressionPtr variable;
    std::vector<CaseClause> cases;
    Block defaultBody;

   public:
    SwitchStatement(ExpressionPtr variable_, std::vector<CaseClause> cases_, Block defaultBody_ = {})
        : variable(std::move(variable_)), cases(std::move(cases_)), defaultBody(std::move(defaultBody_)) {}

    NODE_OVERRIDES;
    StatementKind kind() const noexcept override { return StatementKind::SwitchStatement; };
};

class VariableDeclarationStatement : public Statement {
   protected:
    bool isConst;
    std::string name;
    Visibility visibility;
    ExpressionPtr value;
    TypePtr type;

   public:
    VariableDeclarationStatement(bool isConst_, std::string name_, Visibility visibility_,
                                 ExpressionPtr value_, TypePtr type_ = nullptr);

    bool isConstant() const { return isConst; }

    NODE_OVERRIDES;
    StatementKind kind() const noexcept override { return StatementKind::VariableDeclarationStatement; };
};

class WhileLoopStatement : public Statement {
   protected:
    Block body;
    ExpressionPtr condition;
    bool isDoWhile;

   public:
    WhileLoopStatement(Block body_, ExpressionPtr condition_, bool isDoWhile_ = false)
        : body(std::move(body_)), condition(std::move(condition_)), isDoWhile(isDoWhile_) {}

    NODE_OVERRIDES;
    StatementKind kind() const noexcept override { return StatementKind::WhileLoopStatement; };
};

}  // namespace ast

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_AST_STATEMENTS_H