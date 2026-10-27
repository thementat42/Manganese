#include <core.hpp>
#include <format>
#include <frontend/ast.hpp>
#include <frontend/semantic.hpp>
#include <io/logging.hpp>
#include <utils/result.hpp>

#include "frontend/semantic/symbol_table.hpp"

namespace Manganese::semantic {

auto SemanticAnalyzer::visit(ast::BreakStatement* statement) -> stmtvisit_t {
    if ((context.whileLoopDepth == 0U) && (context.forLoopDepth == 0U) && (context.forEachLoopDepth == 0)) {
        logError(statement, "'break' can only be used in loops ");
        return stmtvisit_t::Failure;
    }
    return stmtvisit_t::Success;
}

auto SemanticAnalyzer::visit(ast::ContinueStatement* statement) -> stmtvisit_t {
    if ((context.whileLoopDepth == 0U) && (context.forLoopDepth == 0U) && (context.forEachLoopDepth == 0)) {
        logError(statement, "'continue' can only be used in loops");
        return stmtvisit_t::Failure;
    }
    return stmtvisit_t::Success;
}

auto SemanticAnalyzer::visit(ast::ForEachLoopStatement* statement) -> stmtvisit_t {
    auto result = stmtvisit_t::Success;
    const ContextGuard guard{context.forEachLoopDepth,
                             static_cast<decltype(context.forEachLoopDepth)>(context.forEachLoopDepth + 1)};

    statement->container->semanticType = typeContext.getPoison();
    if (visit(statement->container) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }

    const SemanticType* containerType = statement->container->semanticType;
    const SemanticType* elementType = typeContext.getPoison();

    if (containerType->isPoison()) {
        logError(statement->container, "Could not deduce type of container in foreach statement");
        result = stmtvisit_t::Failure;
    } else {
        if (containerType->isSlice()) {
            elementType = static_cast<const Slice*>(containerType)->elementType;
        } else if (containerType->isArray()) {
            elementType = static_cast<const Array*>(containerType)->elementType;
        } else {
            logError(statement->container, "Type '{}' is not iterable (expected array or slice)",
                     containerType->toString());
            result = stmtvisit_t::Failure;
        }
    }

    const SemanticType* resolvedElementType = elementType;

    if (statement->element.type != nullptr) {
        if (visit(statement->element.type) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }
        resolvedElementType = statement->element.type->semanticType;
        if (elementType != nullptr && !areTypesCompatible(resolvedElementType, elementType)) {
            logError(statement,
                     "Type mismatch in foreach loop: variable has type '{}', but container element type is '{}'",
                     statement->element.type->toString(interner), elementType->toString());
            result = stmtvisit_t::Failure;
        }
    }
    statement->element.semanticType = resolvedElementType;

    symbolTable.enterScope();
    statement->associatedScope = symbolTable.getCurrentScope();

    if (!statement->element.semanticType->isPoison()) {
        Result insertResult = symbolTable.declare(statement->element.name,
                                                  Symbol{.type = resolvedElementType,
                                                         .node = statement,
                                                         .kind = SymbolKind::Variable,
                                                         .isMutable = statement->element.isMutable});
        if (insertResult == Result::Failure) {
            logError(statement, "Variable '{}' is already declared in this scope",
                     interner.get_view(statement->element.name));
            result = stmtvisit_t::Failure;
        }
    }

    if (visit(statement->body, false, statement->associatedScope) == stmtvisit_t::Failure) {
        result = stmtvisit_t::Failure;
    }

    return result;
}

auto SemanticAnalyzer::visit(ast::ForLoopStatement* statement) -> stmtvisit_t {
    auto result = stmtvisit_t::Success;
    const ContextGuard guard{context.forLoopDepth,
                             static_cast<decltype(context.forLoopDepth)>(context.forLoopDepth + 1)};
    const bool blockNeedsToEnterScope = statement->initializationStep == nullptr;

    if (statement->initializationStep != nullptr) {
        symbolTable.enterScope();
        statement->associatedScope = symbolTable.getCurrentScope();
        if (visit(statement->initializationStep) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }
    }

    if (statement->stopCondition != nullptr) {
        context.inForLoopCondition = true;
        if (visit(statement->stopCondition) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }
        context.inForLoopCondition = false;

        if (statement->stopCondition->semanticType->isPoison()) {
            logError(statement->stopCondition, "Could not deduce type of for loop stop condition {}",
                     statement->stopCondition->toString(interner));
            result = stmtvisit_t::Failure;
        } else {
            const typeCompatibilityResult conditionCanBeBool = areTypesCompatible(
                statement->stopCondition->semanticType, typeContext.getPrimitive(ast::PrimitiveType::boolean));

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
    if (statement->postExpression != nullptr) {
        if (visit(statement->postExpression) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }
    }
    if (visit(statement->body, blockNeedsToEnterScope, statement->associatedScope) == stmtvisit_t::Failure) {
        result = stmtvisit_t::Failure;
    }
    return result;
}

auto SemanticAnalyzer::visit(ast::IfStatement* statement) -> stmtvisit_t {
    auto result = stmtvisit_t::Success;
    const ContextGuard guard{context.ifStatementDepth,
                             static_cast<decltype(context.ifStatementDepth)>(context.ifStatementDepth + 1)};

    const bool hasVar = (statement->variableDeclaration != nullptr);
    if (hasVar) {
        symbolTable.enterScope(statement->associatedScope);
        if (visit(statement->variableDeclaration) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }
    }

    context.inIfCondition = true;
    if (visit(statement->condition) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }
    context.inIfCondition = false;

    if (statement->condition->semanticType->isPoison()) {
        logError(statement, "Could not deduce type of condition {}", statement->condition->toString(interner));
        result = stmtvisit_t::Failure;
    } else {
        const typeCompatibilityResult conditionCanBeBool = areTypesCompatible(
            statement->condition->semanticType, typeContext.getPrimitive(ast::PrimitiveType::boolean));

        if (!conditionCanBeBool) {
            logError(statement,
                     "Condition in if statement must be a boolean type or implicitly convertible to it, not {}",
                     statement->condition->semanticType->toString());
            result = stmtvisit_t::Failure;
        } else if (conditionCanBeBool.result == Compatible_t::Warning) {
            logWarning(statement, "{}", conditionCanBeBool.message);
        }
    }

    if (visit(statement->body, !hasVar, statement->associatedScope) == stmtvisit_t::Failure) {
        result = stmtvisit_t::Failure;
    }

    for (const ast::ElifClause& elif : statement->elifs) {
        const bool elifHasVar = (elif.variableDeclaration != nullptr);
        if (elifHasVar) {
            symbolTable.enterScope(elif.associatedScope);
            if (visit(elif.variableDeclaration) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }
        }

        DISCARD(visit(elif.condition));

        if (elif.condition->semanticType->isPoison()) {
            logError(statement, "Could not deduce type of condition {}", elif.condition->toString(interner));
            result = stmtvisit_t::Failure;
        } else {
            const typeCompatibilityResult conditionCanBeBool = areTypesCompatible(
                elif.condition->semanticType, typeContext.getPrimitive(ast::PrimitiveType::boolean));

            if (!conditionCanBeBool) {
                logError(statement,
                         "Condition in elif statement must be a boolean type or implicitly convertible to it, not {}",
                         elif.condition->semanticType->toString());
                result = stmtvisit_t::Failure;
            } else if (conditionCanBeBool.result == Compatible_t::Warning) {
                logWarning(statement, "{}", conditionCanBeBool.message);
            }
        }

        if (visit(elif.body, !elifHasVar, elif.associatedScope) == stmtvisit_t::Failure) {
            result = stmtvisit_t::Failure;
        }
    }

    if (statement->elseBody.has_value()) {
        if (visit(*statement->elseBody, true, statement->elseAssociatedScope) == stmtvisit_t::Failure) {
            result = stmtvisit_t::Failure;
        }
    }
    return result;
}

auto SemanticAnalyzer::visit(ast::SwitchStatement* statement) -> stmtvisit_t {
    if (statement->target == nullptr) {
        logError(statement, "Switch statement is missing a target expression");
        return stmtvisit_t::Failure;
    }
    if (visit(statement->target) == exprvisit_t::Failure) { return stmtvisit_t::Failure; }
    const SemanticType* targetType = statement->target->semanticType;
    if (targetType->isPoison()) {
        logError(statement, "Could not determine type of switch target expression");
        return stmtvisit_t::Failure;
    }
    if (targetType->isVoid()) {
        logError(statement->target, "Switch target expression cannot evaluate to 'void'");
        return stmtvisit_t::Failure;
    }
    stmtvisit_t result = stmtvisit_t::Success;
    bool first = true;
    for (const ast::CaseClause& caseClause : statement->cases) {
        for (ast::Expression* val : caseClause.values) {
            const bool visitSuccess = visit(val) == exprvisit_t::Success;
            if (!visitSuccess) { result = stmtvisit_t::Failure; }
            if (!val->canFold()) {
                logError(val, "case value must be a constant expression");
                result = stmtvisit_t::Failure;
            }
            if (visitSuccess) {
                const SemanticType* valType = val->semanticType;
                if (!areTypesComparable(targetType, valType)) {
                    logError(val, "Type mismatch in switch case: target has type '{}', but case value has type '{}'",
                             targetType->toString(), valType->toString());
                    result = stmtvisit_t::Failure;
                }
            }
        }
        if (visit(caseClause.body, true, first ? statement->associatedScope : caseClause.associatedScope)
            == Result::Failure) {
            result = stmtvisit_t::Failure;
        }
        first = false;
    }
    if (statement->defaultBody.has_value()
        && visit(*statement->defaultBody, true, first ? statement->associatedScope : statement->defaultAssociatedScope)
            == stmtvisit_t::Failure) {
        result = stmtvisit_t::Failure;
    }
    return result;
}

auto SemanticAnalyzer::visit(ast::WhileLoopStatement* statement) -> stmtvisit_t {
    const ContextGuard guard{context.whileLoopDepth,
                             static_cast<decltype(context.whileLoopDepth)>(context.whileLoopDepth + 1)};

    auto result = stmtvisit_t::Success;

    context.inWhileLoopCondition = true;
    if (visit(statement->condition) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }
    context.inWhileLoopCondition = false;

    if (statement->condition->semanticType->isPoison()) {
        logError(statement, "Could not deduce type of expression {}", statement->condition->toString(interner));
        return stmtvisit_t::Failure;
    }
    const typeCompatibilityResult conditionCanBeBool
        = areTypesCompatible(statement->condition->semanticType, typeContext.getPrimitive(ast::PrimitiveType::boolean));

    if (!conditionCanBeBool) {
        logError(statement, "While loop condition must be a boolean value or implicitly convertible to it, not {}",
                 statement->condition->semanticType->toString());
    } else if (conditionCanBeBool.result == Compatible_t::Warning) {
        logWarning(statement, "{}", conditionCanBeBool.message);
    }

    if (visit(statement->body, true, statement->associatedScope) == stmtvisit_t::Failure) {
        result = stmtvisit_t::Failure;
    }

    return result;
}

}  // namespace Manganese::semantic