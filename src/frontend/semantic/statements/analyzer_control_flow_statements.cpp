#include <core.hpp>
#include <format>
#include <frontend/ast.hpp>
#include <frontend/semantic.hpp>
#include <io/logging.hpp>
#include <utils/result.hpp>

namespace Manganese::semantic {

auto Analyzer::visit(ast::BreakStatement* statement) -> stmtvisit_t {
    if (!context.whileLoopDepth && !context.forLoopDepth) {
        logError(statement, "'break' can only be used in loops ");
        return stmtvisit_t::Failure;
    }
    return stmtvisit_t::Success;
}

auto Analyzer::visit(ast::ContinueStatement* statement) -> stmtvisit_t {
    if (!context.whileLoopDepth && !context.forLoopDepth) {
        logError(statement, "'continue' can only be used in loops");
        return stmtvisit_t::Failure;
    }
    return stmtvisit_t::Success;
}

auto Analyzer::visit(ast::ForLoopStatement* statement) -> stmtvisit_t {
    auto result = stmtvisit_t::Success;
    const ContextGuard guard{context.forLoopDepth,
                             static_cast<decltype(context.forLoopDepth)>(context.forLoopDepth + 1)};
    bool blockNeedsToEnterScope = true;

    if (statement->initializationStep) {
        // We want the variable to be declared inside the scope of the for loop
        // since we enter a scope here, we need to the body visitor know it's already in the appropriate scope
        blockNeedsToEnterScope = false;
        symbolTable.enterScope();
        if (visit(statement->initializationStep) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }
    }

    if (statement->stopCondition) {
        // there is a stop condition, check it
        context.inForLoopCondition = true;
        if (visit(statement->stopCondition) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }
        context.inForLoopCondition = false;

        if (!statement->stopCondition->semanticType) {
            logError(statement->stopCondition, "Could not deduce type of for loop stop condition {}",
                     statement->stopCondition->toString());
            result = stmtvisit_t::Failure;
        } else {
            const typeCompatibilityResult conditionCanBeBool = areTypesCompatible(
                statement->stopCondition->semanticType, typeContext.getPrimitive(ast::PrimitiveType_t::boolean));

            if (!conditionCanBeBool) {
                logError(statement,
                         "Stop condition in for loop must be a boolean type or implicitly convertible to it, not {}",
                         statement->stopCondition->semanticType->toString());
                result = stmtvisit_t::Failure;
            } else if (conditionCanBeBool.result == Compatible_t::Warning) {
                logWarning(statement, "{}", conditionCanBeBool.message);
            }
        }
    }
    if (statement->postExpression) {
        // there is a post expression, check it
        if (visit(statement->postExpression) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }
    }
    if (visit(statement->body, blockNeedsToEnterScope) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }
    return result;
}

auto Analyzer::visit(ast::IfStatement* statement) -> stmtvisit_t {
    auto result = stmtvisit_t::Success;
    const ContextGuard guard{context.ifStatementDepth,
                             static_cast<decltype(context.ifStatementDepth)>(context.ifStatementDepth + 1)};

    context.inIfCondition = true;
    if (visit(statement->condition) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }
    context.inIfCondition = false;

    if (!statement->condition->semanticType) {
        logError(statement, "Could not deduce type of condition {}", statement->condition->toString());
        result = stmtvisit_t::Failure;
    } else {
        const typeCompatibilityResult conditionCanBeBool = areTypesCompatible(
            statement->condition->semanticType, typeContext.getPrimitive(ast::PrimitiveType_t::boolean));

        if (!conditionCanBeBool) {
            logError(statement,
                     "Condition in if statement must be a boolean type or implicitly convertible to it, not {}",
                     statement->condition->semanticType->toString());
            result = stmtvisit_t::Failure;
        } else if (conditionCanBeBool.result == Compatible_t::Warning) {
            logWarning(statement, "{}", conditionCanBeBool.message);
        }
    }

    if (visit(statement->body) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }

    for (ast::ElifClause& elif : statement->elifs) {
        DISCARD(visit(elif.condition));

        if (!elif.condition->semanticType) {
            logError(statement, "Could not deduce type of condition {}", elif.condition->toString());
            result = stmtvisit_t::Failure;
        } else {
            const typeCompatibilityResult conditionCanBeBool = areTypesCompatible(
                elif.condition->semanticType, typeContext.getPrimitive(ast::PrimitiveType_t::boolean));

            if (!conditionCanBeBool) {
                logError(statement,
                         "Condition in elif statement must be a boolean type or implicitly convertible to it, not {}",
                         elif.condition->semanticType->toString());
                result = stmtvisit_t::Failure;
            } else if (conditionCanBeBool.result == Compatible_t::Warning) {
                logWarning(statement, "{}", conditionCanBeBool.message);
            }
        }

        if (visit(elif.body) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }
    }

    if (statement->elseBody.size() != 0) {
        // there is an else body
        if (visit(statement->elseBody) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }
    }
    return result;
}

auto Analyzer::visit(ast::SwitchStatement* statement) -> stmtvisit_t {
    if (!statement->target) {
        logError(statement, "Switch statement is missing a target expression");
        return stmtvisit_t::Failure;
    }
    if (visit(statement->target) == exprvisit_t::Failure) { return stmtvisit_t::Failure; }
    const SemanticType* targetType = statement->target->semanticType;
    if (!targetType) {
        logError(statement, "Could not determine type of switch target expression");
        return stmtvisit_t::Failure;
    }
    if (targetType && targetType->isVoid()) {
        logError(statement->target, "Switch target expression cannot evaluate to 'void'");
        return stmtvisit_t::Failure;
    }
    stmtvisit_t result = stmtvisit_t::Success;
    for (ast::CaseClause& caseClause : statement->cases) {
        for (ast::Expression* val : caseClause.values) {
            const bool visitSuccess = visit(val) == exprvisit_t::Success;
            if (!visitSuccess) { result = stmtvisit_t::Failure; }
            if (!val->fold(typeContext.getTargetInfo()).has_value()) {
                logError(val, "case value must be a constant expression");
                result = stmtvisit_t::Failure;
            }
            if (visitSuccess) {
                const SemanticType* valType = val->semanticType;
                if (valType && !areTypesComparable(targetType, valType)) {
                    logError(val, "Type mismatch in switch case: target has type '{}', but case value has type '{}'",
                             targetType->toString(), valType->toString());
                    result = stmtvisit_t::Failure;
                }
            }
        }
        if (visit(caseClause.body) == Result::Failure) { result = stmtvisit_t::Failure; }
    }
    if (!statement->defaultBody.empty() && visit(statement->defaultBody) == Result::Failure) {
        result = stmtvisit_t::Failure;
    }
    return result;
}

auto Analyzer::visit(ast::WhileLoopStatement* statement) -> stmtvisit_t {
    const ContextGuard guard{context.whileLoopDepth,
                             static_cast<decltype(context.whileLoopDepth)>(context.whileLoopDepth + 1)};

    auto result = stmtvisit_t::Success;

    context.inWhileLoopCondition = true;
    if (visit(statement->condition) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }
    context.inWhileLoopCondition = false;

    if (!statement->condition->semanticType) {
        logError(statement, "Could not deduce type of expression {}", statement->condition->toString());
        return stmtvisit_t::Failure;
    } else {
        const typeCompatibilityResult conditionCanBeBool = areTypesCompatible(
            statement->condition->semanticType, typeContext.getPrimitive(ast::PrimitiveType_t::boolean));

        if (!conditionCanBeBool) {
            logError(statement, "While loop condition must be a boolean value or implicitly convertible to it, not {}",
                     statement->condition->semanticType->toString());
        } else if (conditionCanBeBool.result == Compatible_t::Warning) {
            logWarning(statement, "{}", conditionCanBeBool.message);
        }
    }

    if (visit(statement->body) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }

    return result;
}

}  // namespace Manganese::semantic