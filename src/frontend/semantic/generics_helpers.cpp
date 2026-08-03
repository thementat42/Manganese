#include <frontend/ast.hpp>
#include <frontend/semantic/analyzer.hpp>
#include <frontend/semantic/generics_helpers.hpp>
#include <frontend/semantic/type_context.hpp>

namespace Manganese::semantic {

auto analyzer::visit(ast::AggregateDeclarationStatement*, generic_tag_t) -> stmtvisit_t { return stmtvisit_t::Success; }
auto analyzer::visit(ast::FunctionDeclarationStatement*, generic_tag_t) -> stmtvisit_t { return stmtvisit_t::Success; }

const SemanticType* analyzer::getInstantiatedFunctionType(const ast::FunctionDeclarationStatement* decl,
                                                          const std::vector<const SemanticType*>& typeArgs) {
    if (!decl) [[unlikely]] { return nullptr; }
    InstantiationKey key{.declNode = decl, .typeArgs = typeArgs};
    const InstantiationResult* cachedResult = instantiationCache.find(key);
    if (!cachedResult || cachedResult->state != ResolutionStatus::Success) {
        return nullptr;  // Not instantiated or failed
    }

    const SemanticType* resolvedReturnType = cachedResult->returnType;

    std::vector<Parameter> instantiatedParams;
    instantiatedParams.reserve(decl->parameters.size());

    for (const ast::FunctionParameter& paramNode : decl->parameters) {
        visit(paramNode.type);
        const SemanticType* paramType = paramNode.type->semanticType;
        if (!paramType) { return nullptr; }
        instantiatedParams.push_back(Parameter{.isMutable = paramNode.isMutable, .type = paramType});
    }
    return typeContext.getFunction(std::move(instantiatedParams), resolvedReturnType);
}

const SemanticType* analyzer::getInstantiatedAggregateType(const ast::AggregateDeclarationStatement* decl,
                                                           const std::vector<const SemanticType*>& typeArgs) {
    if (!decl) [[unlikely]] { return nullptr; }
    InstantiationKey key{.declNode = decl, .typeArgs = typeArgs};
    const InstantiationResult* cachedResult = instantiationCache.find(key);
    if (!cachedResult || cachedResult->state != ResolutionStatus::Success) {
        return nullptr;  // Not instantiated or failed
    }
    std::vector<AggregateField> instantiatedFields;
    instantiatedFields.reserve(decl->fields.size());

    for (const ast::AggregateField& fieldNode : decl->fields) {
        // resolveType substitutes generic parameters (e.g. T -> float) using genericStack.top()
        visit(fieldNode.type);
        const SemanticType* fieldType = fieldNode.type->semanticType;
        if (!fieldType) { return nullptr; }
        instantiatedFields.push_back(AggregateField{.name = fieldNode.name, .type = fieldType});
    }

    std::string instantiatedName = decl->name + "$";
    for (std::size_t i = 0; i < typeArgs.size(); ++i) {
        if (i > 0) { instantiatedName += "$"; }
        instantiatedName += typeArgs[i]->toString();
    }

    return typeContext.getNamedAggregate(instantiatedName, std::move(instantiatedFields));
}

}  // namespace Manganese::semantic