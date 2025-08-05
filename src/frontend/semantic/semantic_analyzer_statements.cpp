/**
 * @file semantic_analyzer_statements.cpp
 * @brief Responsible for performing semantic analysis on all the AST statement nodes
 */

#include <frontend/ast.hpp>
#include <frontend/semantic/semantic_analyzer.hpp>
#include <global_macros.hpp>

#include <format>

namespace Manganese {
namespace semantic {

void SemanticAnalyzer::checkStatement(ast::Statement* statement) noexcept_if_release {
    switch (statement->kind()) {
        case ast::StatementKind::AggregateDeclarationStatement:
            checkAggregateDeclarationStatement(static_cast<ast::AggregateDeclarationStatement*>(statement));
            break;
        case ast::StatementKind::AliasStatement:
            checkAliasStatement(static_cast<ast::AliasStatement*>(statement));
            break;
        case ast::StatementKind::BreakStatement:
            checkBreakStatement(static_cast<ast::BreakStatement*>(statement));
            break;
        case ast::StatementKind::ContinueStatement:
            checkContinueStatement(static_cast<ast::ContinueStatement*>(statement));
            break;
        case ast::StatementKind::EmptyStatement:
            // Empty statements are valid, so there's no need to check them
            break;
        case ast::StatementKind::EnumDeclarationStatement:
            checkEnumDeclarationStatement(static_cast<ast::EnumDeclarationStatement*>(statement));
            break;
        case ast::StatementKind::ExpressionStatement:
            checkExpression(static_cast<ast::ExpressionStatement*>(statement)->expression.get());
            break;
        case ast::StatementKind::FunctionDeclarationStatement:
            checkFunctionDeclarationStatement(static_cast<ast::FunctionDeclarationStatement*>(statement));
            break;
        case ast::StatementKind::IfStatement: checkIfStatement(static_cast<ast::IfStatement*>(statement)); break;
        case ast::StatementKind::RepeatLoopStatement:
            checkRepeatLoopStatement(static_cast<ast::RepeatLoopStatement*>(statement));
            break;
        case ast::StatementKind::ReturnStatement:
            checkReturnStatement(static_cast<ast::ReturnStatement*>(statement));
            break;
        case ast::StatementKind::SwitchStatement:
            checkSwitchStatement(static_cast<ast::SwitchStatement*>(statement));
            break;
        case ast::StatementKind::VariableDeclarationStatement:
            checkVariableDeclarationStatement(static_cast<ast::VariableDeclarationStatement*>(statement));
            break;
        case ast::StatementKind::WhileLoopStatement:
            checkWhileLoopStatement(static_cast<ast::WhileLoopStatement*>(statement));
            break;
        default:
            using std::format;
            ASSERT_UNREACHABLE(
                format("No semantic analysis method for statement type {}", static_cast<int>(statement->kind())));
            break;
    }
}

// The specific statement checks are implemented in the statements/ subdirectory

// Alias statements are checked here because this implementation doesn't really fit into any of the categories

void SemanticAnalyzer::checkAliasStatement(ast::AliasStatement* statement) {
    if (statement->baseType->kind() == ast::TypeKind::GenericType) {
        logError("Generic Types cannot be aliased", statement, toStringOr(statement->baseType));
        return;
    }
    bool isInvalidAlias = false;
    if (!typeExists(statement->baseType)) {
        logError("Base type '{}' for alias '{}' does not exist or was not defined", statement,
                 toStringOr(statement->baseType), statement->alias);
        isInvalidAlias = true;
    }
    if (symbolTable.lookupInCurrentScope(statement->alias)) {
        logError("Alias '{}' already exists in the current scope", statement, statement->alias);
        isInvalidAlias = true;
    }
    if (isInvalidAlias) { return; };
    symbolTable.declare(Symbol{.name = statement->alias,
                               .kind = SymbolKind::TypeAlias,
                               .type = statement->baseType,
                               .line = statement->getLine(),
                               .column = statement->getColumn(),
                               .declarationNode = statement,
                               .isConstant = false,  // Type aliases are not constants
                               .scopeDepth = symbolTable.currentScopeDepth(),
                               .visibility = statement->visibility});
}

}  // namespace semantic
}  // namespace Manganese
