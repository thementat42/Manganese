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
    if (!context.whileLoopDepth && !context.forLoopDepth && !context.switchStatementDepth) {
        logError(statement, "'break' can only be used in loops or switch statements");
        return Result::Failure;
    }
    return Result::Success;
}

auto analyzer::visit(ast::ContinueStatement* statement) -> stmtvisit_t {
    if (!context.whileLoopDepth && !context.forLoopDepth) {
        logError(statement, "'continue' can only be used in loops");
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
auto analyzer::visit(ast::IfStatement* statement) -> stmtvisit_t {
    visit(statement->condition);
    visit(statement->body);
    for (auto& elif : statement->elifs) {
        visit(elif.condition);
        visit(elif.body);
    }
    if (statement->elseBody.size() != 0) {
        // there is an else body
        visit(statement->elseBody);
    }
    return Result::Success;
}

auto analyzer::visit(ast::ReturnStatement* statement) -> stmtvisit_t {
    if (!context.inFunction) {
        logError(statement, "'return' can only be used in a function");
        return Result::Failure;
    }

    // void return
    if (!statement->value) {
        if (context.currentFunctionReturnType != nullptr) {
            logError(statement, "Non-void function must return a value");
            return Result::Failure;
        }
        return Result::Success;
    }

    if (visit(statement->value) == Result::Failure) { return Result::Failure; }
    if (!statement->value->semanticType) { logError(statement->value, "Could not deduce type of return expression"); }

    if (!areTypesCompatible(statement->value->semanticType, context.currentFunctionReturnType)) {
        logError(statement, "Function returns '{}' but expression in return statement has type '{}'",
                 (context.currentFunctionReturnType ? context.currentFunctionReturnType->toString() : "void"),
                 statement->value->semanticType->toString());
        return Result::Failure;
    }
    return Result::Success;
}

// auto analyzer::visit(ast::SwitchStatement* statement) -> stmtvisit_t;
// auto analyzer::visit(ast::VariableDeclarationStatement* statement) -> stmtvisit_t;

auto analyzer::visit(ast::WhileLoopStatement* statement) -> stmtvisit_t {
    auto conditionResult = visit(statement->condition);
    if (!statement->condition->semanticType) {
        logError(statement, "Could not deduce type of expression {}", statement->condition->toString());
        return Result::Failure;
    }

    if (statement->condition->semanticType->primitiveType != ast::PrimitiveType_t::boolean) {
        logError(statement, "While loop condition must be a boolean value (found {})",
                 statement->condition->semanticType->toString());
        return Result::Failure;
    }

    ++context.whileLoopDepth;
    auto bodyResult = visit(statement->body);
    --context.whileLoopDepth;
    return (conditionResult == Result::Success && bodyResult == Result::Success) ? Result::Success : Result::Failure;
}

}  // namespace semantic
}  // namespace Manganese
