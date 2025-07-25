/**
 * @file semantic_analyzer_expressions.cpp
 * @brief Responsible for performing semantic analysis on all the AST expression nodes
 */

#include <frontend/semantic/semantic_analyzer.h>
#include <global_macros.h>

#include <format>
#include <stdexcept>
#include <type_traits>
#include <variant>

namespace Manganese {
namespace semantic {

void SemanticAnalyzer::checkExpression(ast::Expression* expression) noexcept_if_release {
    switch (expression->kind()) {
        case ast::ExpressionKind::ArrayLiteralExpression:
            checkArrayLiteralExpression(static_cast<ast::ArrayLiteralExpression*>(expression));
            break;
        case ast::ExpressionKind::AssignmentExpression:
            checkAssignmentExpression(static_cast<ast::AssignmentExpression*>(expression));
            break;
        case ast::ExpressionKind::BinaryExpression:
            checkBinaryExpression(static_cast<ast::BinaryExpression*>(expression));
            break;
        case ast::ExpressionKind::BoolLiteralExpression:
            checkBoolLiteralExpression(static_cast<ast::BoolLiteralExpression*>(expression));
            break;
        case ast::ExpressionKind::BundleInstantiationExpression:
            checkBundleInstantiationExpression(static_cast<ast::BundleInstantiationExpression*>(expression));
            break;
        case ast::ExpressionKind::CharLiteralExpression:
            checkCharLiteralExpression(static_cast<ast::CharLiteralExpression*>(expression));
            break;
        case ast::ExpressionKind::FunctionCallExpression:
            checkFunctionCallExpression(static_cast<ast::FunctionCallExpression*>(expression));
            break;
        case ast::ExpressionKind::GenericExpression:
            checkGenericExpression(static_cast<ast::GenericExpression*>(expression));
            break;
        case ast::ExpressionKind::IdentifierExpression:
            checkIdentifierExpression(static_cast<ast::IdentifierExpression*>(expression));
            break;
        case ast::ExpressionKind::IndexExpression:
            checkIndexExpression(static_cast<ast::IndexExpression*>(expression));
            break;
        case ast::ExpressionKind::MemberAccessExpression:
            checkMemberAccessExpression(static_cast<ast::MemberAccessExpression*>(expression));
            break;
        case ast::ExpressionKind::NumberLiteralExpression:
            checkNumberLiteralExpression(static_cast<ast::NumberLiteralExpression*>(expression));
            break;
        case ast::ExpressionKind::PostfixExpression:
            checkPostfixExpression(static_cast<ast::PostfixExpression*>(expression));
            break;
        case ast::ExpressionKind::PrefixExpression:
            checkPrefixExpression(static_cast<ast::PrefixExpression*>(expression));
            break;
        case ast::ExpressionKind::ScopeResolutionExpression:
            checkScopeResolutionExpression(static_cast<ast::ScopeResolutionExpression*>(expression));
            break;
        case ast::ExpressionKind::StringLiteralExpression:
            checkStringLiteralExpression(static_cast<ast::StringLiteralExpression*>(expression));
            break;
        case ast::ExpressionKind::TypeCastExpression:
            checkTypeCastExpression(static_cast<ast::TypeCastExpression*>(expression));
            break;
        default:
            using std::format;
            ASSERT_UNREACHABLE(
                format("No semantic analysis method for expression type {}",
                       static_cast<int>(expression->kind())));
    }
}

// ===== Specific Expression Checks =====

void SemanticAnalyzer::checkBundleInstantiationExpression(ast::BundleInstantiationExpression* expression) {
    Symbol* bundleSymbol = symbolTable.lookup(expression->name);
    if (!bundleSymbol) {
        logError("Bundle type {} was not declared in any scope", expression, expression->name);
        return;
    }
    if (bundleSymbol->kind != SymbolKind::Bundle) {
        logError("{} is not a bundle type so cannot be instantiated as one", expression, expression->name);
        return;
    }
    ast::BundleDeclarationStatement* bundleDeclaration = static_cast<ast::BundleDeclarationStatement*>(bundleSymbol->declarationNode);
    if (bundleDeclaration->genericTypes.size() != expression->genericTypes.size()) {
        logError("Bundle type {} expects {} generic types, but {} were provided", expression, expression->name,
                 bundleDeclaration->genericTypes.size(), expression->genericTypes.size());
        return;
    }
    if (bundleDeclaration->fields.size() != expression->fields.size()) {
        logError("Bundle type {} expects {} fields, but {} were provided", expression, expression->name,
                 bundleDeclaration->fields.size(), expression->fields.size());
        return;
    }
    bool validInstantiation = true;
    for (size_t i = 0; i < expression->fields.size(); ++i) {
        const auto& field = expression->fields[i];
        const auto& expectedField = bundleDeclaration->fields[i];
        if (field.name != expectedField.name) {
            logError("Field {} in bundle instantiation does not match field name {} in bundle type {} (Note: bundle fields should be instantiated in order)", expression, field.name, expectedField.name, expression->name);
            validInstantiation = false;
            continue;
        }
        checkExpression(field.value.get());
        if (!areTypesCompatible(field.value->getType(), expectedField.type.get())) {
            logError("Field {} in bundle instantiation has type {}, but was declared with type {}", expression, field.value->toString(), field.value->getType()->toString(), expectedField.type->toString());
            validInstantiation = false;
        }
    }
    if (!validInstantiation) {
        return;
    }
    expression->setType(std::make_shared<ast::SymbolType>(expression->name));
}

void SemanticAnalyzer::checkFunctionCallExpression(ast::FunctionCallExpression* expression) {
    if (expression->callee->kind() != ast::ExpressionKind::IdentifierExpression) {
        // TODO: Support function calls indexing into arrays or module members
        // TODO: Allow calling generic expressions (provided the underlying type is an identifier that was declared as a function)
        logError("Function call must be made to an identifier, not {}", expression, expression->callee->toString());
        return;
    }
    auto identifierExpression = static_cast<ast::IdentifierExpression*>(expression->callee.get());
    Symbol* functionSymbol = symbolTable.lookup(identifierExpression->value);
    if (!functionSymbol) {
        logError("Function '{}' was not declared in any scope", expression, identifierExpression->value);
        return;
    }
    if (functionSymbol->kind != SymbolKind::Function) {
        logError("{} is not a function, so cannot be called", expression, identifierExpression->value);
        return;
    }
    ast::FunctionDeclarationStatement* functionDeclaration = static_cast<ast::FunctionDeclarationStatement*>(functionSymbol->declarationNode);
    if (functionDeclaration->parameters.size() != expression->arguments.size()) {
        logError("Function {} expects {} arguments, but {} were provided", expression, identifierExpression->value,
                 functionDeclaration->parameters.size(), expression->arguments.size());
        return;
    }
    for (size_t i = 0; i < expression->arguments.size(); ++i) {
        ast::Expression* argument = expression->arguments[i].get();
        ast::Type* expectedType = functionDeclaration->parameters[i].type.get();
        checkExpression(argument);
        if (!argument->getType()) {
            logError("Could not deduce the type of argument {} in function call, assuming 'int32'", expression, i + 1);
            argument->setType(std::make_shared<ast::SymbolType>("int32"));
        }
        if (!areTypesCompatible(argument->getType(), expectedType)) {
            logError("Argument {} in function call to {} has type {}, but expected type is {}", expression, i + 1, argument->toString(),
                     argument->getType()->toString(), expectedType->toString());
            return;
        }
    }
}
void SemanticAnalyzer::checkGenericExpression(ast::GenericExpression* expression) {
    DISCARD(expression);
    PRINT_LOCATION;
    throw std::runtime_error("Not implemented");
}

}  // namespace semantic
}  // namespace Manganese