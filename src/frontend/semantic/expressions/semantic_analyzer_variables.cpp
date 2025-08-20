#include <frontend/semantic/semantic_analyzer.hpp>

#include "frontend/ast/ast_expressions.hpp"

namespace Manganese {

namespace semantic {

void SemanticAnalyzer::visit(ast::AssignmentExpression* expression) {
    visit(expression->assignee.get());
    visit(expression->value.get());
    if (expression->assignee->kind() != ast::ExpressionKind::IdentifierExpression
        && expression->assignee->kind() != ast::ExpressionKind::IndexExpression) {
        logError("Cannot assign to non-variable expression: {}", expression, toStringOr(expression->assignee));
        return;
    }
    if (expression->op != lexer::TokenType::Assignment) {
        // e.g. x += y
        if (!handleInPlaceAssignment(expression)) {
            // Some error already in the in place assignment, don't keep checking
            return;
        }
    }

    if (expression->assignee->kind() == ast::ExpressionKind::IdentifierExpression) {
        if (!checkIdentifierAssignmentExpression(expression)) {
            return;
        }
    } else if (expression->assignee->kind() == ast::ExpressionKind::IndexExpression) {
        if (!checkIndexAssignmentExpression(expression)) {
            return;
        }
    }
    if (!areTypesCompatible(expression->assignee->getType(), expression->value->getType())) {
        logError("{} cannot be assigned to {}. {} has type {}, but {} has type {}", expression,
                 toStringOr(expression->value), toStringOr(expression->assignee), toStringOr(expression->value),
                 toStringOr(expression->value->getType()), toStringOr(expression->assignee),
                 toStringOr(expression->assignee->getType()));
        return;
    }
}

void SemanticAnalyzer::visit(ast::IdentifierExpression* expression) {
    const Symbol* symbol = symbolTable.lookup(expression->value);
    if (!symbol) { logError("{} was not declared in any scope.", expression, expression->value); }
    expression->setType(symbol->type);
    return;
}

// ===== Helpers =====

bool SemanticAnalyzer::checkIdentifierAssignmentExpression(ast::AssignmentExpression* expression) {
    // Checking if the identifier is declared in the current scope was already done in checkIdentifierExpression
    auto identifierExpression = static_cast<ast::IdentifierExpression*>(expression->assignee.get());
    const Symbol* symbol = symbolTable.lookup(identifierExpression->value);
    if (!symbol) {
        logError("{} was not declared in any scope. Declare it using 'let'", expression, identifierExpression->value);
        return false;
    }
    if (!symbol->isMutable) {
        if (symbol->kind == SymbolKind::Constant) {
            logError("{} was declared constant, so it cannot be reassigned. To make {} mutable, declare it using 'let mut'",
                     expression, identifierExpression->value, identifierExpression->value);
        } else if (symbol->kind == SymbolKind::ConstantFunctionParameter) {
            logError(
                "Cannot modify parameter '{}' as it was declared as constant. Mark the parameter as 'mut' to modify it within the function",
                expression, identifierExpression->value);
        }
        return false;
    }
    return true;
}

bool SemanticAnalyzer::checkIndexAssignmentExpression(ast::AssignmentExpression* expression) {
    auto indexExpression = static_cast<ast::IndexExpression*>(expression->assignee.get());

    ast::Expression* currentObject = indexExpression->variable.get();

    // Traverse to the root object through nested index expressions
    while (currentObject->kind() == ast::ExpressionKind::IndexExpression) {
        currentObject = static_cast<ast::IndexExpression*>(currentObject)->variable.get();
    }

    // Now currentObject is the root container (e.g., 'array' in array[i][j])
    if (currentObject->kind() == ast::ExpressionKind::IdentifierExpression) {
        auto containerIdentifier = static_cast<ast::IdentifierExpression*>(currentObject);
        const Symbol* containerSymbol = symbolTable.lookup(containerIdentifier->value);

        if (containerSymbol && !containerSymbol->isMutable) {
            logError("Cannot modify element of constant array '{}'. To make it mutable, declare it using 'let'",
                     expression, containerIdentifier->value);
            return false;
        }
    }
    return true;
}

constexpr lexer::TokenType getBinaryOperatorFromAssignmentOperator(lexer::TokenType assignmentOp) noexcept_if_release {
    using enum lexer::TokenType;
    switch (assignmentOp) {
        case PlusAssign: return Plus;
        case MinusAssign: return Minus;
        case MulAssign: return Mul;
        case DivAssign: return Div;
        case FloorDivAssign: return FloorDiv;
        case ModAssign: return Mod;
        case ExpAssign: return Exp;
        case BitAndAssign: return BitAnd;
        case BitOrAssign: return BitOr;
        case BitXorAssign: return BitXor;
        case BitLShiftAssign: return BitLShift;
        case BitRShiftAssign: return BitRShift;
        default:
            ASSERT_UNREACHABLE(std::format("Cannot convert assignment operator {} to binary operator",
                                           lexer::tokenTypeToString(assignmentOp)));
            return Unknown;
    }
}

bool SemanticAnalyzer::handleInPlaceAssignment(Manganese::ast::AssignmentExpression* expression) {
    auto binaryOperator = getBinaryOperatorFromAssignmentOperator(expression->op);

    // x op= y is basically just x = x op y
    // So for type checking, make a temporary binary expression with x op y
    // and verify that that's valid.
    auto tempBinaryExpression = std::make_unique<ast::BinaryExpression>(
        // We temporarily give the binary expression ownership of the pointers
        // Since the ast nodes only accept unique_ptrs (and take ownership of them)
        std::move(expression->assignee), binaryOperator, std::move(expression->value));

    visit(tempBinaryExpression.get());

    // Regardless of whether or not the expression was valid, we need to retake ownership of the pointers
    // so that the assignment expression remains valid
    expression->assignee = std::move(tempBinaryExpression->left);
    expression->value = std::move(tempBinaryExpression->right);

    if (!tempBinaryExpression->getType()) {
        logError("Could not resolve type of {} in assignment", expression, tempBinaryExpression->toString());
        return false;
    }
    if (!areTypesCompatible(expression->assignee->getType(), tempBinaryExpression->getType())) {
        logError("{} cannot be assigned to {}. {} has type {}, but {} has type {}", expression,
                 toStringOr(expression->value), toStringOr(expression->assignee), toStringOr(expression->value),
                 toStringOr(expression->value->getType()), toStringOr(expression->assignee),
                 toStringOr(expression->assignee->getType()));
        return false;
    }
    return true;
}

}  // namespace semantic

}  // namespace Manganese