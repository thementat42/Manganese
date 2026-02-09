#include <frontend/ast.hpp>
#include <frontend/semantic.hpp>
#include <global_macros.hpp>
#include <memory>

namespace Manganese {

namespace semantic {

// Note: all types are nullptr for the moment since this is just meant to collect names
// Types are set later on

void analyzer::_collectTypesInStatement(ast::Statement* stmt) {
    using enum ast::StatementKind;

    switch (stmt->kind()) {
        case AggregateDeclarationStatement: {
            auto aggregateStmt = static_cast<ast::AggregateDeclarationStatement*>(stmt);
            table.declare(Symbol{.name = aggregateStmt->name,
                                 .kind = SymbolKind::Aggregate,
                                 .type = nullptr,
                                 .node = aggregateStmt,
                                 .isMutable = false,
                                 .visibility = aggregateStmt->visibility});
            break;
        }
        case AliasStatement: {
            auto aliasStmt = static_cast<ast::AliasStatement*>(stmt);
            table.declare(Symbol{.name = aliasStmt->alias,
                                 .kind = SymbolKind::TypeAlias,
                                 .type = nullptr,
                                 .node = aliasStmt,
                                 .isMutable = false,
                                 .visibility = aliasStmt->visibility});
            break;
        }
        case EnumDeclarationStatement: {
            auto enumDecl = static_cast<ast::EnumDeclarationStatement*>(stmt);
            table.declare(Symbol{.name = enumDecl->name,
                                 .kind = SymbolKind::Enum,
                                 .type = nullptr,
                                 .node = enumDecl,
                                 .isMutable = false,
                                 .visibility = enumDecl->visibility});
            break;
        }
        case FunctionDeclarationStatement:  // statements with bodies (which can have type declarations)
        case IfStatement:
        case RepeatLoopStatement:
        case WhileLoopStatement: _collectTypesInStatementBody(stmt); break;
        default: return;  // not a type that requires collection
    }
}

void analyzer::_collectTypesInStatementBody(ast::Statement* stmt) {
    this->table.enterScope();
    using enum ast::StatementKind;
    switch (stmt->kind()) {
        case AggregateDeclarationStatement: {
            auto aggregateStmt = static_cast<ast::AggregateDeclarationStatement*>(stmt);
            table.declare(Symbol{.name = aggregateStmt->name,
                                 .kind = SymbolKind::Aggregate,
                                 .type = nullptr,
                                 .node = aggregateStmt,
                                 .isMutable = false,
                                 .visibility = aggregateStmt->visibility});
            break;
        }
        case AliasStatement: {
            auto aliasStmt = static_cast<ast::AliasStatement*>(stmt);
            table.declare(Symbol{.name = aliasStmt->alias,
                                 .kind = SymbolKind::TypeAlias,
                                 .type = nullptr,
                                 .node = aliasStmt,
                                 .isMutable = false,
                                 .visibility = aliasStmt->visibility});
            break;
        }
        case EnumDeclarationStatement: {
            auto enumDecl = static_cast<ast::EnumDeclarationStatement*>(stmt);
            table.declare(Symbol{.name = enumDecl->name,
                                 .kind = SymbolKind::Enum,
                                 .type = nullptr,
                                 .node = enumDecl,
                                 .isMutable = false,
                                 .visibility = enumDecl->visibility});
            break;
        }
        case FunctionDeclarationStatement: {
            auto _stmtWithBody = static_cast<ast::FunctionDeclarationStatement*>(stmt);
            for (const auto& subStatement : _stmtWithBody->body) { _collectTypesInStatementBody(subStatement.get()); }
            break;
        }
        case IfStatement: {
            auto _stmtWithBody = static_cast<ast::IfStatement*>(stmt);
            for (const auto& subStatement : _stmtWithBody->body) { _collectTypesInStatementBody(subStatement.get()); }
            break;
        }
        case RepeatLoopStatement: {
            auto _stmtWithBody = static_cast<ast::RepeatLoopStatement*>(stmt);
            for (const auto& subStatement : _stmtWithBody->body) { _collectTypesInStatementBody(subStatement.get()); }
            break;
        }
        case WhileLoopStatement: {
            auto _stmtWithBody = static_cast<ast::WhileLoopStatement*>(stmt);
            for (const auto& subStatement : _stmtWithBody->body) { _collectTypesInStatementBody(subStatement.get()); }
            break;
        }
        default: ASSERT_UNREACHABLE("");
    }
    this->table.exitScope();
}

}  // namespace semantic

}  // namespace Manganese