#include <core.hpp>
#include <cstddef>
#include <frontend/ast.hpp>
#include <frontend/semantic.hpp>
#include <frontend/semantic/generics_helpers.hpp>
#include <frontend/semantic/type_context.hpp>

#include <string>
#include <utility>
#include <utils/result.hpp>
#include <vector>

namespace Manganese::semantic {

auto SemanticAnalyzer::visit(ast::AggregateDeclarationStatement* stmt, generic_tag_t /*unused*/) -> stmtvisit_t {
    const InstantiationKey key{.declNode = stmt, .typeArgs = genericsStack.top()};

    if (const auto* cached = instantiationCache.find(key)) {
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
        if (fieldType->isPoison()) {
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

auto SemanticAnalyzer::visit(ast::FunctionDeclarationStatement* stmt, generic_tag_t /*unused*/) -> stmtvisit_t {
    const InstantiationKey key{.declNode = stmt, .typeArgs = genericsStack.top()};

    if (const auto* cached = instantiationCache.find(key)) {
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

    const ContextGuard<bool> guard{context.inFunction, true};

    instantiationCache.markAsInProgress(key);
    auto oldParams = activeGenericParams;
    activeGenericParams.clear();

    for (std::size_t i = 0; i < stmt->genericTypes.size(); ++i) { activeGenericParams[stmt->genericTypes[i]] = i; }

    const SemanticType* resolvedReturnType = typeContext.getVoid();
    if (stmt->returnType != nullptr) {
        resolvedReturnType = resolveGenericType(stmt->returnType);
        if (resolvedReturnType == nullptr || resolvedReturnType->isPoison()) {
            activeGenericParams = std::move(oldParams);
            instantiationCache.markAsFailure(key);
            return stmtvisit_t::Failure;
        }
    }

    symbolTable.enterGenericCheckingMode();
    symbolTable.enterScope();
    const SemanticType* previousFunctionReturnType = context.currentFunctionReturnType;
    context.currentFunctionReturnType = resolvedReturnType;

    bool success = true;

    for (const auto& param : stmt->parameters) {
        const SemanticType* paramType = resolveGenericType(param.type);
        if (param.isVariadic) { paramType = typeContext.getArray(paramType, std::nullopt); }

        if (paramType == nullptr || paramType->isPoison()) {
            success = false;
            break;
        }

        const bool declarationResult
            = symbolTable.declare(
                  param.name,
                  Symbol{.type = paramType,
                         .node = stmt,
                         .kind = (param.isMutable ? SymbolKind::Parameter : SymbolKind::ConstantParameter),
                         .isMutable = param.isMutable,
                         .status = ResolutionStatus::Success})
            == Result::Failure;

        if (declarationResult) {
            logError(stmt, "Redefinition of parameter '{}' in generic function '{}'", param.name, stmt->name);
            success = false;
            break;
        }

        if (param.defaultValue != nullptr) {
            if (visit(param.defaultValue) == exprvisit_t::Failure) {
                success = false;
                break;
            }
            const SemanticType* defaultValueType = param.defaultValue->semanticType;
            if (defaultValueType == nullptr || defaultValueType->isPoison()
                || !areTypesCompatible(defaultValueType, paramType)) {
                logError(stmt, "Invalid default value type for parameter '{}'", param.name);
                success = false;
                break;
            }
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
    symbolTable.exitGenericCheckingMode();
    symbolTable.exitScope();
    activeGenericParams = std::move(oldParams);

    if (success) {
        instantiationCache.markAsSuccess(key, resolvedReturnType);
    } else {
        instantiationCache.markAsFailure(key);
    }

    return success ? stmtvisit_t::Success : stmtvisit_t::Failure;
}

const SemanticType* SemanticAnalyzer::getInstantiatedAggregateType(const ast::AggregateDeclarationStatement* decl,
                                                           const TypeList& typeArgs) {
    const InstantiationKey key{.declNode = decl, .typeArgs = typeArgs};
    const InstantiationResult* cachedResult = instantiationCache.find(key);
    if (cachedResult == nullptr || cachedResult->state != ResolutionStatus::Success) {
        return typeContext.getPoison();  // Not instantiated or failed
    }

    // Temporarily bind generic parameters for field type resolution
    auto oldParams = activeGenericParams;
    activeGenericParams.clear();
    for (std::size_t i = 0; i < decl->genericTypes.size(); ++i) { activeGenericParams[decl->genericTypes[i]] = i; }

    std::vector<AggregateField> instantiatedFields;
    instantiatedFields.reserve(decl->fields.size());

    for (const ast::AggregateField& fieldNode : decl->fields) {
        const SemanticType* fieldType = resolveGenericType(fieldNode.type);
        if (fieldType->isPoison()) {
            activeGenericParams = std::move(oldParams);
            return typeContext.getPoison();
        }
        instantiatedFields.push_back(AggregateField{.name = fieldNode.name, .type = fieldType});
    }

    activeGenericParams = std::move(oldParams);

    std::string instantiatedName = decl->name + "$";
    for (std::size_t i = 0; i < typeArgs.size(); ++i) {
        if (i > 0) { instantiatedName += "$"; }
        instantiatedName += typeArgs[i]->toString();
    }

    return typeContext.getNamedAggregate(std::string(instantiatedName), std::move(instantiatedFields));
}

const SemanticType* SemanticAnalyzer::getInstantiatedFunctionType(const ast::FunctionDeclarationStatement* decl,
                                                          const TypeList& typeArgs) {
    const InstantiationKey key{.declNode = decl, .typeArgs = typeArgs};
    const InstantiationResult* cachedResult = instantiationCache.find(key);
    if (cachedResult == nullptr || cachedResult->state != ResolutionStatus::Success) {
        return typeContext.getPoison();  // Not instantiated or failed
    }

    const SemanticType* resolvedReturnType = cachedResult->returnType;

    // Temporarily bind generic parameters for parameter type resolution
    auto oldParams = activeGenericParams;
    activeGenericParams.clear();
    for (std::size_t i = 0; i < decl->genericTypes.size(); ++i) { activeGenericParams[decl->genericTypes[i]] = i; }

    std::vector<Parameter> instantiatedParams;
    instantiatedParams.reserve(decl->parameters.size());

    for (const ast::FunctionParameter& paramNode : decl->parameters) {
        const SemanticType* paramType = resolveGenericType(paramNode.type);
        if (paramType->isPoison()) {
            activeGenericParams = std::move(oldParams);
            return typeContext.getPoison();
        }
        instantiatedParams.push_back(
            Parameter{.type = paramType, .isMutable = paramNode.isMutable, .isVariadic = paramNode.isVariadic});
    }

    activeGenericParams = std::move(oldParams);

    return typeContext.getFunction(std::move(instantiatedParams), resolvedReturnType);
}

const SemanticType* SemanticAnalyzer::resolveGenericType(const ast::Type* type) {
    if (type->primitiveType != ast::PrimitiveType::not_primitive) {
        return typeContext.getPrimitive(type->primitiveType);
    }
    using enum ast::TypeKind;

    switch (type->kind) {
        case PoisonedType: return typeContext.getPoison();
        case AggregateType: {
            const auto* aggregateType = static_cast<const ast::AggregateType*>(type);
            TypeList fields;
            fields.reserve(aggregateType->fieldTypes.size());

            for (const ast::Type* field : aggregateType->fieldTypes) {
                const SemanticType* fieldType = resolveGenericType(field);
                if (fieldType->isPoison()) { return typeContext.getPoison(); }
                fields.push_back(fieldType);
            }
            return typeContext.getAnonymousAggregate(std::move(fields));
        }
        case ArrayType: {
            const auto* arrayType = static_cast<const ast::ArrayType*>(type);
            const SemanticType* elementType = resolveGenericType(arrayType->elementType);
            if (elementType->isPoison()) { return typeContext.getPoison(); }
            if (!arrayType->lengthExpression->canFold()) {
                logError(arrayType->lengthExpression, "Array length must be a compile-time constant");
                return typeContext.getPoison();
            }
            if (!length.is_number()) {
                logError(arrayType->lengthExpression, "Array length must be an integer value");
                return typeContext.getPoison();
            }
            const auto lengthValue = length.number_unchecked();
            if (lengthValue.is_error()) {
                logError(arrayType->lengthExpression, "{}", lengthValue.error_unchecked());
                return typeContext.getPoison();
            }
            if (!lengthValue.is_integer()) {
                logError(arrayType->lengthExpression, "Array length must be an integer value");
                return typeContext.getPoison();
            }
            return typeContext.getArray(elementType, length.number_unchecked().value_as<std::size_t>());
        }
        case FunctionType: {
            const auto* functionType = static_cast<const ast::FunctionType*>(type);
            std::vector<Parameter> params;
            params.reserve(functionType->parameterTypes.size());

            for (const auto& param : functionType->parameterTypes) {
                const SemanticType* paramType = resolveGenericType(param.type);
                if (paramType->isPoison()) { return typeContext.getPoison(); }
                params.push_back(
                    Parameter{.type = paramType, .isMutable = param.isMutable, .isVariadic = param.isVariadic});
            }

            const SemanticType* returnType = resolveGenericType(functionType->returnType);
            if (returnType->isPoison()) { return typeContext.getPoison(); }
            return typeContext.getFunction(std::move(params), returnType);
        }
        case GenericInstantiationType: {
            const auto* genericType = static_cast<const ast::GenericInstantiationType*>(type);
            TypeList resolvedTypes;
            resolvedTypes.reserve(genericType->typeParameters.size());

            for (const ast::Type* param : genericType->typeParameters) {
                const SemanticType* paramType = resolveGenericType(param);
                if (paramType->isPoison()) { return typeContext.getPoison(); }
                resolvedTypes.push_back(paramType);
            }

            const Symbol* symbol = nullptr;
            if (genericType->baseType->kind == ast::TypeKind::IdentifierType) {
                const auto* IdentifierType = static_cast<const ast::IdentifierType*>(genericType->baseType);
                symbol = symbolTable.lookup(IdentifierType->name);
            } else if (genericType->baseType->kind == ast::TypeKind::ScopedType) {
                // Properly resolve namespace-qualified base types (e.g., Data::Pair)
                symbol = resolveTypeSymbol(genericType->baseType);
            }

            if (symbol == nullptr || symbol->node == nullptr) {
                logError(type, "Unknown generic base declaration");
                return typeContext.getPoison();
            }

            if (symbol->kind == SymbolKind::Aggregate || symbol->kind == SymbolKind::GenericType) {
                auto* aggregate = static_cast<ast::AggregateDeclarationStatement*>(symbol->node);
                const StackGuard guard{genericsStack, std::move(resolvedTypes)};

                Scope* previousScope = symbolTable.getCurrentScope();
                if (symbol->hostScope != nullptr) { symbolTable.setCurrentScope(symbol->hostScope); }

                auto visitResult = visit(aggregate, generic_tag);

                if (symbol->hostScope != nullptr) { symbolTable.setCurrentScope(previousScope); }

                if (visitResult == stmtvisit_t::Failure) { return typeContext.getPoison(); }
                return getInstantiatedAggregateType(aggregate, genericsStack.top());
            }

            logError(type, "Symbol '{}' is not a generic type", genericType->baseType->toString());
            return typeContext.getPoison();
        }
        case PointerType: {
            const auto* pointerType = static_cast<const ast::PointerType*>(type);
            const SemanticType* baseType = resolveGenericType(pointerType->baseType);
            if (baseType->isPoison()) { return typeContext.getPoison(); }
            return typeContext.getPointer(baseType, pointerType->isMutable);
        }
        case ScopedType: {
            // Qualified non-generic type resolution (e.g., foo::bar::MyStruct)
            const Symbol* symbol = resolveTypeSymbol(type);
            if (symbol == nullptr) {
                logError(type, "Unknown scoped type '{}'", type->toString());
                return typeContext.getPoison();
            }

            if (symbol->kind == SymbolKind::Aggregate || symbol->kind == SymbolKind::TypeAlias) { return symbol->type; }

            logError(type, "Symbol '{}' is not a type", type->toString());
            return typeContext.getPoison();
        }
        case IdentifierType: {
            const auto* IdentifierType = static_cast<const ast::IdentifierType*>(type);

            // some symbol type (e.g. T)
            if (auto it = activeGenericParams.find(IdentifierType->name); it != activeGenericParams.end()) {
                const std::size_t index = it->second;
                if (!genericsStack.is_empty() && index < genericsStack.top().size()) {
                    return genericsStack.top()[index];
                }
                logError(type, "Unbound generic parameter '{}'", IdentifierType->name);
                return typeContext.getPoison();
            }

            // Lookup named, non-generic type in symbolt able
            const Symbol* symbol = symbolTable.lookup(IdentifierType->name);
            if (symbol == nullptr) {
                logError(type, "Unknown type name '{}'", IdentifierType->name);
                return typeContext.getPoison();
            }
            if (symbol->kind == SymbolKind::Aggregate || symbol->kind == SymbolKind::TypeAlias) { return symbol->type; }

            logError(type, "Symbol '{}' is not a type", IdentifierType->name);
            return typeContext.getPoison();
        }
        case TypeofType: {
            auto* nestedExpression = static_cast<const ast::TypeofType*>(type)->expression;
            if (visit(nestedExpression) == exprvisit_t::Failure) { return typeContext.getPoison(); }
            return nestedExpression->semanticType;
        }
    }
    ASSERT_UNREACHABLE("Unknown ast::TypeKind in resolveGenericType");
}

}  // namespace Manganese::semantic