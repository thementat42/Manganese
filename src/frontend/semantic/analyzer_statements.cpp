#include <core.hpp>
#include <cstdint>
#include <frontend/ast.hpp>
#include <frontend/semantic.hpp>
#include <io/logging.hpp>
#include <utils/result.hpp>

#include "frontend/ast/ast_base.hpp"

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
    auto result = Result::Success;
    ContextGuard guard(context.ifStatementDepth, static_cast<uint8_t>(context.ifStatementDepth + 1));

    if (visit(statement->condition) == Result::Failure) { result = Result::Failure; }

    if (!statement->condition->semanticType) {
        logError(statement, "Could not deduce type of condition {}", statement->condition->toString());
        result = Result::Failure;
    } else {
        const auto conditionCanBeBool = areTypesCompatible(statement->condition->semanticType,
                                                           typeContext.getPrimitive(ast::PrimitiveType_t::boolean));
        if (!conditionCanBeBool) {
            logError(statement,
                     "Condition in if statement must be a boolean type or implicitly convertible to it, not {}",
                     statement->condition->semanticType->toString());
            result = Result::Failure;
        } else if (conditionCanBeBool.result == Compatible_t::Warning) {
            logWarning(statement, "{}", conditionCanBeBool.message);
        }
    }

    if (visit(statement->body) == Result::Failure) { result = Result::Failure; }

    for (auto& elif : statement->elifs) {
        visit(elif.condition);

        if (!elif.condition->semanticType) {
            logError(statement, "Could not deduce type of condition {}", elif.condition->toString());
            result = Result::Failure;
        } else {
            const auto conditionCanBeBool = areTypesCompatible(elif.condition->semanticType,
                                                               typeContext.getPrimitive(ast::PrimitiveType_t::boolean));
            if (!conditionCanBeBool) {
                logError(statement,
                         "Condition in elif statement must be a boolean type or implicitly convertible to it, not {}",
                         elif.condition->semanticType->toString());
                result = Result::Failure;
            } else if (conditionCanBeBool.result == Compatible_t::Warning) {
                logWarning(statement, "{}", conditionCanBeBool.message);
            }
        }

        if (visit(elif.body) == Result::Failure) { result = Result::Failure; }
    }

    if (statement->elseBody.size() != 0) {
        // there is an else body
        if (visit(statement->elseBody) == Result::Failure) { result = Result::Failure; }
    }
    return result;
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
    ContextGuard guard(context.whileLoopDepth, static_cast<uint8_t>(context.whileLoopDepth + 1));
    auto result = Result::Success;

    if (visit(statement->condition) == Result::Failure) { result = Result::Failure; }
    if (!statement->condition->semanticType) {
        logError(statement, "Could not deduce type of expression {}", statement->condition->toString());
        return Result::Failure;
    } else {
        const auto conditionCanBeBool = areTypesCompatible(statement->condition->semanticType,
                                                           typeContext.getPrimitive(ast::PrimitiveType_t::boolean));

        if (!conditionCanBeBool) {
            logError(statement, "While loop condition must be a boolean value or implicitly convertible to it, not {}",
                     statement->condition->semanticType->toString());
        } else if (conditionCanBeBool.result == Compatible_t::Warning) {
            logWarning(statement, "{}", conditionCanBeBool.message);
        }
    }

    if (visit(statement->body) == Result::Failure) { result = Result::Failure; }

    return result;
}

}  // namespace semantic
}  // namespace Manganese
