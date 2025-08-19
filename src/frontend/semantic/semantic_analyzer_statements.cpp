/**
 * @file semantic_analyzer_statements.cpp
 * @brief Responsible for performing semantic analysis on all the AST statement nodes
 */

#include <frontend/ast.hpp>
#include <frontend/semantic/semantic_analyzer.hpp>
#include <global_macros.hpp>

#include <format>
#include "frontend/ast/ast_statements.hpp"

namespace Manganese {
namespace semantic {
// ===== Statement checks that don't fit into another file =====

void SemanticAnalyzer::visit(ast::AliasStatement* statement) {
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
    symbolTable.declare(Symbol{
        .name = statement->alias,
        .kind = SymbolKind::TypeAlias,
        .type = statement->baseType,
        .line = statement->getLine(),
        .column = statement->getColumn(),
        .declarationNode = statement,
        .isConstant = false,  // Type aliases are not constants
        .scopeDepth = symbolTable.currentScopeDepth(),
        .visibility = statement->visibility,
        });
}

void SemanticAnalyzer::visit(ast::ExpressionStatement* statement) {
    visit(statement->expression.get());
}

}  // namespace semantic
}  // namespace Manganese
