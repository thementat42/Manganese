#include <core.hpp>
#include <cstddef>
#include <frontend/ast.hpp>
#include <frontend/lexer/token.hpp>
#include <frontend/semantic/semantic_analyzer.hpp>
#include <frontend/semantic/symbol_table.hpp>
#include <frontend/semantic/type_context.hpp>
#include <mnstl/number.hxx>
#include <utility>
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

    const Symbol* symbol = resolveScopeSymbol(expression->identifier);
    if (symbol == nullptr || symbol->node == nullptr) {
        logError(expression->identifier, "Use of undeclared symbol '{}'", expression->identifier->toString());
        return exprvisit_t::Failure;
    }
    const StackGuard guard{genericsStack, std::move(resolvedTypeArguments)};

    if (symbol->kind == SymbolKind::Function) {
        auto* functionDeclaration = static_cast<ast::FunctionDeclarationStatement*>(symbol->node);
        if (functionDeclaration->genericTypes.size() != genericsStack.top().size()) {
            logError(expression, "Generic function '{}' expects {} type arguments, but {} were provided",
                     functionDeclaration->name, functionDeclaration->genericTypes.size(), genericsStack.top().size());
            return exprvisit_t::Failure;
        }

        // set up params for function instantiation
        auto oldParams = activeGenericParams;
        activeGenericParams.clear();
        for (std::size_t i = 0; i < functionDeclaration->genericTypes.size(); ++i) {
            activeGenericParams[functionDeclaration->genericTypes[i]] = i;
        }

        stmtvisit_t visitRes;
        {
            // If we don't do this, the analyzer will think that any instantiation of a generic function inside another
            // function body is a nested function declaration, which is wrong
            const ContextGuard<bool> instantiationNestingGuard{context.inFunction, false};
            Scope* previousScope = symbolTable.getCurrentScope();
            if (symbol->hostScope != nullptr) { symbolTable.setCurrentScope(symbol->hostScope); }

            visitRes = visit(functionDeclaration, generic_tag);

            if (symbol->hostScope != nullptr) { symbolTable.setCurrentScope(previousScope); }
        }
        const SemanticType* concreteType = nullptr;
        if (visitRes != stmtvisit_t::Failure) {
            concreteType = getInstantiatedFunctionType(functionDeclaration, genericsStack.top());
        }

        activeGenericParams = std::move(oldParams);

        if (concreteType == nullptr || concreteType->isPoison()) {
            logError(expression, "Failed to materialize instantiated function type for '{}'",
                     functionDeclaration->name);
            return exprvisit_t::Failure;
        }
        expression->semanticType = concreteType;
        expression->identifier->semanticType = concreteType;  // Attach to base for parent visitors
        return exprvisit_t::Success;
    }
    if (symbol->kind == SymbolKind::Aggregate || symbol->kind == SymbolKind::GenericType) {
        auto* aggregateDecl = static_cast<ast::AggregateDeclarationStatement*>(symbol->node);

        if (aggregateDecl->genericTypes.size() != genericsStack.top().size()) {
            logError(expression, "Generic aggregate '{}' expects {} type arguments, but {} were provided",
                     aggregateDecl->name, aggregateDecl->genericTypes.size(), genericsStack.top().size());
            return exprvisit_t::Failure;
        }

        // set up params for instantiation
        auto oldParams = activeGenericParams;
        activeGenericParams.clear();
        for (std::size_t i = 0; i < aggregateDecl->genericTypes.size(); ++i) {
            activeGenericParams[aggregateDecl->genericTypes[i]] = i;
        }

        // Instantiate the generic aggregate definition with host scope restored
        Scope* previousScope = symbolTable.getCurrentScope();
        if (symbol->hostScope != nullptr) { symbolTable.setCurrentScope(symbol->hostScope); }

        const stmtvisit_t visitRes = visit(aggregateDecl, generic_tag);

        if (symbol->hostScope != nullptr) { symbolTable.setCurrentScope(previousScope); }

        const SemanticType* concreteType = nullptr;
        if (visitRes != stmtvisit_t::Failure) {
            concreteType = getInstantiatedAggregateType(aggregateDecl, genericsStack.top());
        }

        activeGenericParams = std::move(oldParams);

        if (concreteType == nullptr || concreteType->isPoison()) {
            logError(expression, "Failed to materialize instantiated aggregate type for '{}'", aggregateDecl->name);
            return exprvisit_t::Failure;
        }

        expression->semanticType = concreteType;
        expression->identifier->semanticType = concreteType;  // Attach to base for parent visitors
        return exprvisit_t::Success;
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