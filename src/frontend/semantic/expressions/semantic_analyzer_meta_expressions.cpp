#include <core.hpp>
#include <frontend/ast.hpp>
#include <frontend/lexer/token.hpp>
#include <frontend/semantic/semantic_analyzer.hpp>
#include <frontend/semantic/symbol_table.hpp>
#include <frontend/semantic/type_context.hpp>
#include <utils/result.hpp>
#include <vector>

namespace Manganese::semantic {

auto SemanticAnalyzer::visit(ast::AlignofExpression* expression) -> exprvisit_t {
    expression->semanticType = typeContext.getPoison();
    if (visit(expression->type) == exprvisit_t::Failure) { return exprvisit_t::Failure; }
    const SemanticType* targetSemanticType = expression->type->semanticType;
    if (targetSemanticType->isPoison()) {
        logError(expression, "Invalid type in alignof expression {}", expression->toString());
        return exprvisit_t::Failure;
    }
    expression->semanticType = typeContext.getUSizeType();
    return exprvisit_t::Success;
}

auto SemanticAnalyzer::visit(ast::GenericInstantiationExpression* expression) -> exprvisit_t {
    expression->semanticType = typeContext.getPoison();
    TypeList resolvedTypeArguments;
    resolvedTypeArguments.reserve(expression->types.size());

    for (ast::Type* type : expression->types) {
        if (visit(type) == typevisit_t::Failure) { return exprvisit_t::Failure; }
        const SemanticType* resolved = type->semanticType;
        if (resolved->isPoison()) {
            logError(type, "Failed to resolve generic type argument '{}' in generic expression", type->toString());
            return exprvisit_t::Failure;
        }
        resolvedTypeArguments.push_back(resolved);
    }

    expression->semanticTypes = resolvedTypeArguments;  // copy

    const Symbol* symbol = resolveScopeSymbol(expression->identifier);
    if (symbol == nullptr || symbol->node == nullptr) {
        logError(expression->identifier, "Use of undeclared symbol '{}'", expression->identifier->toString());
        return exprvisit_t::Failure;
    }

    if (symbol->kind == SymbolKind::Function) {
        return instantiateGenericFunction(expression, symbol, resolvedTypeArguments);
    }
    if (symbol->kind == SymbolKind::Aggregate || symbol->kind == SymbolKind::GenericType) {
        return instantiateGenericAggregate(expression, symbol, resolvedTypeArguments);
    }

    logError(expression->identifier, "Symbol '{}' is neither a generic function nor a generic aggregate",
             expression->identifier->toString());
    return exprvisit_t::Failure;
}

auto SemanticAnalyzer::visit(ast::SizeofExpression* expression) -> exprvisit_t {
    expression->semanticType = typeContext.getPoison();
    if (visit(expression->type) == exprvisit_t::Failure) { return exprvisit_t::Failure; }
    const SemanticType* targetSemanticType = expression->type->semanticType;
    if (targetSemanticType->isPoison()) {
        logError(expression, "Invalid type in sizeof expression {}", expression->toString());
        return exprvisit_t::Failure;
    }
    expression->semanticType = typeContext.getUSizeType();
    return exprvisit_t::Success;
}


}  // namespace Manganese::semantic