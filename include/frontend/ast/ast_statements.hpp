#ifndef MANGANESE_INCLUDE_FRONTEND_AST_AST_STATEMENTS_HPP
#define MANGANESE_INCLUDE_FRONTEND_AST_AST_STATEMENTS_HPP

#include <cstddef>
#include <frontend/ast/ast_base.hpp>
#include <frontend/lexer/token.hpp>
#include <optional>
#include <utility>
#include <vector>
#include <utils/string_interner.hpp>

namespace Manganese::ast {

struct Declaration : public Statement {
    utils::StringID name;
    utils::StringID mangledName;
    const semantic::SemanticType* semanticType = nullptr;

    Declaration(StatementKind _kind, utils::StringID _name) noexcept : Statement(_kind), name(_name) {}
    virtual ~Declaration() = default;
};

struct AggregateField {
    utils::StringID name;
    Type* type;
    std::size_t line, column;
    bool isMutable;
};

struct AggregateDeclarationStatement final : public Declaration {
    std::vector<utils::StringID> genericTypes;
    std::vector<AggregateField> fields;
    Visibility visibility = Visibility::Private;

    AggregateDeclarationStatement(utils::StringID _name, std::vector<utils::StringID>&& _genericTypes,
                                  std::vector<AggregateField>&& _fields) noexcept :
        Declaration(StatementKind::AggregateDeclarationStatement, _name),
        genericTypes(std::move(_genericTypes)),
        fields(std::move(_fields)) {}
    MN_AST_STANDARD_INTERFACE(AggregateDeclarationStatement);

    bool isDeclaration() const noexcept override { return true; }
};

struct AliasStatement final : public Declaration {
    Type* baseType;
    Visibility visibility = Visibility::Private;

    AliasStatement(Type* _baseType, utils::StringID _alias) noexcept :
        Declaration(StatementKind::AliasStatement, _alias), baseType(_baseType) {}

    MN_AST_STANDARD_INTERFACE(AliasStatement);
    bool isDeclaration() const noexcept override { return true; }
};

struct BreakStatement final : public Statement {
    explicit BreakStatement() noexcept : Statement(StatementKind::BreakStatement) {}

    MN_AST_STANDARD_INTERFACE(BreakStatement);
};

struct ContinueStatement final : public Statement {
    explicit ContinueStatement() noexcept : Statement(StatementKind::ContinueStatement) {}

    MN_AST_STANDARD_INTERFACE(ContinueStatement);
};

struct EmptyStatement final : public Statement {
    explicit EmptyStatement() noexcept : Statement(StatementKind::EmptyStatement) {}
    MN_AST_STANDARD_INTERFACE(EmptyStatement);
};

struct EnumValue {
    utils::StringID name;
    Expression* value;
    std::size_t line, column;
};

struct EnumDeclarationStatement final : public Declaration {
    Type* baseType;
    std::vector<EnumValue> values;
    Visibility visibility = Visibility::Private;

    EnumDeclarationStatement(utils::StringID _name, Type* _baseType, std::vector<EnumValue>&& _values) noexcept :
        Declaration(StatementKind::EnumDeclarationStatement, _name),
        baseType(_baseType),
        values(std::move(_values)) {}

    MN_AST_STANDARD_INTERFACE(EnumDeclarationStatement);
    bool isDeclaration() const noexcept override { return true; }
};

/**
 * Wrapper struct to convert an expression into a statement
 */
struct ExpressionStatement final : public Statement {
    Expression* expression;

    explicit ExpressionStatement(Expression* _expression) noexcept :
        Statement(StatementKind::ExpressionStatement), expression(_expression) {}

    MN_AST_STANDARD_INTERFACE(ExpressionStatement);
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

    MN_AST_STANDARD_INTERFACE(ForLoopStatement);
};

struct FunctionParameter {
    utils::StringID name;
    Type* type;
    Expression* defaultValue;
    std::size_t line, column;
    bool isMutable;
    bool isVariadic;
};

struct FunctionDeclarationStatement final : public Declaration {
    std::vector<utils::StringID> genericTypes;
    std::vector<FunctionParameter> parameters;
    Type* returnType;
    Block body;
    Visibility visibility = Visibility::Private;

