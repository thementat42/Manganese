#include <frontend/semantic/semantic_analyzer.h>

namespace Manganese {

namespace semantic {

void SemanticAnalyzer::checkAssignmentExpression(ast::AssignmentExpression* expression) {
    checkExpression(expression->assignee.get());
    checkExpression(expression->value.get());
    if (expression->assignee->kind() != ast::ExpressionKind::IdentifierExpression
        && expression->assignee->kind() != ast::ExpressionKind::IndexExpression) {
        logError("Cannot assign to non-variable expression: {}", expression, toStringOr(expression->assignee));
        return;
    }
    if (expression->op != lexer::TokenType::Assignment) {
        if (!handleInPlaceAssignment(expression)) { return; }
    }

    if (expression->assignee->kind() == ast::ExpressionKind::IdentifierExpression) {
        // Checking if the identifier is declared in the current scope was already done in checkIdentifierExpression
        auto identifierExpression = static_cast<ast::IdentifierExpression*>(expression->assignee.get());
        const Symbol* symbol = symbolTable.lookupInCurrentScope(identifierExpression->value);
        if (!symbol) {
            logError("{} was not declared in this scope. Declare it using 'let'", expression,
                     identifierExpression->value);
            return;
        }
        if (symbol->isConstant) {
            logError("{} was declared constant, so it cannot be reassigned. To make {} mutable, declare it using 'let'",
                     expression, identifierExpression->value, identifierExpression->value);
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

void SemanticAnalyzer::checkIdentifierExpression(ast::IdentifierExpression* expression) {
    const Symbol* location = symbolTable.lookupInCurrentScope(expression->value);
    if (location) {
        expression->setType(location->type);
        return;
    }
    if (symbolTable.lookup(expression->value)) {
        // If the symbol can't be found in this scope, try parent scopes
        //? Warning or error
        logError(
            "{} was not declared in this scope (but was declared in a parent scope). Access external variables either using an explicit access (a_module::{}) or passing it as an argument to a function",
            expression, expression->value, expression->value);
    } else {
        logError("{} was not declared in any scope.", expression, expression->value);
    }
}

// ===== Helpers =====

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

    checkBinaryExpression(tempBinaryExpression.get());

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