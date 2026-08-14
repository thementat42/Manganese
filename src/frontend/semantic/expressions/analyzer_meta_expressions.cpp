#include <core.hpp>
#include <frontend/ast.hpp>
#include <frontend/lexer/token.hpp>
#include <frontend/semantic/analyzer.hpp>
#include <frontend/semantic/symbol_table.hpp>
#include <frontend/semantic/type_context.hpp>
#include <mnstl/number.hxx>
#include <utility>
#include <utils/result.hpp>
#include <vector>

namespace Manganese::semantic {

auto analyzer::visit(ast::AlignofExpression* expression) -> exprvisit_t {
    if (visit(expression->type) == exprvisit_t::Failure) { return exprvisit_t::Failure; }
    const SemanticType* targetSemanticType = expression->type->semanticType;
    if (!targetSemanticType) {
        logError(expression, "Invalid type in alignof expression {}", expression->toString());
        return exprvisit_t::Failure;
    }
    expression->semanticType = typeContext.getUSizeType();
    return exprvisit_t::Success;
}

auto analyzer::visit(ast::GenericExpression* expression) -> exprvisit_t {
    TypeList resolvedTypeArguments;
    resolvedTypeArguments.reserve(expression->types.size());

    for (ast::Type* type : expression->types) {
        visit(type);
        const SemanticType* resolved = type->semanticType;
        if (!resolved) {
            logError(type, "Failed to resolve generic type argument '{}' in generic expression", type->toString());
            return exprvisit_t::Failure;
        }
        resolvedTypeArguments.push_back(resolved);
    }

    ast::Expression* targetDeclaration = unwrapBaseDeclaration(expression->identifier);
    if (!targetDeclaration || targetDeclaration->kind != ast::ExpressionKind::IdentifierExpression) {
        logError(expression->identifier,
                 "Target of generic expression must be a named function or aggregate declaration");
        return exprvisit_t::Failure;
    }
    auto* identifier = static_cast<ast::IdentifierExpression*>(targetDeclaration);
    const Symbol* symbol = symbolTable.lookup(identifier->value);
    if (!symbol || !symbol->node) {
        logError(identifier, "Use of undeclared symbol '{}'", identifier->value);
        return exprvisit_t::Failure;
    }
    StackGuard guard{genericsStack, std::move(resolvedTypeArguments)};

    if (symbol->kind == SymbolKind::Function) {
        auto* functionDeclaration = static_cast<ast::FunctionDeclarationStatement*>(symbol->node);
        if (functionDeclaration->genericTypes.size() != genericsStack.top().size()) {
            logError(expression, "Generic function '{}' expects {} type arguments, but {} were provided",
                     functionDeclaration->name, functionDeclaration->genericTypes.size(), genericsStack.top().size());
            return exprvisit_t::Failure;
        }
        if (visit(functionDeclaration, generic_tag) == stmtvisit_t::Failure) { return exprvisit_t::Failure; }
        const SemanticType* concreteType = getInstantiatedFunctionType(functionDeclaration, genericsStack.top());
        if (!concreteType) {
            logError(expression, "Failed to materialize instantiated function type for '{}'",
                     functionDeclaration->name);
            return exprvisit_t::Failure;
        }
        expression->semanticType = concreteType;
        expression->identifier->semanticType = concreteType;  // Attach to base for parent visitors
        return exprvisit_t::Success;
    } else if (symbol->kind == SymbolKind::Aggregate || symbol->kind == SymbolKind::GenericType) {
        auto* aggregateDecl = static_cast<ast::AggregateDeclarationStatement*>(symbol->node);

        if (aggregateDecl->genericTypes.size() != genericsStack.top().size()) {
            logError(expression, "Generic aggregate '{}' expects {} type arguments, but {} were provided",
                     aggregateDecl->name, aggregateDecl->genericTypes.size(), genericsStack.top().size());
            return exprvisit_t::Failure;
        }

        // Type-check / instantiate the generic aggregate definition
        if (visit(aggregateDecl, generic_tag) == stmtvisit_t::Failure) { return exprvisit_t::Failure; }

        // Retrieve the concrete aggregate type layout from cache/context
        const SemanticType* concreteType = getInstantiatedAggregateType(aggregateDecl, genericsStack.top());
        if (!concreteType) {
            logError(expression, "Failed to materialize instantiated aggregate type for '{}'", aggregateDecl->name);
            return exprvisit_t::Failure;
        }

        expression->semanticType = concreteType;
        expression->identifier->semanticType = concreteType;  // Attach to base for parent visitors
        return exprvisit_t::Success;
    }

    logError(identifier, "Symbol '{}' is neither a generic function nor a generic aggregate", identifier->value);
    return exprvisit_t::Failure;
}

auto analyzer::visit(ast::SizeofExpression* expression) -> exprvisit_t {
    if (visit(expression->type) == exprvisit_t::Failure) { return exprvisit_t::Failure; }
    const SemanticType* targetSemanticType = expression->type->semanticType;
    if (!targetSemanticType) {
        logError(expression, "Invalid type in sizeof expression {}", expression->toString());
        return exprvisit_t::Failure;
    }
    expression->semanticType = typeContext.getUSizeType();
    return exprvisit_t::Success;
}

}  // namespace Manganese::semantic