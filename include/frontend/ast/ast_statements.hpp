#ifndef MANGANESE_INCLUDE_FRONTEND_AST_AST_STATEMENTS_HPP
#define MANGANESE_INCLUDE_FRONTEND_AST_AST_STATEMENTS_HPP

#include <cstddef>
#include <frontend/ast/ast_base.hpp>
#include <frontend/lexer/token.hpp>
#include <optional>
#include <utils/named_parameter.hpp>
#include <utils/slice.hpp>
#include <utils/string_interner.hpp>

namespace Manganese::ast {

struct Declaration : public Statement {
    utils::StringID name;
    utils::OptionalStringID mangledName;
    const semantic::SemanticType* semanticType = nullptr;

    Declaration(StatementKind _kind, utils::StringID _name) noexcept : Statement(_kind), name(_name) {}
    ~Declaration() override = default;
};

struct ScopeOwner {
    semantic::Scope* associatedScope = nullptr;
};

struct AggregateField {
    utils::StringID name;
    Type* type;
    std::size_t line, column;
    bool isMutable;
};

struct AggregateDeclarationStatement final : public Declaration {
    utils::Slice<utils::StringID> genericTypes;
    utils::Slice<AggregateField> fields;
    Visibility visibility = Visibility::Private;

    AggregateDeclarationStatement(utils::StringID _name, utils::Slice<utils::StringID> _genericTypes,
                                  utils::Slice<AggregateField> _fields) noexcept :
        Declaration(StatementKind::AggregateDeclarationStatement, _name),
        genericTypes(_genericTypes),
        fields(_fields) {}
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
    utils::Slice<EnumValue> values;
    Visibility visibility = Visibility::Private;

    EnumDeclarationStatement(utils::StringID _name, Type* _baseType, utils::Slice<EnumValue> _values) noexcept :
        Declaration(StatementKind::EnumDeclarationStatement, _name), baseType(_baseType), values(_values) {}

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

using ForLoopCondition = utils::NamedParameter<Expression*, struct ForCond>;
using ForLoopPost = utils::NamedParameter<Expression*, struct ForPost>;

struct ForLoopStatement final : public Statement, ScopeOwner {
    Statement* initializationStep;
    Expression* stopCondition;
    Expression* postExpression;
    Block body;

    ForLoopStatement(Statement* _initializationStep, ForLoopCondition _stopCondition, ForLoopPost _postExpression,
                     Block _body) noexcept :
        Statement(StatementKind::ForLoopStatement),
        initializationStep(_initializationStep),
        stopCondition(_stopCondition.get()),
        postExpression(_postExpression.get()),
        body(_body) {}

    MN_AST_STANDARD_INTERFACE(ForLoopStatement);
};

struct FunctionParameter {
    utils::StringID name;
    Type* type;
    Expression* defaultValue;
    std::size_t line, column;
    bool isMutable;
};

struct FunctionDeclarationStatement final : public Declaration, ScopeOwner {
    utils::Slice<utils::StringID> genericTypes;
    utils::Slice<FunctionParameter> parameters;
    Type* returnType;
    Block body;
    Visibility visibility = Visibility::Private;

    FunctionDeclarationStatement(utils::StringID _name, utils::Slice<utils::StringID> _genericTypes,
                                 utils::Slice<FunctionParameter> _parameters, Type* _returnType, Block _body) noexcept :
        Declaration(StatementKind::FunctionDeclarationStatement, _name),
        genericTypes(_genericTypes),
        parameters(_parameters),
        returnType(_returnType),
        body(_body) {}

    MN_AST_STANDARD_INTERFACE(FunctionDeclarationStatement);
    bool isDeclaration() const noexcept override { return true; }
};

struct ElifClause {
    Expression* condition;
    Block body;
    semantic::Scope* associatedScope;
};

struct IfStatement final : public Statement, ScopeOwner {
    Expression* condition;
    Block body;
    std::optional<Block> elseBody;  // elseBody might be empty
    semantic::Scope* elseAssociatedScope = nullptr;
    utils::Slice<ElifClause> elifs;

    IfStatement(Expression* _condition, Block _body, utils::Slice<ElifClause> _elifs,
                std::optional<Block> _elseBody = {}) noexcept :
        Statement(StatementKind::IfStatement), condition(_condition), body(_body), elseBody(_elseBody), elifs(_elifs) {}

    MN_AST_STANDARD_INTERFACE(IfStatement);
};

struct ImportStatement final : public Statement {
    utils::Slice<utils::StringID> path;
    utils::OptionalStringID alias;

    ImportStatement(utils::Slice<utils::StringID> _path, utils::OptionalStringID _alias) noexcept :
        Statement(StatementKind::ImportStatement), path(_path), alias(_alias) {}

    MN_AST_STANDARD_INTERFACE(ImportStatement);
};

struct ModuleDeclarationStatement final : public Statement {
    utils::StringID name;

    explicit ModuleDeclarationStatement(utils::StringID _name) noexcept :
        Statement(StatementKind::ModuleDeclarationStatement), name(_name) {}

    MN_AST_STANDARD_INTERFACE(ModuleDeclarationStatement);
};

struct NamespaceStatement final : public Statement, ScopeOwner {
    utils::StringID name;
    Block block;

    NamespaceStatement(utils::StringID _name, Block _block) noexcept :
        Statement(StatementKind::NamespaceStatement), name(_name), block(_block) {}

    MN_AST_STANDARD_INTERFACE(NamespaceStatement);
};

struct NestedBlockStatement final : public Statement, ScopeOwner {
    Block block;

    NestedBlockStatement(Block _block) noexcept : Statement(StatementKind::NestedBlockStatement), block(_block) {}
    MN_AST_STANDARD_INTERFACE(NestedBlockStatement);
};

struct ReturnStatement final : public Statement {
    Expression* value;

    explicit ReturnStatement(Expression* _value = nullptr) noexcept :
        Statement(StatementKind::ReturnStatement), value(_value) {}

    MN_AST_STANDARD_INTERFACE(ReturnStatement);
};

struct CaseClause {
    utils::Slice<Expression*> values;
    Block body;
    semantic::Scope* associatedScope;
};

struct SwitchStatement final : public Statement, ScopeOwner {
    Expression* target;
    utils::Slice<CaseClause> cases;
    std::optional<Block> defaultBody;
    semantic::Scope* defaultAssociatedScope;

    SwitchStatement(Expression* _target, utils::Slice<CaseClause> _cases,
                    std::optional<Block> _defaultBody = {}) noexcept :
        Statement(StatementKind::SwitchStatement), target(_target), cases(_cases), defaultBody(_defaultBody) {}

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

struct WhileLoopStatement final : public Statement, ScopeOwner {
    Block body;
    Expression* condition;
    bool isDoWhile;

    WhileLoopStatement(Block _body, Expression* _condition, bool _isDoWhile = false) noexcept :
        Statement(StatementKind::WhileLoopStatement), body(_body), condition(_condition), isDoWhile(_isDoWhile) {}

    MN_AST_STANDARD_INTERFACE(WhileLoopStatement);
};

struct PoisonedStatement final : public Statement {
    PoisonedStatement() noexcept : Statement(StatementKind::PoisonedStatement) {}

    MN_AST_STANDARD_INTERFACE(PoisonedStatement);
};

}  // namespace Manganese::ast

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_AST_STATEMENTS_HPP