#ifndef MANGANESE_INCLUDE_FRONTEND_AST_AST_STATEMENTS_HPP
#define MANGANESE_INCLUDE_FRONTEND_AST_AST_STATEMENTS_HPP

#include <cstddef>
#include <frontend/ast/ast_base.hpp>
#include <frontend/lexer/token.hpp>
#include <mnstl/slice.hxx>
#include <optional>
#include <utils/string_interner.hpp>


namespace Manganese::ast {

struct Declaration : public Statement {
    utils::StringID name;
    utils::OptionalStringID mangledName;
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
    mnstl::Slice<utils::StringID> genericTypes;
    mnstl::Slice<AggregateField> fields;
    Visibility visibility = Visibility::Private;

    AggregateDeclarationStatement(utils::StringID _name, mnstl::Slice<utils::StringID> _genericTypes,
                                  mnstl::Slice<AggregateField> _fields) noexcept :
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
    mnstl::Slice<EnumValue> values;
    Visibility visibility = Visibility::Private;

    EnumDeclarationStatement(utils::StringID _name, Type* _baseType, mnstl::Slice<EnumValue> _values) noexcept :
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

struct ForLoopStatement final : public Statement {
    Statement* initializationStep;
    Expression* stopCondition;
    Expression* postExpression;
    Block body;

    ForLoopStatement(Statement* _initializationStep, Expression* _stopCondition, Expression* _postExpression,
                     Block _body) noexcept :
        Statement(StatementKind::ForLoopStatement),
        initializationStep(_initializationStep),
        stopCondition(_stopCondition),
        postExpression(_postExpression),
        body(_body) {}

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
    mnstl::Slice<utils::StringID> genericTypes;
    mnstl::Slice<FunctionParameter> parameters;
    Type* returnType;
    Block body;
    Visibility visibility = Visibility::Private;

    FunctionDeclarationStatement(utils::StringID _name, mnstl::Slice<utils::StringID> _genericTypes,
                                 mnstl::Slice<FunctionParameter> _parameters, Type* _returnType, Block _body) noexcept :
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
};

struct IfStatement final : public Statement {
    Expression* condition;
    Block body;
    std::optional<Block> elseBody;  // elseBody might be empty
    mnstl::Slice<ElifClause> elifs;

    IfStatement(Expression* _condition, Block _body, mnstl::Slice<ElifClause> _elifs, std::optional<Block> _elseBody = {}) noexcept :
        Statement(StatementKind::IfStatement), condition(_condition), body(_body), elseBody(_elseBody), elifs(_elifs) {}

    MN_AST_STANDARD_INTERFACE(IfStatement);
};

struct ImportStatement final : public Statement {
    mnstl::Slice<utils::StringID> path;
    utils::OptionalStringID alias;

    ImportStatement(mnstl::Slice<utils::StringID> _path, utils::OptionalStringID _alias) noexcept :
        Statement(StatementKind::ImportStatement), path(_path), alias(_alias) {}

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

    NamespaceStatement(utils::StringID _name, Block _block) noexcept :
        Statement(StatementKind::NamespaceStatement), name(_name), block(_block) {}

    MN_AST_STANDARD_INTERFACE(NamespaceStatement);
};

struct NestedBlockStatement final : public Statement {
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
    mnstl::Slice<Expression*> values;
    Block body;
};

struct SwitchStatement final : public Statement {
    Expression* target;
    mnstl::Slice<CaseClause> cases;
    std::optional<Block> defaultBody;

    SwitchStatement(Expression* _target, mnstl::Slice<CaseClause> _cases, std::optional<Block> _defaultBody = {}) noexcept :
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

struct WhileLoopStatement final : public Statement {
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

inline EmptyStatement* getEmptyStatement() noexcept {
    static EmptyStatement instance{};
    return &instance;
}

}  // namespace Manganese::ast

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_AST_STATEMENTS_HPP