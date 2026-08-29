#include <core.hpp>
#include <cstddef>
#include <frontend/ast.hpp>
#include <frontend/semantic.hpp>
#include <frontend/semantic/symbol_table.hpp>
#include <frontend/semantic/type_context.hpp>
#include <mnstl/number.hxx>
#include <utility>
#include <vector>

namespace Manganese::semantic {

auto Analyzer::visit(ast::AggregateType* type) -> typevisit_t {
    const auto* aggregateType = static_cast<const ast::AggregateType*>(type);
    TypeList resolvedFields;
    resolvedFields.reserve(aggregateType->fieldTypes.size());

    for (ast::Type* fieldType : aggregateType->fieldTypes) {
        visit(fieldType);
        const SemanticType* resolvedFieldType = fieldType->semanticType;
        if (!resolvedFieldType) { return typevisit_t::Failure; }
        resolvedFields.push_back(resolvedFieldType);
    }
    type->semanticType = typeContext.getAnonymousAggregate(std::move(resolvedFields));
    return typevisit_t::Success;
}

auto Analyzer::visit(ast::ArrayType* type) -> typevisit_t {
    const auto* arrayType = static_cast<const ast::ArrayType*>(type);

    // for nested arrays
    const SemanticType* outerVarType = context.currentVariableDeclarationType;
    context.currentVariableDeclarationType = nullptr;
    visit(arrayType->elementType);
    context.currentVariableDeclarationType = outerVarType;

    const SemanticType* elementType = arrayType->elementType->semanticType;

    if (!elementType) {
        logError(type, "Cannot form array of invalid type '{}'", arrayType->elementType->toString());
        return typevisit_t::Failure;
    }
    if (elementType->isVoid()) { logError(type, "Cannot form an array of 'void'"); }

    std::size_t length;
    if (arrayType->lengthExpression) {
        if (visit(arrayType->lengthExpression) == typevisit_t::Failure) { return typevisit_t::Failure; }
        const mnstl::fold_result_t fold = arrayType->lengthExpression->fold();
        if (!fold.is_number()) {
            logError(arrayType->lengthExpression, "Array length ({}) must be a constant expression",
                     arrayType->lengthExpression->toString());
            return typevisit_t::Failure;
        }
        const mnstl::number_t lengthValue = fold.number_unchecked();
        if (lengthValue.is_error()) {
            logError(arrayType->lengthExpression, "{}", lengthValue.error_unchecked());
            return typevisit_t::Failure;
        }
        if (!lengthValue.is_integer()) {
            logError(arrayType->lengthExpression, "Array length must be an integer value");
            return typevisit_t::Failure;
        }
        if (lengthValue <= 0) {
            logError(arrayType->lengthExpression, "Array length must be greater than 0 (got {})",
                     lengthValue.to_string());
            return typevisit_t::Failure;
        }
        length = lengthValue.value_as<std::size_t>();
    } else if (context.currentVariableDeclarationType && context.currentVariableDeclarationType->isArray()) {
        length = static_cast<const Array*>(context.currentVariableDeclarationType)->length;
    } else [[unlikely]] {
        logError(type, "Cannot infer array length; explicitly specify length or provide an initializer");
        return typevisit_t::Failure;
    }

    type->semanticType = typeContext.getArray(elementType, length);
    return typevisit_t::Success;
}

auto Analyzer::visit(ast::FunctionType* type) -> typevisit_t {
    const ast::FunctionType* functionType = static_cast<const ast::FunctionType*>(type);
    std::vector<Parameter> resolvedParameterTypes;
    for (const ast::FunctionParameterType& parameterType : functionType->parameterTypes) {
        visit(parameterType.type);
        const SemanticType* resolvedParameterType = parameterType.type->semanticType;
        if (!resolvedParameterType) { return typevisit_t::Failure; }

        resolvedParameterTypes.push_back({.isMutable = parameterType.isMutable,
                                          .isVariadic = parameterType.isVariadic,
                                          .type = resolvedParameterType});
    }
    const SemanticType* returnType = typeContext.getVoid();
    if (functionType->returnType) {
        // function is not returning void
        visit(functionType->returnType);
        returnType = functionType->returnType->semanticType;
        if (!returnType) { return typevisit_t::Failure; }
    }
    type->semanticType = typeContext.getFunction(std::move(resolvedParameterTypes), returnType);
    return typevisit_t::Success;
}

auto Analyzer::visit(ast::GenericInstantiationType* type) -> typevisit_t {
    const SemanticType* resolved = resolveGenericType(type);
    if (!resolved) { return typevisit_t::Failure; }
    type->semanticType = resolved;
    return typevisit_t::Success;
}

auto Analyzer::visit(ast::PointerType* type) -> typevisit_t {
    const auto* pointerType = static_cast<const ast::PointerType*>(type);
    visit(pointerType->baseType);
    const SemanticType* baseType = pointerType->baseType->semanticType;
    if (!baseType) {
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

auto Analyzer::visit(ast::ScopedType* type) -> typevisit_t {
    const Symbol* scopeSymbol = nullptr;

    if (type->scope->kind == ast::TypeKind::SymbolType) {
        // regular identifier (e.g. Foo::Bar)
        scopeSymbol = symbolTable.lookup(static_cast<ast::SymbolType*>(type->scope)->name);
    } else if (type->scope->kind == ast::TypeKind::ScopedType) {
        // chained resolution (e.g. Foo::Bar::Baz): recursively resolve it
        if (visit(type->scope) == typevisit_t::Failure) { return typevisit_t::Failure; }
        scopeSymbol = context.nestedScopeResolutionCurrentSymbol;
    }

    if (!scopeSymbol) {
        logError(type, "Unknown scope");
        return typevisit_t::Failure;
    }
    if (!scopeSymbol->scopeDefined) {
        logError(type, "'{}' is not a namespace or module", type->scope->toString());
        return typevisit_t::Failure;
    }
    if (type->type->kind != ast::TypeKind::SymbolType) {
        logError(type->type, "Expected an identifier in a scope resolution expression");
        return typevisit_t::Failure;
    }
    const std::string_view memberName = static_cast<ast::SymbolType*>(type->type)->name;

    Symbol* memberSymbol = symbolTable.scopedLookup(scopeSymbol->scopeDefined, memberName);
    if (!memberSymbol) {
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

auto Analyzer::visit(ast::SymbolType* type) -> typevisit_t {
    const auto* symbolType = static_cast<const ast::SymbolType*>(type);
    if (symbolType->primitiveType != ast::PrimitiveType_t::not_primitive) {
        type->semanticType = typeContext.getPrimitive(symbolType->primitiveType);
        return typevisit_t::Success;
    }
    Symbol* symbol = symbolTable.lookup(symbolType->name);
    if (!symbol) {
        logError(type, "Unknown type '{}'", symbolType->name);
        return typevisit_t::Failure;
    }
    if (symbol->kind == SymbolKind::TypeAlias && symbol->status != ResolutionStatus::Success) {
        auto* aliasStatement = static_cast<ast::AliasStatement*>(symbol->node);
        if (visit(aliasStatement) == typevisit_t::Failure) { return typevisit_t::Failure; }
    }
    if (!symbol->type) {
        logError(type, "'{}' is not a valid type", symbolType->name);
        return typevisit_t::Failure;
    }
    type->semanticType = symbol->type;
    return typevisit_t::Success;
}

auto Analyzer::visit(ast::TypeofType* type) -> typevisit_t {
    const auto* typeofType = static_cast<const ast::TypeofType*>(type);
    if (visit(typeofType->expression) == typevisit_t::Failure) { return typevisit_t::Failure; }
    type->semanticType = typeofType->expression->semanticType;
    return typevisit_t::Success;
}

}  // namespace Manganese::semantic
