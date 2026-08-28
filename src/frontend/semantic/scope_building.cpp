#include <core.hpp>
#include <frontend/ast.hpp>
#include <frontend/semantic.hpp>
#include <io/logging.hpp>
#include <string_view>
#include <utils/result.hpp>

#include "frontend/ast/ast_statements.hpp"

namespace Manganese::semantic {

// Note: all types are nullptr for the moment since this is just meant to collect names
// Types are set later on

Result Analyzer::buildScopeTree() {
    // first pass -- collect all user-defined types
    Result result = Result::Success;
    for (ast::Statement* stmt : parsedFile.program) {
        if (_buildStatementScope(stmt) == Result::Failure) { result = Result::Failure; }
    }
    return result;
}

static FORCE_INLINE void _reportRedeclaration(std::string_view redeclaredSymbolName, ast::ASTNode* node) {
    logging::logError(node->line, node->column, "'{}' was already declared in this scope", redeclaredSymbolName);
}

Result Analyzer::_buildStatementScope(ast::Statement* stmt) {
    using enum ast::StatementKind;

    switch (stmt->kind) {
        case AggregateDeclarationStatement: {
            auto* aggregateStmt = static_cast<ast::AggregateDeclarationStatement*>(stmt);

            // Creates an Aggregate shell with name, empty fields ({}), and ResolutionStatus::Unresolved
            const SemanticType* shellType = typeContext.getNamedAggregate(std::string(aggregateStmt->name), /*fields=*/{});

            const Result result = symbolTable.declare(aggregateStmt->name,
                                                      Symbol{
                                                          .type = shellType,
                                                          .node = aggregateStmt,
                                                          .kind = SymbolKind::Aggregate,
                                                          .visibility = aggregateStmt->visibility,
                                                          .isMutable = false,
                                                      });
            if (result == Result::Failure) { _reportRedeclaration(aggregateStmt->name, aggregateStmt); }
            return result;
        }
        case AliasStatement: {
            auto* aliasStmt = static_cast<ast::AliasStatement*>(stmt);

            const Result result = symbolTable.declare(aliasStmt->alias,
                                                      Symbol{
                                                          .type = nullptr,
                                                          .node = aliasStmt,
                                                          .kind = SymbolKind::TypeAlias,
                                                          .visibility = aliasStmt->visibility,
                                                          .isMutable = false,
                                                      });
            if (result == Result::Failure) { _reportRedeclaration(aliasStmt->alias, aliasStmt); }
            return result;
        }
        case EnumDeclarationStatement: {
            auto* enumDecl = static_cast<ast::EnumDeclarationStatement*>(stmt);
            const SemanticType* enumType = typeContext.getEnum(enumDecl->name);
            const Result result = symbolTable.declare(enumDecl->name,
                                                      Symbol{
                                                          .type = enumType,
                                                          .node = enumDecl,
                                                          .kind = SymbolKind::Enum,
                                                          .visibility = enumDecl->visibility,
                                                          .isMutable = false,
                                                      });
            if (result == Result::Failure) { _reportRedeclaration(enumDecl->name, enumDecl); }
            return result;
        }
        case FunctionDeclarationStatement: {
            auto* funcStmt = static_cast<ast::FunctionDeclarationStatement*>(stmt);

            // Register the function itself in the current scope
            auto result = symbolTable.declare(funcStmt->name,
                                              Symbol{
                                                  .type = nullptr,
                                                  .node = funcStmt,
                                                  .kind = SymbolKind::Function,
                                                  .visibility = funcStmt->visibility,
                                                  .isMutable = false,
                                              });
            if (result == Result::Failure) { _reportRedeclaration(funcStmt->name, funcStmt); }

            // Process the internal block statements
            Result bodyResult = _buildBodyScope(funcStmt->body);
            return (result == Result::Success && bodyResult == Result::Success) ? Result::Success : Result::Failure;
        }
        case IfStatement: {
            auto* ifStmt = static_cast<ast::IfStatement*>(stmt);
            Result result = Result::Success;
            if (_buildBodyScope(ifStmt->body) == Result::Failure) { result = Result::Failure; }
            for (const ast::ElifClause& elif : ifStmt->elifs) {
                if (_buildBodyScope(elif.body) == Result::Failure) { result = Result::Failure; }
            }
            if (!ifStmt->elseBody.empty() && _buildBodyScope(ifStmt->elseBody) == Result::Failure) {
                result = Result::Failure;
            }

            return result;
        }
        case NamespaceStatement: {
            auto* namespaceStatement = static_cast<ast::NamespaceStatement*>(stmt);
            return _buildNamespaceScope(namespaceStatement);
        }
        case NestedBlockStatement: {
            auto* block = static_cast<ast::NestedBlockStatement*>(stmt);
            return _buildBodyScope(block->block);
        }
        case SwitchStatement: {
            auto* switchStmt = static_cast<ast::SwitchStatement*>(stmt);
            Result result = Result::Success;
            for (const ast::CaseClause& clause : switchStmt->cases) {
                if (_buildBodyScope(clause.body) == Result::Failure) { result = Result::Failure; }
            }
            if (!switchStmt->defaultBody.empty() && _buildBodyScope(switchStmt->defaultBody) == Result::Failure) {
                result = Result::Failure;
            }
            return result;
        }
        case WhileLoopStatement: {
            auto* whileStmt = static_cast<ast::WhileLoopStatement*>(stmt);
            return _buildBodyScope(whileStmt->body);
        }
        default: return Result::Success;  // Statements that don't introduce scopes or declare types
    }
}

Result Analyzer::_buildBodyScope(const ast::Block& body) {
    symbolTable.enterScope();
    Result result = Result::Success;

    for (ast::Statement* subStatement : body) {
        if (_buildStatementScope(subStatement) == Result::Failure) { result = Result::Failure; }
    }

    symbolTable.exitScope();
    return result;
}

Result Analyzer::_buildNamespaceScope(ast::NamespaceStatement* node) {
    symbolTable.enterNamespace(node->name, node);
    Result result = Result::Success;

    for (ast::Statement* subStatement : node->block) {
        if (_buildStatementScope(subStatement) == Result::Failure) { result = Result::Failure; }
    }

    symbolTable.exitNamespace();
    return result;
}

}  // namespace Manganese::semantic