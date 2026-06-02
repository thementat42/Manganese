#include <core.hpp>
#include <frontend/ast.hpp>
#include <frontend/semantic.hpp>
#include <utils/result.hpp>

namespace Manganese {
namespace semantic {

// auto analyzer::visit(ast::AggregateDeclarationStatement* statement) -> stmtvisit_t;
// auto analyzer::visit(ast::AliasStatement* statement) -> stmtvisit_t;

auto analyzer::visit(ast::BreakStatement*) -> stmtvisit_t {
    return Result::Success;  // nothing to check
}
auto analyzer::visit(ast::ContinueStatement*) -> stmtvisit_t {
    return Result::Success;  // nothing to check
}
auto analyzer::visit(ast::EmptyStatement*) -> stmtvisit_t {
    return Result::Success;  // nothing to check
}

// auto analyzer::visit(ast::EnumDeclarationStatement* statement) -> stmtvisit_t;
auto analyzer::visit(ast::ExpressionStatement* statement) -> stmtvisit_t {
    const exprvisit_t expressionVisitResult = visit(statement->expression);
    return expressionVisitResult == Result::Failure ? Result::Failure : Result::Success;
}

// auto analyzer::visit(ast::FunctionDeclarationStatement* statement) -> stmtvisit_t;
// auto analyzer::visit(ast::IfStatement* statement) -> stmtvisit_t;
// auto analyzer::visit(ast::RepeatLoopStatement* statement) -> stmtvisit_t;
// auto analyzer::visit(ast::ReturnStatement* statement) -> stmtvisit_t;
// auto analyzer::visit(ast::SwitchStatement* statement) -> stmtvisit_t;
// auto analyzer::visit(ast::VariableDeclarationStatement* statement) -> stmtvisit_t;
// auto analyzer::visit(ast::WhileLoopStatement* statement) -> stmtvisit_t;

}  // namespace semantic
}  // namespace Manganese
