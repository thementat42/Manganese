/**
 * @file semantic_analyzer_expressions.cpp
 * @brief Responsible for performing semantic analysis on all the AST expression nodes
 */

#include <format>
#include <frontend/ast.hpp>
#include <frontend/semantic/semantic_analyzer.hpp>
#include <global_macros.hpp>


namespace Manganese {
using ast::toStringOr;

namespace semantic {
// void SemanticAnalyzer::visit(ast::Expression* expression) noexcept_if_release {
//     switch (expression->kind()) {
//         case ast::ExpressionKind::AggregateInstantiationExpression:
//             checkAggregateInstantiationExpression(static_cast<ast::AggregateInstantiationExpression*>(expression));
//             break;
//         case ast::ExpressionKind::ArrayLiteralExpression:
//             visit(static_cast<ast::ArrayLiteralExpression*>(expression));
//             break;
//         case ast::ExpressionKind::AssignmentExpression:
//             visit(static_cast<ast::AssignmentExpression*>(expression));
//             break;
//         case ast::ExpressionKind::BinaryExpression:
//             visit(static_cast<ast::BinaryExpression*>(expression));
//             break;
//         case ast::ExpressionKind::BoolLiteralExpression:
//             visit(static_cast<ast::BoolLiteralExpression*>(expression));
//             break;
//         case ast::ExpressionKind::CharLiteralExpression:
//             visit(static_cast<ast::CharLiteralExpression*>(expression));
//             break;
//         case ast::ExpressionKind::FunctionCallExpression:
//             visit(static_cast<ast::FunctionCallExpression*>(expression));
//             break;
//         case ast::ExpressionKind::GenericExpression:
//             visit(static_cast<ast::GenericExpression*>(expression));
//             break;
//         case ast::ExpressionKind::IdentifierExpression:
//             visit(static_cast<ast::IdentifierExpression*>(expression));
//             break;
//         case ast::ExpressionKind::IndexExpression:
//             visit(static_cast<ast::IndexExpression*>(expression));
//             break;
//         case ast::ExpressionKind::MemberAccessExpression:
//             visit(static_cast<ast::MemberAccessExpression*>(expression));
//             break;
//         case ast::ExpressionKind::NumberLiteralExpression:
//             visit(static_cast<ast::NumberLiteralExpression*>(expression));
//             break;
//         case ast::ExpressionKind::PostfixExpression:
//             visit(static_cast<ast::PostfixExpression*>(expression));
//             break;
//         case ast::ExpressionKind::PrefixExpression:
//             visit(static_cast<ast::PrefixExpression*>(expression));
//             break;
//         case ast::ExpressionKind::ScopeResolutionExpression:
//             visit(static_cast<ast::ScopeResolutionExpression*>(expression));
//             break;
//         case ast::ExpressionKind::StringLiteralExpression:
//             visit(static_cast<ast::StringLiteralExpression*>(expression));
//             break;
//         case ast::ExpressionKind::TypeCastExpression:
//             visit(static_cast<ast::TypeCastExpression*>(expression));
//             break;
//         default:
//             using std::format;
//             ASSERT_UNREACHABLE(
//                 format("No semantic analysis method for expression type {}", static_cast<int>(expression->kind())));
//     }
// }

// ===== Specific Expression Checks =====

void SemanticAnalyzer::visit(ast::AggregateInstantiationExpression* expression) {
    const Symbol* aggregateSymbol = symbolTable.lookup(expression->name);
    if (!aggregateSymbol) {
        logError("Aggregate type {} was not declared in any scope", expression, expression->name);
        return;
    }
    if (aggregateSymbol->kind != SymbolKind::Aggregate) {
        logError("{} is not an aggregate type so cannot be instantiated as one", expression, expression->name);
        return;
    }
    ast::AggregateDeclarationStatement* aggregateDeclaration
        = static_cast<ast::AggregateDeclarationStatement*>(aggregateSymbol->declarationNode);
    if (aggregateDeclaration->genericTypes.size() != expression->genericTypes.size()) {
        logError("Aggregate type {} expects {} generic types, but {} were provided", expression, expression->name,
                 aggregateDeclaration->genericTypes.size(), expression->genericTypes.size());
        return;
    }
    if (aggregateDeclaration->fields.size() != expression->fields.size()) {
        logError("Aggregate type {} expects {} fields, but {} were provided", expression, expression->name,
                 aggregateDeclaration->fields.size(), expression->fields.size());
        return;
    }
    bool validInstantiation = true;
    for (size_t i = 0; i < expression->fields.size(); ++i) {
        const auto& field = expression->fields[i];
        const auto& expectedField = aggregateDeclaration->fields[i];
        if (field.name != expectedField.name) {
            logError(
                "Field {} in aggregate instantiation does not match field name {} in aggregate type {} (Note: aggregate fields should be instantiated in order)",
                expression, field.name, expectedField.name, expression->name);
            validInstantiation = false;
            continue;
        }
        visit(field.value.get());
        if (!areTypesCompatible(field.value->getType(), expectedField.type.get())) {
            logError("Field {} in aggregate instantiation has type {}, but was declared with type {}", expression,
                     toStringOr(field.value), toStringOr(field.value->getType()), toStringOr(expectedField.type));
            validInstantiation = false;
        }
    }
    if (!validInstantiation) { return; }
    expression->setType(std::make_shared<ast::SymbolType>(expression->name));
}

void SemanticAnalyzer::visit(ast::FunctionCallExpression* expression) {
    if (expression->callee->kind() != ast::ExpressionKind::IdentifierExpression) {
        // TODO: Support function calls indexing into arrays or module members
        // TODO: Allow calling generic expressions (provided the underlying type is an identifier that was declared
        // as a function)
        logError("Function call must be made to an identifier, not {} (of type {})", expression,
                 toStringOr(expression->callee), toStringOr(expression->getType()));
        return;
    }
    auto identifierExpression = static_cast<ast::IdentifierExpression*>(expression->callee.get());
    const Symbol* functionSymbol = symbolTable.lookup(identifierExpression->value);
    if (!functionSymbol) {
        logError("Function '{}' was not declared in any scope", expression, identifierExpression->value);
        return;
    }

    if (!functionSymbol->type || functionSymbol->type->kind() != ast::TypeKind::FunctionType) {
        logError("{} is not callable (i.e., it is not a function type)", expression, identifierExpression->value);
        return;
    }

    auto* functionType = static_cast<const ast::FunctionType*>(functionSymbol->type.get());
    if (functionType->parameterTypes.size() != expression->arguments.size()) {
        logError("Function {} expects {} arguments, but {} were provided", expression, identifierExpression->value,
                 functionType->parameterTypes.size(), expression->arguments.size());
        return;
    }
    for (size_t i = 0; i < expression->arguments.size(); ++i) {
        ast::Expression* argument = expression->arguments[i].get();
        ast::Type* expectedType = functionType->parameterTypes[i].type.get();
        visit(argument);
        if (!argument->getType()) {
            logError("Could not deduce the type of argument {} in function call, assuming 'int32'", expression, i + 1);
            argument->setType(std::make_shared<ast::SymbolType>("int32"));
        }
        if (!areTypesCompatible(argument->getType(), expectedType)) {
            logError("Argument {} in call to {} has type {}, but expected type is {}", expression, i + 1,
                     identifierExpression->value, toStringOr(argument->getType()), toStringOr(expectedType));
            return;
        }
    }
    // Set the type of the call expression to the function's return type
    expression->setType(functionType->returnType);
}

void SemanticAnalyzer::visit(ast::GenericExpression* expression) {
    DISCARD(expression);
    NOT_IMPLEMENTED("Full generic support is deferred");
}

}  // namespace semantic
}  // namespace Manganese