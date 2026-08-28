#include <core.hpp>
#include <cstddef>
#include <frontend/ast.hpp>
#include <frontend/semantic.hpp>
#include <frontend/semantic/generics_helpers.hpp>
#include <frontend/semantic/type_context.hpp>
#include <mnstl/fold_result.hxx>

namespace Manganese::semantic {

auto Analyzer::visit(ast::AggregateDeclarationStatement* stmt, generic_tag_t) -> stmtvisit_t {
    const InstantiationKey key{.declNode = stmt, .typeArgs = genericsStack.top()};

    if (auto* cached = instantiationCache.find(key)) {
        if (cached->state == ResolutionStatus::InProgress) {
            logError(stmt, "Decursive aggregate layout dependency in '{}'", stmt->name);
            return stmtvisit_t::Failure;
        }
        return cached->state == ResolutionStatus::Success ? stmtvisit_t::Success : stmtvisit_t::Failure;
    }

    instantiationCache.markAsInProgress(key);
    auto oldParams = activeGenericParams;
    activeGenericParams.clear();

    for (std::size_t i = 0; i < stmt->genericTypes.size(); ++i) { activeGenericParams[stmt->genericTypes[i]] = i; }
    bool success = true;
    for (const auto& field : stmt->fields) {
        const SemanticType* fieldType = resolveGenericType(field.type);
        if (!fieldType) {
            success = false;
            break;
        }
    }
    activeGenericParams = std::move(oldParams);
    if (success) {
        instantiationCache.markAsSuccess(key, nullptr);
    } else {
        instantiationCache.markAsFailure(key);
    }

    return success ? stmtvisit_t::Success : stmtvisit_t::Failure;
}

auto Analyzer::visit(ast::FunctionDeclarationStatement* stmt, generic_tag_t) -> stmtvisit_t {
    const InstantiationKey key{.declNode = stmt, .typeArgs = genericsStack.top()};

    if (auto* cached = instantiationCache.find(key)) {
        if (cached->state == ResolutionStatus::InProgress) {
            logError(stmt, "Recursive generic function instantiation dependency in '{}'", stmt->name);
            return stmtvisit_t::Failure;
        }
        return cached->state == ResolutionStatus::Success ? stmtvisit_t::Success : stmtvisit_t::Failure;
    }

    if (context.inFunction) {
        logError(stmt, "Cannot declare nested functions");
        instantiationCache.markAsFailure(key);
        return stmtvisit_t::Failure;
    }

    ContextGuard<bool> guard{context.inFunction, true};

    instantiationCache.markAsInProgress(key);
    auto oldParams = activeGenericParams;
    activeGenericParams.clear();

    for (std::size_t i = 0; i < stmt->genericTypes.size(); ++i) { activeGenericParams[stmt->genericTypes[i]] = i; }

    const SemanticType* resolvedReturnType = typeContext.getVoid();
    if (stmt->returnType) {
        resolvedReturnType = resolveGenericType(stmt->returnType);
        if (!resolvedReturnType) {
            activeGenericParams = std::move(oldParams);
            instantiationCache.markAsFailure(key);
            return stmtvisit_t::Failure;
        }
    }

    symbolTable.enterScope();
    const SemanticType* previousFunctionReturnType = context.currentFunctionReturnType;
    context.currentFunctionReturnType = resolvedReturnType;

    bool success = true;

    for (const auto& param : stmt->parameters) {
        const SemanticType* paramType = resolveGenericType(param.type);
        if (!paramType) {
            success = false;
            break;
        }

        const bool declarationResult
            = symbolTable.declare(
                  param.name,
                  Symbol{.type = paramType,
                         .node = nullptr,
                         .kind = (param.isMutable ? SymbolKind::Parameter : SymbolKind::ConstantParameter),
                         .isMutable = param.isMutable,
                         .status = ResolutionStatus::Success})
            == Result::Failure;
        if (declarationResult) {
            logError(stmt, "Redefinition of parameter '{}' in generic function '{}'", param.name, stmt->name);
            success = false;
            break;
        }
    }

    if (success) {
        for (auto* bodyStmt : stmt->body) {
            if (visit(bodyStmt) == stmtvisit_t::Failure) {
                success = false;
                break;
            }
        }
    }

    context.currentFunctionReturnType = previousFunctionReturnType;
    symbolTable.exitScope();
    activeGenericParams = std::move(oldParams);

    if (success) {
        instantiationCache.markAsSuccess(key, resolvedReturnType);
    } else {
        instantiationCache.markAsFailure(key);
    }

    return success ? stmtvisit_t::Success : stmtvisit_t::Failure;
}

const SemanticType* Analyzer::getInstantiatedFunctionType(const ast::FunctionDeclarationStatement* decl,
                                                          const TypeList& typeArgs) {
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
        const SemanticType* paramType = resolveGenericType(paramNode.type);
        if (!paramType) { return nullptr; }
        instantiatedParams.push_back(
            Parameter{.isMutable = paramNode.isMutable, .isVariadic = paramNode.isVariadic, .type = paramType});
    }
    return typeContext.getFunction(std::move(instantiatedParams), resolvedReturnType);
}

