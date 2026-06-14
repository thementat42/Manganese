#include <core.hpp>
#include <frontend/ast.hpp>
#include <frontend/semantic.hpp>
#include <io/logging.hpp>
#include <utils/result.hpp>


namespace Manganese {
namespace semantic {

// auto analyzer::visit(ast::AggregateDeclarationStatement* statement) -> stmtvisit_t;
// auto analyzer::visit(ast::AliasStatement* statement) -> stmtvisit_t;

auto analyzer::visit(ast::BreakStatement* statement) -> stmtvisit_t {
    if (!contextFlags.inWhileLoop && !contextFlags.inForLoop) {
        logging::logError(statement->getLine(), statement->getColumn(),
                          "'break' can only be used in loops or switch statements");
        return Result::Failure;
    }
    return Result::Success;
}
auto analyzer::visit(ast::ContinueStatement* statement) -> stmtvisit_t {
    if (!contextFlags.inWhileLoop && !contextFlags.inForLoop) {
        logging::logError(statement->getLine(), statement->getColumn(), "'continue' can only be used in loops");
        return Result::Failure;
    }
    return Result::Success;
}
auto analyzer::visit(ast::EmptyStatement*) -> stmtvisit_t {
    return Result::Success;  // nothing to check
}

// auto analyzer::visit(ast::EnumDeclarationStatement* statement) -> stmtvisit_t;
auto analyzer::visit(ast::ExpressionStatement* statement) -> stmtvisit_t { return visit(statement->expression); }
// auto analyzer::visit(ast::ForLoopStatement* statement) -> stmtvisit_t;
// auto analyzer::visit(ast::FunctionDeclarationStatement* statement) -> stmtvisit_t;
// auto analyzer::visit(ast::IfStatement* statement) -> stmtvisit_t;
// auto analyzer::visit(ast::ReturnStatement* statement) -> stmtvisit_t;
// auto analyzer::visit(ast::SwitchStatement* statement) -> stmtvisit_t;
// auto analyzer::visit(ast::VariableDeclarationStatement* statement) -> stmtvisit_t;
// auto analyzer::visit(ast::WhileLoopStatement* statement) -> stmtvisit_t;

}  // namespace semantic
}  // namespace Manganese
