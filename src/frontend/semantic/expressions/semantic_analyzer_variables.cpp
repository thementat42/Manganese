#include <frontend/semantic/semantic_analyzer.h>

namespace Manganese {

namespace semantic {
void SemanticAnalyzer::checkAssignmentExpression(ast::AssignmentExpression* expression) {
    checkExpression(expression->assignee.get());
    checkExpression(expression->value.get());
    if (expression->assignee->kind() != ast::ExpressionKind::IdentifierExpression &&
        expression->assignee->kind() != ast::ExpressionKind::IndexExpression) {
        logError("Cannot assign to non-variable expression: {}", expression, expression->assignee->toString());
        return;
    }
    if (expression->assignee->kind() == ast::ExpressionKind::IdentifierExpression) {
        // Checking if the identifier is declared in the current scope was already done in checkIdentifierExpression
        auto identifierExpression = static_cast<ast::IdentifierExpression*>(expression->assignee.get());
        if (symbolTable.lookupInCurrentScope(identifierExpression->value)->isConstant) {
            logError("{} was declared constant, so it cannot be reassigned. To make {} mutable, declare it using 'let'", expression, identifierExpression->value, identifierExpression->value);
            return;
        }
    }
    if (!areTypesCompatible(expression->assignee->getType(), expression->value->getType())) {
        logError("{} cannot be assigned to {}. {} has type {}, but {} has type {}", expression,
                 expression->value->toString(), expression->assignee->toString(),
                 expression->value->toString(), expression->value->getType()->toString(),
                 expression->assignee->toString(), expression->assignee->getType()->toString());
        return;
    }
}

void SemanticAnalyzer::checkIdentifierExpression(ast::IdentifierExpression* expression) {
    Symbol* location = symbolTable.lookupInCurrentScope(expression->value);
    if (location) {
        expression->setType(location->type);
        return;
    }
    if (symbolTable.lookup(expression->value)) {
        // If the symbol can't be found in this scope, try parent scopes
        //? Warning or error
        logError("{} was not declared in this scope (but was declared in a parent scope). Access external variables either using an explicit access (a_module::{}) or passing it as an argument to a function", expression, expression->value, expression->value);
    } else {
        logError("{} was not declared in any scope.", expression, expression->value);
    }
}

}  // namespace semantic

}  // namespace Manganese