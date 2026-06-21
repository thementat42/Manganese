#include <frontend/ast.hpp>
#include <frontend/semantic.hpp>
#include <core.hpp>

namespace Manganese {

namespace semantic {

// Note: all types are nullptr for the moment since this is just meant to collect names
// Types are set later on

void analyzer::_collectTypesInStatement(ast::Statement* stmt) {
    using enum ast::StatementKind;

    switch (stmt->kind) {
        case AggregateDeclarationStatement: {
            auto aggregateStmt = static_cast<ast::AggregateDeclarationStatement*>(stmt);
            symbolTable.declare(aggregateStmt->name,
                          Symbol{
                              .type = nullptr,
                              .node = aggregateStmt,
                              .kind = SymbolKind::Aggregate,
                              .visibility = aggregateStmt->visibility,
                              .isMutable = false,
                          });
            break;
        }
        case AliasStatement: {
            auto aliasStmt = static_cast<ast::AliasStatement*>(stmt);
            symbolTable.declare(aliasStmt->alias,
                          Symbol{.type = nullptr,
                                 .node = aliasStmt,
                                 .kind = SymbolKind::TypeAlias,
                                 .visibility = aliasStmt->visibility,
                                 .isMutable = false});
            break;
        }
        case EnumDeclarationStatement: {
            auto enumDecl = static_cast<ast::EnumDeclarationStatement*>(stmt);
            symbolTable.declare(enumDecl->name,
                          Symbol{.type = nullptr,
                                 .node = enumDecl,
                                 .kind = SymbolKind::Enum,
                                 .visibility = enumDecl->visibility,
                                 .isMutable = false});
            break;
        }
        case FunctionDeclarationStatement:  // statements with bodies (which can have type declarations)
        case IfStatement:
        case WhileLoopStatement: _collectTypesInStatementBody(stmt); break;
        default: return;  // not a type that requires collection
    }
}

void analyzer::_collectTypesInStatementBody(ast::Statement* stmt) {
    this->symbolTable.enterScope();
    using enum ast::StatementKind;
    switch (stmt->kind) {
        case AggregateDeclarationStatement: {
            auto aggregateStmt = static_cast<ast::AggregateDeclarationStatement*>(stmt);
            symbolTable.declare(aggregateStmt->name,
                          Symbol{.type = nullptr,
                                 .node = aggregateStmt,
                                 .kind = SymbolKind::Aggregate,
                                 .visibility = aggregateStmt->visibility,
                                 .isMutable = false});
            break;
        }
        case AliasStatement: {
            auto aliasStmt = static_cast<ast::AliasStatement*>(stmt);
            symbolTable.declare(aliasStmt->alias,
                          Symbol{.type = nullptr,
                                 .node = aliasStmt,
                                 .kind = SymbolKind::TypeAlias,
                                 .visibility = aliasStmt->visibility,
                                 .isMutable = false});
            break;
        }
        case EnumDeclarationStatement: {
            auto enumDecl = static_cast<ast::EnumDeclarationStatement*>(stmt);
            symbolTable.declare(enumDecl->name,
                          Symbol{.type = nullptr,
                                 .node = enumDecl,
                                 .kind = SymbolKind::Enum,
                                 .visibility = enumDecl->visibility,
                                 .isMutable = false});
            break;
        }
        case FunctionDeclarationStatement: {
            auto _stmtWithBody = static_cast<ast::FunctionDeclarationStatement*>(stmt);
            for (const auto& subStatement : _stmtWithBody->body) { _collectTypesInStatementBody(subStatement); }
            break;
        }
        case IfStatement: {
            auto _stmtWithBody = static_cast<ast::IfStatement*>(stmt);
            for (const auto& subStatement : _stmtWithBody->body) { _collectTypesInStatementBody(subStatement); }
            break;
        }
        case WhileLoopStatement: {
            auto _stmtWithBody = static_cast<ast::WhileLoopStatement*>(stmt);
            for (const auto& subStatement : _stmtWithBody->body) { _collectTypesInStatementBody(subStatement); }
            break;
        }
        default: ASSERT_UNREACHABLE("");
    }
    this->symbolTable.exitScope();
}

}  // namespace semantic

}  // namespace Manganese