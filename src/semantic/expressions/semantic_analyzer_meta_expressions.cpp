#include <ast.hpp>
#include <core.hpp>
#include <lexer/token.hpp>
#include <semantic/semantic_analyzer.hpp>
#include <semantic/symbol_table.hpp>
#include <semantic/type_context.hpp>

namespace Manganese::semantic {

auto SemanticAnalyzer::visit(ast::AlignofExpression* expression) -> exprvisit_t {
    expression->semanticType = typeContext.getPoison();
    if (visit(expression->type) == exprvisit_t::Failure) { return exprvisit_t::Failure; }
    const SemanticType* targetSemanticType = expression->type->semanticType;
    if (targetSemanticType->isPoison()) {
        logError(expression, "Invalid type in alignof expression {}", expression->toString(interner));
        return exprvisit_t::Failure;
    }
    expression->semanticType = typeContext.getUSizeType();
    return exprvisit_t::Success;
}

auto SemanticAnalyzer::visit(ast::CopyExpression* expression) -> exprvisit_t {
    expression->semanticType = typeContext.getPoison();
    if (visit(expression->copiedExpression) == exprvisit_t::Failure) { return exprvisit_t::Failure; }
    if (!isLvalue(expression->copiedExpression)) {
        logWarning(expression->copiedExpression,
                   "copying an rvalue expression '{}' is redundant; consider removing the copy",
                   expression->copiedExpression->toString(interner));
    }
    expression->semanticType = expression->copiedExpression->semanticType;

    return exprvisit_t::Success;
}

auto SemanticAnalyzer::visit(ast::GenericInstantiationExpression* expression) -> exprvisit_t {
    expression->semanticType = typeContext.getPoison();
    TypeList resolvedTypeArguments;
    resolvedTypeArguments.reserve(expression->types.size);

    for (ast::Type* type : expression->types) {
        if (visit(type) == typevisit_t::Failure) { return exprvisit_t::Failure; }
        const SemanticType* resolved = type->semanticType;
        if (resolved->isPoison()) {
            logError(type, "Failed to resolve generic type argument '{}' in generic expression",
                     type->toString(interner));
            return exprvisit_t::Failure;
        }
        resolvedTypeArguments.push_back(resolved);
    }

    expression->semanticTypes = arena.emplace_range(resolvedTypeArguments);

    const Symbol* symbol = resolveScopeSymbol(expression->identifier);
    if (symbol == nullptr || symbol->node == nullptr) {
        logError(expression->identifier, "Use of undeclared symbol '{}'", expression->identifier->toString(interner));
        return exprvisit_t::Failure;
    }

    if (symbol->kind == SymbolKind::Function) {
        return instantiateGenericFunction(expression, symbol, resolvedTypeArguments);
    }
    if (symbol->kind == SymbolKind::Aggregate || symbol->kind == SymbolKind::GenericType) {
        return instantiateGenericAggregate(expression, symbol, resolvedTypeArguments);
    }

    logError(expression->identifier, "Symbol '{}' is neither a generic function nor a generic aggregate",
             expression->identifier->toString(interner));
    return exprvisit_t::Failure;
}

auto SemanticAnalyzer::visit(ast::ReferenceExpression* expression) -> exprvisit_t {
    expression->semanticType = typeContext.getPoison();
    if (visit(expression->referencedExpression) == exprvisit_t::Failure) { return exprvisit_t::Failure; }

    const SemanticType* operandType = expression->referencedExpression->semanticType;
    if (operandType == nullptr || operandType->isPoison()) { return exprvisit_t::Failure; }

    if (operandType->isReference()) {
        logError(expression, "Cannot form a reference to a reference type");
        return exprvisit_t::Failure;
    }
    if (operandType->isVoid()) {
        logError(expression, "Cannot form a reference to void");
        return exprvisit_t::Failure;
    }

    if (!isLvalue(expression->referencedExpression)) {
        logError(expression, "Cannot form a reference to an rvalue");
        return exprvisit_t::Failure;
    }

    if (expression->isMutable && !isMutableExpression(expression->referencedExpression)) {
        logError(expression, "Cannot form a mutable reference to immutable expression '{}'",
                 expression->referencedExpression->toString(interner));
        return exprvisit_t::Failure;
    }

    expression->semanticType = typeContext.getReference(operandType, expression->isMutable);
    return exprvisit_t::Success;
}

auto SemanticAnalyzer::visit(ast::SizeofExpression* expression) -> exprvisit_t {
    expression->semanticType = typeContext.getPoison();
    if (visit(expression->type) == exprvisit_t::Failure) { return exprvisit_t::Failure; }
    const SemanticType* targetSemanticType = expression->type->semanticType;
    if (targetSemanticType->isPoison()) {
        logError(expression, "Invalid type in sizeof expression {}", expression->toString(interner));
        return exprvisit_t::Failure;
    }
    expression->semanticType = typeContext.getUSizeType();
    return exprvisit_t::Success;
}

}  // namespace Manganese::semantic