const SemanticType* Analyzer::getInstantiatedAggregateType(const ast::AggregateDeclarationStatement* decl,
                                                           const TypeList& typeArgs) {
    if (!decl) [[unlikely]] { return nullptr; }
    InstantiationKey key{.declNode = decl, .typeArgs = typeArgs};
    const InstantiationResult* cachedResult = instantiationCache.find(key);
    if (!cachedResult || cachedResult->state != ResolutionStatus::Success) {
        return nullptr;  // Not instantiated or failed
    }
    std::vector<AggregateField> instantiatedFields;
    instantiatedFields.reserve(decl->fields.size());

    for (const ast::AggregateField& fieldNode : decl->fields) {
        const SemanticType* fieldType = resolveGenericType(fieldNode.type);
        if (!fieldType) { return nullptr; }
        instantiatedFields.push_back(AggregateField{.name = fieldNode.name, .type = fieldType});
    }

    std::string instantiatedName = decl->name + "$";
    for (std::size_t i = 0; i < typeArgs.size(); ++i) {
        if (i > 0) { instantiatedName += "$"; }
        instantiatedName += typeArgs[i]->toString();
    }

    return typeContext.getNamedAggregate(std::move(instantiatedName), std::move(instantiatedFields));
}

const SemanticType* Analyzer::resolveGenericType(const ast::Type* type) {
    if (!type) [[unlikely]] { return nullptr; }
    if (type->primitiveType != ast::PrimitiveType_t::not_primitive) {
        return typeContext.getPrimitive(type->primitiveType);
    }
    using enum ast::TypeKind;

    switch (type->kind) {
        case AggregateType: {
            const auto* aggregateType = static_cast<const ast::AggregateType*>(type);
            TypeList fields;
            fields.reserve(aggregateType->fieldTypes.size());

            for (const ast::Type* field : aggregateType->fieldTypes) {
                const SemanticType* fieldType = resolveGenericType(field);
                if (!fieldType) { return nullptr; }
                fields.push_back(fieldType);
            }
            return typeContext.getAnonymousAggregate(std::move(fields));
        }
        case ArrayType: {
            const auto* arrayType = static_cast<const ast::ArrayType*>(type);
            const SemanticType* elementType = resolveGenericType(arrayType->elementType);
            if (!elementType) { return nullptr; }
            mnstl::fold_result_t length = arrayType->lengthExpression->fold();
            if (!length.has_value()) {
                logError(arrayType->lengthExpression, "Array length must be a compile-time constant");
                return nullptr;
            }
            if (!length.is_number()) {
                logError(arrayType->lengthExpression, "Array length must be an integer value");
                return nullptr;
            }
            const auto lengthValue = length.number_unchecked();
            if (lengthValue.is_error()) {
                logError(arrayType->lengthExpression, "{}", lengthValue.error_unchecked());
                return nullptr;
            }
            if (!lengthValue.is_integer()) {
                logError(arrayType->lengthExpression, "Array length must be an integer value");
                return nullptr;
            }
            return typeContext.getArray(elementType, length.number_unchecked().value_as<std::size_t>());
        }
        case FunctionType: {
            auto* functionType = static_cast<const ast::FunctionType*>(type);
            std::vector<Parameter> params;
            params.reserve(functionType->parameterTypes.size());

            for (const auto& param : functionType->parameterTypes) {
                const SemanticType* paramType = resolveGenericType(param.type);
                if (!paramType) { return nullptr; }
                params.push_back(
                    Parameter{.isMutable = param.isMutable, .isVariadic = param.isVariadic, .type = paramType});
            }

            const SemanticType* returnType = resolveGenericType(functionType->returnType);
            if (!returnType) { return nullptr; }
            return typeContext.getFunction(std::move(params), returnType);
        }
        case GenericInstantiationType: {
            const auto* genericType = static_cast<const ast::GenericInstantiationType*>(type);
            TypeList resolvedTypes;
            resolvedTypes.reserve(genericType->typeParameters.size());

            for (const ast::Type* param : genericType->typeParameters) {
                const SemanticType* paramType = resolveGenericType(param);
                if (!paramType) { return nullptr; }
                resolvedTypes.push_back(paramType);
            }

            const Symbol* symbol = nullptr;
            if (genericType->baseType->kind == SymbolType) {
                const auto* symbolType = static_cast<const ast::SymbolType*>(genericType->baseType);
                symbol = symbolTable.lookup(symbolType->name);
            } else if (genericType->baseType->kind == ScopedType) {
                // const auto* scopedType = static_cast<const ast::ScopedType*>(genericType->baseType);
                symbol = nullptr;// symbolTable.scopedLookup(scopedType->qualifier, scopedType->baseType);
            }
            if (!symbol || !symbol->node) {
                logError(type, "Unknown generic base declaration");
                return nullptr;
            }

            if (symbol->kind == SymbolKind::Aggregate || symbol->kind == SymbolKind::GenericType) {
                auto* aggregate = static_cast<ast::AggregateDeclarationStatement*>(symbol->node);
                StackGuard guard{genericsStack, std::move(resolvedTypes)};
                if (visit(aggregate, generic_tag) == stmtvisit_t::Failure) { return nullptr; }
                return getInstantiatedAggregateType(aggregate, genericsStack.top());
            }

            logError(type, "Symbol '{}' is not a generic type", genericType->baseType->toString());
            return nullptr;
        }
        case PointerType: {
            const auto* pointerType = static_cast<const ast::PointerType*>(type);
            const SemanticType* baseType = resolveGenericType(pointerType->baseType);
            if (!baseType) { return nullptr; }
            return typeContext.getPointer(baseType, pointerType->isMutable);
        }
        case ScopedType: {
            // TODO
            return nullptr;
        }
        case SymbolType: {
            const auto* symbolType = static_cast<const ast::SymbolType*>(type);

            // some symbol type (e.g. T)
            if (auto it = activeGenericParams.find(symbolType->name); it != activeGenericParams.end()) {
                std::size_t index = it->second;
                if (!genericsStack.is_empty() && index < genericsStack.top().size()) {
                    return genericsStack.top()[index];
                }
                logError(type, "Unbound generic parameter '{}'", symbolType->name);
                return nullptr;
            }

            // Lookup named, non-generic type in symbolt able
            const Symbol* symbol = symbolTable.lookup(symbolType->name);
            if (!symbol) {
                logError(type, "Unknown type name '{}'", symbolType->name);
                return nullptr;
            }
            if (symbol->kind == SymbolKind::Aggregate || symbol->kind == SymbolKind::TypeAlias) { return symbol->type; }

            logError(type, "Symbol '{}' is not a type", symbolType->name);
            return nullptr;
        }
        case TypeofType: {
            auto* nestedExpression = static_cast<const ast::TypeofType*>(type)->expression;
            if (visit(nestedExpression) == exprvisit_t::Failure) { return nullptr; }
            return nestedExpression->semanticType;
        }
        default: ASSERT_UNREACHABLE("Unknown ast::TypeKind in resolveGenericType");
    }
}

}  // namespace Manganese::semantic