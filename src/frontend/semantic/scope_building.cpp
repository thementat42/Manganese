#include <core.hpp>
#include <frontend/ast.hpp>
#include <frontend/semantic.hpp>
#include <io/logging.hpp>
#include <string_view>
#include <utils/resolution_status.hpp>
#include <utils/result.hpp>

namespace Manganese::semantic {

// Note: all types are nullptr for the moment since this is just meant to collect names
// Types are set later on

Result SemanticAnalyzer::buildScopeTree(parser::ParsedFile& file) {
    // first pass -- collect all user-defined types
    Result result = Result::Success;
    for (ast::Statement* stmt : file.program) {
        if (_buildStatementScope(stmt) == Result::Failure) { result = Result::Failure; }
    }
    return result;
}

Result SemanticAnalyzer::_buildStatementScope(ast::Statement* stmt) {
    using enum ast::StatementKind;

    auto reportRedeclaration = [](std::string_view redeclaredSymbolName, ast::ASTNode* node) {
        logging::logError(node->line, node->column, "'{}' was already declared in this scope", redeclaredSymbolName);
    };

    switch (stmt->kind) {
        case AggregateDeclarationStatement: {
            auto* aggregateStmt = static_cast<ast::AggregateDeclarationStatement*>(stmt);

            // Creates an Aggregate shell with name, empty fields ({}), and ResolutionStatus::Unresolved
            const SemanticType* shellType
                = typeContext.getNamedAggregate(interner.get_view(aggregateStmt->name), /*fieldTypes=*/{});

            const Result result = symbolTable.declare(aggregateStmt->name,
                                                      Symbol{
                                                          .type = shellType,
                                                          .node = aggregateStmt,
                                                          .kind = SymbolKind::Aggregate,
                                                          .visibility = aggregateStmt->visibility,
                                                          .isMutable = false,
                                                      });
            if (result == Result::Failure) {
                reportRedeclaration(interner.get_view(aggregateStmt->name), aggregateStmt);
            }
            return result;
        }
        case AliasStatement: {
            auto* aliasStmt = static_cast<ast::AliasStatement*>(stmt);

            const Result result = symbolTable.declare(aliasStmt->name,
                                                      Symbol{
                                                          .type = nullptr,
                                                          .node = aliasStmt,
                                                          .kind = SymbolKind::TypeAlias,
                                                          .visibility = aliasStmt->visibility,
                                                          .isMutable = false,
                                                      });
            if (result == Result::Failure) { reportRedeclaration(interner.get_view(aliasStmt->name), aliasStmt); }
            return result;
        }
        case EnumDeclarationStatement: {
            auto* enumDecl = static_cast<ast::EnumDeclarationStatement*>(stmt);
            const SemanticType* enumType = typeContext.getEnum(interner.get_view(enumDecl->name));
            const Result result = symbolTable.declare(enumDecl->name,
                                                      Symbol{
                                                          .type = enumType,
                                                          .node = enumDecl,
                                                          .kind = SymbolKind::Enum,
                                                          .visibility = enumDecl->visibility,
                                                          .isMutable = false,
                                                      });
            if (result == Result::Failure) { reportRedeclaration(interner.get_view(enumDecl->name), enumDecl); }
            return result;
        }
        case ForLoopStatement: {
            auto* forLoopStmt = static_cast<ast::ForLoopStatement*>(stmt);
            return _buildBodyScope(forLoopStmt->body, &forLoopStmt->associatedScope);
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
            if (result == Result::Failure) { reportRedeclaration(interner.get_view(funcStmt->name), funcStmt); }

            if (!funcStmt->genericTypes.empty()) { return result; }

            // Process the internal block statements
            const Result bodyResult = _buildBodyScope(funcStmt->body, &funcStmt->associatedScope);
            return (result == Result::Success && bodyResult == Result::Success) ? Result::Success : Result::Failure;
        }
        case IfStatement: {
            auto* ifStmt = static_cast<ast::IfStatement*>(stmt);
            Result result = Result::Success;
            if (_buildBodyScope(ifStmt->body, &ifStmt->associatedScope) == Result::Failure) {
                result = Result::Failure;
            }
            for (ast::ElifClause& elif : ifStmt->elifs) {
                if (_buildBodyScope(elif.body, &elif.associatedScope) == Result::Failure) { result = Result::Failure; }
            }
            if (ifStmt->elseBody.has_value()
                && _buildBodyScope(*ifStmt->elseBody, &ifStmt->elseAssociatedScope) == Result::Failure) {
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
            return _buildBodyScope(block->block, &block->associatedScope);
        }
        case SwitchStatement: {
            auto* switchStmt = static_cast<ast::SwitchStatement*>(stmt);
            Result result = Result::Success;
            for (ast::CaseClause& clause : switchStmt->cases) {
                if (_buildBodyScope(clause.body, &clause.associatedScope) == Result::Failure) {
                    result = Result::Failure;
                }
            }
            if (switchStmt->defaultBody.has_value()
                && _buildBodyScope(*switchStmt->defaultBody, &switchStmt->defaultAssociatedScope) == Result::Failure) {
                result = Result::Failure;
            }
            return result;
        }
        case WhileLoopStatement: {
            auto* whileStmt = static_cast<ast::WhileLoopStatement*>(stmt);
            return _buildBodyScope(whileStmt->body, &whileStmt->associatedScope);
        }
        default: return Result::Success;  // Statements that don't introduce scopes or declare types
    }
}

Result SemanticAnalyzer::_buildBodyScope(ast::Block block, Scope** outScope) {
    symbolTable.enterScope();
    if (outScope != nullptr) { *outScope = symbolTable.getCurrentScope(); }
    Result result = Result::Success;

    for (ast::Statement* statement : block) {
        if (_buildStatementScope(statement) == Result::Failure) { result = Result::Failure; }
    }

    symbolTable.exitScope();
    return result;
}

Result SemanticAnalyzer::_buildNamespaceScope(ast::NamespaceStatement* node) {
    symbolTable.enterNamespace(node->name, node);
    node->associatedScope = symbolTable.getCurrentScope();
    Result result = Result::Success;

    for (ast::Statement* subStatement : node->block) {
        if (_buildStatementScope(subStatement) == Result::Failure) { result = Result::Failure; }
    }

    symbolTable.exitNamespace();
    return result;
}

}  // namespace Manganese::semantic