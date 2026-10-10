#include <core.hpp>
#include <cstddef>
#include <frontend/ast.hpp>
#include <frontend/semantic.hpp>
#include <frontend/semantic/clone_context.hpp>
#include <frontend/semantic/generics_helpers.hpp>
#include <frontend/semantic/type_context.hpp>
#include <string>
#include <utility>
#include <utils/expression_folding.hpp>
#include <utils/resolution_status.hpp>
#include <utils/result.hpp>
#include <vector>

namespace Manganese::semantic {

auto SemanticAnalyzer::instantiateGenericAggregate(ast::GenericInstantiationExpression* expression,
                                                   const Symbol* symbol, const TypeList& typeArgs) -> exprvisit_t {
    auto* aggregateDeclaration = static_cast<ast::AggregateDeclarationStatement*>(symbol->node);

    if (aggregateDeclaration->genericTypes.size != typeArgs.size()) {
        logError(expression, "Generic aggregate '{}' expects {} type arguments, but {} were provided",
                 interner.get_view(aggregateDeclaration->name), aggregateDeclaration->genericTypes.size, typeArgs.size());
        return exprvisit_t::Failure;
    }

    InstantiationKey key{.declNode = aggregateDeclaration, .typeArgs = typeArgs};

    if (const auto* cached = instantiationCache.find(key)) {
        if (cached->state == ResolutionStatus::Success) {
            expression->semanticType = cached->semanticType;
            expression->identifier->semanticType = cached->semanticType;
            return exprvisit_t::Success;
        }
        if (cached->state == ResolutionStatus::InProgress) {
            logError(expression, "Recursive generic aggregate instantiation detected for '{}'",
                     interner.get_view(aggregateDeclaration->name));
            return exprvisit_t::Failure;
        }
    }

    instantiationCache.markAsInProgress(key);

    CloneContext cloneContext{.arena = arena, .substitutions = {}, .declarationSubstitutions = {}};
    for (std::size_t i = 0; i < aggregateDeclaration->genericTypes.size; ++i) {
        cloneContext.substitutions[aggregateDeclaration->genericTypes[i]] = typeArgs[i];
    }

    auto* clonedAggregate = aggregateDeclaration->clone(&cloneContext);
    clonedAggregate->mangledName = utils::OptionalStringID{.id = getMangledName(aggregateDeclaration->name, typeArgs)};

    Scope* previousScope = symbolTable.getCurrentScope();
    if (symbol->hostScope != nullptr) { symbolTable.setCurrentScope(symbol->hostScope); }

    const stmtvisit_t visitRes = visit(clonedAggregate);

    if (symbol->hostScope != nullptr) { symbolTable.setCurrentScope(previousScope); }

    if (visitRes == stmtvisit_t::Failure) {
        instantiationCache.markAsFailure(key);
        logError(expression, "Failed to analyze instantiated aggregate '{}'", interner.get_view(aggregateDeclaration->name));
        return exprvisit_t::Failure;
    }

    const SemanticType* concreteType = getInstantiatedAggregateType(clonedAggregate);
    if (concreteType == nullptr || concreteType->isPoison()) {
        instantiationCache.markAsFailure(key);
        logError(expression, "Failed to materialize instantiated aggregate type for '{}'", interner.get_view(aggregateDeclaration->name));
        return exprvisit_t::Failure;
    }

    {
        Scope* targetScope = (symbol->hostScope != nullptr) ? symbol->hostScope : symbolTable.getCurrentScope();
        Scope* _previousScope = symbolTable.getCurrentScope();
        symbolTable.setCurrentScope(targetScope);

        DISCARD(symbolTable.declare(*clonedAggregate->mangledName,
                                    Symbol{.type = concreteType,
                                           .node = clonedAggregate,
                                           .kind = SymbolKind::Aggregate,
                                           .visibility = aggregateDeclaration->visibility,
                                           .isMutable = false,
                                           .status = ResolutionStatus::Success}));

        symbolTable.setCurrentScope(_previousScope);
    }

    instantiationCache.markAsSuccess(key, concreteType, *clonedAggregate->mangledName, clonedAggregate);
    instantiatedDeclarations.push_back(clonedAggregate);

    expression->semanticType = concreteType;
    expression->identifier->semanticType = concreteType;

    return exprvisit_t::Success;
}

auto SemanticAnalyzer::instantiateGenericFunction(ast::GenericInstantiationExpression* expression, const Symbol* symbol,
                                                  const TypeList& typeArgs) -> exprvisit_t {
    auto* functionDeclaration = static_cast<ast::FunctionDeclarationStatement*>(symbol->node);

    if (functionDeclaration->genericTypes.size != typeArgs.size()) {
        logError(expression, "Generic function '{}' expects {} type arguments, but {} were provided",
                 interner.get_view(functionDeclaration->name), functionDeclaration->genericTypes.size, typeArgs.size());
        return exprvisit_t::Failure;
    }

    InstantiationKey key{.declNode = functionDeclaration, .typeArgs = typeArgs};

    if (const auto* cached = instantiationCache.find(key)) {
        if (cached->state == ResolutionStatus::Success) {
            expression->semanticType = cached->semanticType;
            expression->identifier->semanticType = cached->semanticType;
            return exprvisit_t::Success;
        }
        if (cached->state == ResolutionStatus::InProgress) {
            logError(expression, "Recursive generic instantiation detected for '{}'", interner.get_view(functionDeclaration->name));
            return exprvisit_t::Failure;
        }
    }

    instantiationCache.markAsInProgress(key);

    CloneContext cloneContext{.arena = arena, .substitutions = {}, .declarationSubstitutions = {}};
    for (std::size_t i = 0; i < functionDeclaration->genericTypes.size; ++i) {
        cloneContext.substitutions[functionDeclaration->genericTypes[i]] = typeArgs[i];
    }

    auto* clonedFunction = functionDeclaration->clone(&cloneContext);
    clonedFunction->mangledName = utils::OptionalStringID{.id = getMangledName(functionDeclaration->name, typeArgs)};

    stmtvisit_t visitRes;
    {
        const ContextGuard<bool> instantiationGuard{context.isInstantiatingGeneric, true};
        const ContextGuard<bool> functionNestingGuard{context.inFunction, false};

        Scope* previousScope = symbolTable.getCurrentScope();
        if (symbol->hostScope != nullptr) { symbolTable.setCurrentScope(symbol->hostScope); }

        symbolTable.enterGenericCheckingMode();
        visitRes = visit(clonedFunction);
        symbolTable.exitGenericCheckingMode();

        if (symbol->hostScope != nullptr) { symbolTable.setCurrentScope(previousScope); }
    }

    if (visitRes == stmtvisit_t::Failure) {
        instantiationCache.markAsFailure(key);
        logError(expression, "Failed to analyze instantiated function '{}'", interner.get_view(functionDeclaration->name));
        return exprvisit_t::Failure;
    }

    const SemanticType* concreteType = getInstantiatedFunctionType(clonedFunction);
    if (concreteType == nullptr || concreteType->isPoison()) {
        instantiationCache.markAsFailure(key);
        logError(expression, "Failed to materialize instantiated function type for '{}'", interner.get_view(functionDeclaration->name));
        return exprvisit_t::Failure;
    }

    {
        Scope* targetScope = (symbol->hostScope != nullptr) ? symbol->hostScope : symbolTable.getCurrentScope();
        Scope* previousScope = symbolTable.getCurrentScope();
        symbolTable.setCurrentScope(targetScope);

        DISCARD(symbolTable.declare(*clonedFunction->mangledName,
                                    Symbol{.type = concreteType,
                                           .node = clonedFunction,
                                           .kind = SymbolKind::Function,
                                           .visibility = functionDeclaration->visibility,
                                           .isMutable = false,
                                           .status = ResolutionStatus::Success}));

        symbolTable.setCurrentScope(previousScope);
    }

    instantiationCache.markAsSuccess(key, concreteType, *clonedFunction->mangledName, clonedFunction);
    instantiatedDeclarations.push_back(clonedFunction);

    expression->semanticType = concreteType;
    expression->identifier->semanticType = concreteType;
    return exprvisit_t::Success;
}

const SemanticType* SemanticAnalyzer::getInstantiatedAggregateType(
    const ast::AggregateDeclarationStatement* clonedDecl) {
    std::vector<AggregateField> instantiatedFields;
    instantiatedFields.reserve(clonedDecl->fields.size);

    for (const ast::AggregateField& fieldNode : clonedDecl->fields) {
        const SemanticType* fieldType = fieldNode.type->semanticType;
        if (fieldType == nullptr || fieldType->isPoison()) { return typeContext.getPoison(); }
        instantiatedFields.push_back(AggregateField{.name = interner.get_view(fieldNode.name), .type = fieldType});
    }

    std::string_view mangledNameView = clonedDecl->mangledName.has_value() ? interner.get_view(*clonedDecl->mangledName) : std::string_view{};
    return typeContext.getNamedAggregate(mangledNameView, std::move(instantiatedFields));
}

const SemanticType* SemanticAnalyzer::getInstantiatedFunctionType(const ast::FunctionDeclarationStatement* clonedDecl) {
    const SemanticType* resolvedReturnType
        = (clonedDecl->returnType != nullptr) ? clonedDecl->returnType->semanticType : typeContext.getVoid();

    if (resolvedReturnType->isPoison()) { return typeContext.getPoison(); }

    std::vector<Parameter> instantiatedParams;
    instantiatedParams.reserve(clonedDecl->parameters.size);

    for (const ast::FunctionParameter& paramNode : clonedDecl->parameters) {
        const SemanticType* paramType = paramNode.type->semanticType;
        if (paramType == nullptr || paramType->isPoison()) { return typeContext.getPoison(); }

        instantiatedParams.push_back(
            Parameter{.type = paramType, .isMutable = paramNode.isMutable, .isVariadic = paramNode.isVariadic});
    }

    return typeContext.getFunction(std::move(instantiatedParams), resolvedReturnType);
}

}  // namespace Manganese::semantic