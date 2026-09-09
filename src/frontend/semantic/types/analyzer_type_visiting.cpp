#include <core.hpp>
#include <cstddef>
#include <frontend/ast.hpp>
#include <frontend/semantic.hpp>
#include <frontend/semantic/symbol_table.hpp>
#include <frontend/semantic/type_context.hpp>

#include <mnstl/number.hxx>
#include <string_view>
#include <utility>
#include <vector>

namespace Manganese::semantic {

auto SemanticAnalyzer::visit(ast::AggregateType* type) -> typevisit_t {
    const auto* aggregateType = static_cast<const ast::AggregateType*>(type);
    TypeList resolvedFields;
    resolvedFields.reserve(aggregateType->fieldTypes.size());

    for (ast::Type* fieldType : aggregateType->fieldTypes) {
        DISCARD(visit(fieldType));
        const SemanticType* resolvedFieldType = fieldType->semanticType;
        if (resolvedFieldType->isPoison()) { return typevisit_t::Failure; }
        resolvedFields.push_back(resolvedFieldType);
    }
    type->semanticType = typeContext.getAnonymousAggregate(std::move(resolvedFields));
    return typevisit_t::Success;
}

auto SemanticAnalyzer::visit(ast::ArrayType* type) -> typevisit_t {
    const auto* arrayType = static_cast<const ast::ArrayType*>(type);

    const SemanticType* outerVarType = context.currentVariableDeclarationType;
    const SemanticType* innerVarType = nullptr;
    if (outerVarType != nullptr && outerVarType->isArray()) {
        innerVarType = static_cast<const Array*>(outerVarType)->elementType;
    }

    context.currentVariableDeclarationType = innerVarType;
    auto visitResult = visit(arrayType->elementType);
    context.currentVariableDeclarationType = outerVarType;

    if (visitResult == typevisit_t::Failure) { return typevisit_t::Failure; }

    const SemanticType* elementType = arrayType->elementType->semanticType;
    if (elementType->isPoison()) {
        logError(type, "Cannot form array of invalid type '{}'", arrayType->elementType->toString());
        return typevisit_t::Failure;
    }
    if (elementType->isVoid()) { logError(type, "Cannot form an array of 'void'"); }

    std::optional<std::size_t> length = std::nullopt;
    if (arrayType->lengthExpression != nullptr) {
        if (visit(arrayType->lengthExpression) == typevisit_t::Failure) { return typevisit_t::Failure; }
        if (!arrayType->lengthExpression->canFold()) {
            logError(arrayType->lengthExpression, "Array length must be a constant integer expression");
            return typevisit_t::Failure;
        }
        const auto lengthVal = fold.number_unchecked();
        if (lengthVal <= 0) {
            logError(arrayType->lengthExpression, "Array length must be greater than 0");
            return typevisit_t::Failure;
        }
        length = lengthVal.value_as<std::size_t>();
    } else if (outerVarType != nullptr && outerVarType->isArray()) {
        length = static_cast<const Array*>(outerVarType)->length;
    }

    type->semanticType = typeContext.getArray(elementType, length);
    return typevisit_t::Success;
}

auto SemanticAnalyzer::visit(ast::FunctionType* type) -> typevisit_t {
    const ast::FunctionType* functionType = static_cast<const ast::FunctionType*>(type);
    std::vector<Parameter> resolvedParameterTypes;
    for (const ast::FunctionParameterType& parameterType : functionType->parameterTypes) {
        DISCARD(visit(parameterType.type));
        const SemanticType* resolvedParameterType = parameterType.type->semanticType;
        if (resolvedParameterType->isPoison()) { return typevisit_t::Failure; }

        resolvedParameterTypes.push_back({.type = resolvedParameterType,
                                          .isMutable = parameterType.isMutable,
                                          .isVariadic = parameterType.isVariadic});
    }
    const SemanticType* returnType = typeContext.getVoid();
    if (functionType->returnType != nullptr) {
        // function is not returning void
        DISCARD(visit(functionType->returnType));
        returnType = functionType->returnType->semanticType;
        if (returnType->isPoison()) { return typevisit_t::Failure; }
    }
    type->semanticType = typeContext.getFunction(std::move(resolvedParameterTypes), returnType);
    return typevisit_t::Success;
}

auto SemanticAnalyzer::visit(ast::GenericInstantiationType* type) -> typevisit_t {
    const SemanticType* resolved = resolveGenericType(type);
    if (resolved == nullptr) { return typevisit_t::Failure; }
    type->semanticType = resolved;
    return typevisit_t::Success;
}

auto SemanticAnalyzer::visit(ast::PointerType* type) -> typevisit_t {
    const auto* pointerType = static_cast<const ast::PointerType*>(type);
    DISCARD(visit(pointerType->baseType));
    const SemanticType* baseType = pointerType->baseType->semanticType;
    if (baseType->isPoison()) {
        logError(type, "Cannot form pointer to invalid type '{}'", pointerType->baseType->toString());
        return typevisit_t::Failure;
    }
    if (baseType->isVoid()) {
        logError(type, "Cannot form pointer to a void expression");
        return typevisit_t::Failure;
    }
    type->semanticType = typeContext.getPointer(baseType, pointerType->isMutable);
    return typevisit_t::Success;
}

auto SemanticAnalyzer::visit(ast::ScopedType* type) -> typevisit_t {
    const Symbol* scopeSymbol = nullptr;

    if (type->scope->kind == ast::TypeKind::IdentifierType) {
        // regular identifier (e.g. Foo::Bar)
        scopeSymbol = symbolTable.lookup(static_cast<ast::IdentifierType*>(type->scope)->name);
    } else if (type->scope->kind == ast::TypeKind::ScopedType) {
        // chained resolution (e.g. Foo::Bar::Baz): recursively resolve it
        if (visit(type->scope) == typevisit_t::Failure) { return typevisit_t::Failure; }
        scopeSymbol = context.nestedScopeResolutionCurrentSymbol;
    }

    if (scopeSymbol == nullptr) {
        logError(type, "Unknown scope");
        return typevisit_t::Failure;
    }
    if (scopeSymbol->scopeDefined == nullptr) {
        logError(type, "'{}' is not a namespace or module", type->scope->toString());
        return typevisit_t::Failure;
    }
    if (type->type->kind != ast::TypeKind::IdentifierType) {
        logError(type->type, "Expected an identifier in a scope resolution expression");
        return typevisit_t::Failure;
    }
    const std::string_view memberName = static_cast<ast::IdentifierType*>(type->type)->name;

    const Symbol* memberSymbol = decltype(symbolTable)::scopedLookup(scopeSymbol->scopeDefined, memberName);
    if (memberSymbol == nullptr) {
        logError(type->type, "No member named '{}' in scope", memberName);
        return typevisit_t::Failure;
    }

    context.nestedScopeResolutionCurrentSymbol = memberSymbol;

    if (memberSymbol->kind == SymbolKind::Namespace) { return typevisit_t::Success; }

    if (memberSymbol->kind != SymbolKind::Aggregate && memberSymbol->kind != SymbolKind::TypeAlias) {
        logError(type->type, "'{}' in scope '{}' is not a type", memberName,
                 scopeSymbol->scopeDefined->getQualifiedName());
        return typevisit_t::Failure;
    }
    type->semanticType = memberSymbol->type;
    context.nestedScopeResolutionCurrentSymbol = memberSymbol;
    return typevisit_t::Success;
}

auto SemanticAnalyzer::visit(ast::IdentifierType* type) -> typevisit_t {
    const auto* IdentifierType = static_cast<const ast::IdentifierType*>(type);
    if (IdentifierType->primitiveType != ast::PrimitiveType::not_primitive) {
        type->semanticType = typeContext.getPrimitive(IdentifierType->primitiveType);
        return typevisit_t::Success;
    }
    const Symbol* symbol = symbolTable.lookup(IdentifierType->name);
    if (symbol == nullptr) {
        logError(type, "Unknown type '{}'", IdentifierType->name);
        return typevisit_t::Failure;
    }
    if (symbol->kind == SymbolKind::TypeAlias && symbol->status != ResolutionStatus::Success) {
        auto* aliasStatement = static_cast<ast::AliasStatement*>(symbol->node);
        if (visit(aliasStatement) == typevisit_t::Failure) { return typevisit_t::Failure; }
    }
    if (symbol->type == nullptr) {
        logError(type, "'{}' is not a valid type", IdentifierType->name);
        return typevisit_t::Failure;
    }
    type->semanticType = symbol->type;
    return typevisit_t::Success;
}

auto SemanticAnalyzer::visit(ast::TypeofType* type) -> typevisit_t {
    const auto* typeofType = static_cast<const ast::TypeofType*>(type);
    if (visit(typeofType->expression) == typevisit_t::Failure) { return typevisit_t::Failure; }
    type->semanticType = typeofType->expression->semanticType;
    return typevisit_t::Success;
}

auto SemanticAnalyzer::visit(ast::PoisonedType* /*unused*/) -> typevisit_t { return typevisit_t::Failure; }

}  // namespace Manganese::semantic