    FunctionDeclarationStatement(utils::StringID _name, std::vector<utils::StringID>&& _genericTypes,
                                 std::vector<FunctionParameter>&& _parameters, Type* _returnType,
                                 Block&& _body) noexcept :
        Declaration(StatementKind::FunctionDeclarationStatement, _name),
        genericTypes(std::move(_genericTypes)),
        parameters(std::move(_parameters)),
        returnType(_returnType),
        body(std::move(_body)) {}

    MN_AST_STANDARD_INTERFACE(FunctionDeclarationStatement);
    bool isDeclaration() const noexcept override { return true; }
};

struct ElifClause {
    Expression* condition;
    Block body;
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

    MN_AST_STANDARD_INTERFACE(IfStatement);
};

struct ImportStatement final : public Statement {
    std::vector<utils::StringID> path;
    std::optional<utils::StringID> alias;

    ImportStatement(std::vector<utils::StringID>&& _path, std::optional<utils::StringID>&& _alias) noexcept :
        Statement(StatementKind::ImportStatement), path(std::move(_path)), alias(_alias) {}

    MN_AST_STANDARD_INTERFACE(ImportStatement);
};

struct ModuleDeclarationStatement final : public Statement {
    utils::StringID name;

    explicit ModuleDeclarationStatement(utils::StringID _name) noexcept :
        Statement(StatementKind::ModuleDeclarationStatement), name(_name) {}

    MN_AST_STANDARD_INTERFACE(ModuleDeclarationStatement);
};

struct NamespaceStatement final : public Statement {
    utils::StringID name;
    Block block;

    NamespaceStatement(utils::StringID _name, Block&& _block) noexcept :
        Statement(StatementKind::NamespaceStatement), name(_name), block(std::move(_block)) {}

    MN_AST_STANDARD_INTERFACE(NamespaceStatement);
};

struct NestedBlockStatement final : public Statement {
    Block block;

    NestedBlockStatement(Block&& _block) noexcept :
        Statement(StatementKind::NestedBlockStatement), block(std::move(_block)) {}
    MN_AST_STANDARD_INTERFACE(NestedBlockStatement);
};

struct ReturnStatement final : public Statement {
    Expression* value;

    explicit ReturnStatement(Expression* _value = nullptr) noexcept :
        Statement(StatementKind::ReturnStatement), value(_value) {}

    MN_AST_STANDARD_INTERFACE(ReturnStatement);
};

struct CaseClause {
    std::vector<Expression*> values;
    Block body;
};

struct SwitchStatement final : public Statement {
    Expression* target;
    std::vector<CaseClause> cases;
    Block defaultBody;

    SwitchStatement(Expression* _target, std::vector<CaseClause>&& _cases, Block&& _defaultBody = {}) noexcept :
        Statement(StatementKind::SwitchStatement),
        target(_target),
        cases(std::move(_cases)),
        defaultBody(std::move(_defaultBody)) {}

    MN_AST_STANDARD_INTERFACE(SwitchStatement);
};

struct VariableDeclarationStatement final : public Declaration {
    Visibility visibility;
    bool isMutable;
    Expression* value;
    Type* type;

    VariableDeclarationStatement(bool _isMutable, utils::StringID _name, Visibility _visibility, Expression* _value,
                                 Type* _type) noexcept :
        Declaration(StatementKind::VariableDeclarationStatement, _name),
        visibility(_visibility),
        isMutable(_isMutable),
        value(_value),
        type(_type) {}

    MN_AST_STANDARD_INTERFACE(VariableDeclarationStatement);
    bool isDeclaration() const noexcept override { return true; }
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

    MN_AST_STANDARD_INTERFACE(WhileLoopStatement);
};

struct PoisonedStatement final : public Statement {
    PoisonedStatement() noexcept : Statement(StatementKind::PoisonedStatement) {}

    MN_AST_STANDARD_INTERFACE(PoisonedStatement);
};

inline EmptyStatement* getEmptyStatement() noexcept {
    static EmptyStatement instance{};
    return &instance;
}

}  // namespace Manganese::ast

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_AST_STATEMENTS_HPP