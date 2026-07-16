#include <core.hpp>
#include <frontend/ast.hpp>
#include <frontend/semantic.hpp>
#include <io/logging.hpp>
#include <string_view>

namespace Manganese {

namespace semantic {

// Note: all types are nullptr for the moment since this is just meant to collect names
// Types are set later on

Result analyzer::collectTypes() {
    // first pass -- collect all user-defined types
    Result result = Result::Success;
    for (const auto& stmt : parsedFile.program) {
        if (_collectTypesInStatement(stmt) == Result::Failure) { result = Result::Failure; }
    }
    return result;
}

void analyzer::_reportRedeclaration(std::string_view redeclaredSymbolName, ast::ASTNode* node) const {
    logging::logError(node->getLine(), node->getColumn(), "'{}' was already declared in this scope",
                      redeclaredSymbolName);
}

Result analyzer::_collectTypesInStatement(ast::Statement* stmt) {
    using enum ast::StatementKind;

    switch (stmt->kind) {
        case AggregateDeclarationStatement: {
            auto aggregateStmt = static_cast<ast::AggregateDeclarationStatement*>(stmt);
            auto result = symbolTable.declare(aggregateStmt->name,
                                              Symbol{
                                                  .type = nullptr,
                                                  .node = aggregateStmt,
                                                  .kind = SymbolKind::Aggregate,
                                                  .visibility = aggregateStmt->visibility,
                                                  .isMutable = false,
                                              });
            if (result == Result::Failure) { _reportRedeclaration(aggregateStmt->name, aggregateStmt); }
            return result;
        }
        case AliasStatement: {
            auto aliasStmt = static_cast<ast::AliasStatement*>(stmt);
            auto result = symbolTable.declare(aliasStmt->alias,
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
            auto enumDecl = static_cast<ast::EnumDeclarationStatement*>(stmt);
            auto result = symbolTable.declare(enumDecl->name,
                                              Symbol{
                                                  .type = nullptr,
                                                  .node = enumDecl,
                                                  .kind = SymbolKind::Enum,
                                                  .visibility = enumDecl->visibility,
                                                  .isMutable = false,
                                              });
            if (result == Result::Failure) { _reportRedeclaration(enumDecl->name, enumDecl); }
            return result;
        }
        case FunctionDeclarationStatement: {
            auto funcStmt = static_cast<ast::FunctionDeclarationStatement*>(stmt);

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
            auto bodyResult = _collectTypesInStatementBody(funcStmt->body);
            return (result == Result::Success && bodyResult == Result::Success) ? Result::Success : Result::Failure;
        }
        case IfStatement: {
            auto ifStmt = static_cast<ast::IfStatement*>(stmt);
            Result result = Result::Success;
            if (_collectTypesInStatementBody(ifStmt->body) == Result::Failure) { result = Result::Failure; }
            for (const auto& elif : ifStmt->elifs) {
                if (_collectTypesInStatementBody(elif.body) == Result::Failure) { result = Result::Failure; }
            }
            if (!ifStmt->elseBody.empty() && _collectTypesInStatementBody(ifStmt->elseBody) == Result::Failure) {
                result = Result::Failure;
            }

            return result;
        }
        case WhileLoopStatement: {
            auto whileStmt = static_cast<ast::WhileLoopStatement*>(stmt);
            return _collectTypesInStatementBody(whileStmt->body);
        }
        default: return Result::Success;  // Statements that don't introduce scopes or declare types
    }
}

Result analyzer::_collectTypesInStatementBody(const ast::Block& body) {
    symbolTable.enterScope();
    Result result = Result::Success;

    for (const auto& subStatement : body) {
        if (_collectTypesInStatement(subStatement) == Result::Failure) { result = Result::Failure; }
    }

    symbolTable.exitScope();  // Guaranteed to run without being skipped by case returns
    return result;
}

}  // namespace semantic

}  // namespace Manganese