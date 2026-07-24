#include <core.hpp>
#include <frontend/ast.hpp>
#include <frontend/semantic.hpp>

namespace Manganese {
namespace semantic {

auto analyzer::visit(ast::AggregateType* type) -> typevisit_t {
    const ast::AggregateType* aggregateType = static_cast<const ast::AggregateType*>(type);
    std::vector<const SemanticType*> resolvedFields;
    resolvedFields.reserve(aggregateType->fieldTypes.size());

    for (ast::Type* fieldType : aggregateType->fieldTypes) {
        visit(fieldType);
        const SemanticType* resolvedFieldType = fieldType->semanticType;
        if (!resolvedFieldType) { return Result::Failure; }
        resolvedFields.push_back(resolvedFieldType);
    }
    type->semanticType = typeContext.getAnonymousAggregate(std::move(resolvedFields));
    return Result::Success;
}

auto analyzer::visit(ast::ArrayType* type) -> typevisit_t {
    const ast::ArrayType* arrayType = static_cast<const ast::ArrayType*>(type);

    // for nested arrays
    const SemanticType* outerVarType = context.currentVariableDeclarationType;
    context.currentVariableDeclarationType = nullptr;
    visit(arrayType->elementType);
    context.currentVariableDeclarationType = outerVarType;

    const SemanticType* elementType = arrayType->elementType->semanticType;

    if (!elementType) {
        logError(type, "Cannot form array of invalid type '{}'", arrayType->elementType->toString());
        return Result::Failure;
    }
    size_t length;
    if (arrayType->lengthExpression) {
        if (visit(arrayType->lengthExpression) == Result::Failure) { return Result::Failure; }
        mnstl::fold_result_t fold = arrayType->lengthExpression->fold();
        if (!fold.is_number()) {
            logError(arrayType->lengthExpression, "Array length ({}) must be a constant expression",
                     arrayType->lengthExpression->toString());
            return Result::Failure;
        }
        const mnstl::number_t lengthValue = fold.number_unchecked();
        if (lengthValue.is_error()) {
            logError(arrayType->lengthExpression, "{}", lengthValue.error_unchecked());
            return Result::Failure;
        }
        if (!lengthValue.is_integer()) {
            logError(arrayType->lengthExpression, "Array length must be an integer value");
            return Result::Failure;
        }
        if (lengthValue <= 0) {
            logError(arrayType->lengthExpression, "Array length must be greater than 0 (got {})",
                     lengthValue.to_string());
            return Result::Failure;
        }
        length = lengthValue.value_as<size_t>();
    } else if (context.currentVariableDeclarationType && context.currentVariableDeclarationType->isArray()) {
        length = static_cast<const Array*>(context.currentVariableDeclarationType)->length;
    } else [[unlikely]] {
        logError(type, "Cannot infer array length; explicitly specify length or provide an initializer");
        return Result::Failure;
    }

    type->semanticType = typeContext.getArray(elementType, length);
    return Result::Success;
}
auto analyzer::visit(ast::FunctionType* type) -> typevisit_t {
    const ast::FunctionType* functionType = static_cast<const ast::FunctionType*>(type);
    std::vector<Parameter> resolvedParameterTypes;
    for (const ast::FunctionParameterType& parameterType : functionType->parameterTypes) {
        visit(parameterType.type);
        const SemanticType* resolvedParameterType = parameterType.type->semanticType;
        if (!resolvedParameterType) { return Result::Failure; }

        resolvedParameterTypes.push_back({.isMutable = parameterType.isMutable, .type = resolvedParameterType});
    }
    const SemanticType* returnType = nullptr;
    if (functionType->returnType) {
        // function is not returning void
        visit(functionType->returnType);
        returnType = functionType->returnType->semanticType;
        if (!returnType) { return Result::Failure; }
    }
    type->semanticType = typeContext.getFunction(std::move(resolvedParameterTypes), returnType);
    return Result::Success;
}
auto analyzer::visit(ast::GenericType* type) -> typevisit_t {
    const ast::GenericType* genericType = static_cast<const ast::GenericType*>(type);
    visit(genericType->baseType);
    const SemanticType* baseType = genericType->baseType->semanticType;
    if (!baseType) { return Result::Failure; }
    std::vector<const SemanticType*> resolvedTypeParameters;
    resolvedTypeParameters.reserve(genericType->typeParameters.size());

    for (ast::Type* typeParameter : genericType->typeParameters) {
        visit(typeParameter);
        const SemanticType* resolvedTypeParameter = typeParameter->semanticType;
        if (!resolvedTypeParameter) { return Result::Failure; }
        resolvedTypeParameters.push_back(resolvedTypeParameter);
    }
    type->semanticType = typeContext.getGenericInstance(baseType, std::move(resolvedTypeParameters));
    return Result::Success;
}
auto analyzer::visit(ast::PointerType* type) -> typevisit_t {
    const ast::PointerType* pointerType = static_cast<const ast::PointerType*>(type);
    visit(pointerType->baseType);
    const SemanticType* baseType = pointerType->baseType->semanticType;
    if (!baseType) {
        logError(type, "Cannot form pointer to invalid type '{}'", pointerType->baseType->toString());
        return Result::Failure;
    }
    type->semanticType = typeContext.getPointer(baseType, pointerType->isMutable);
    return Result::Success;
}
auto analyzer::visit(ast::SymbolType* type) -> typevisit_t {
    const ast::SymbolType* symbolType = static_cast<const ast::SymbolType*>(type);
    if (symbolType->primitiveType != ast::PrimitiveType_t::not_primitive) {
        type->semanticType = typeContext.getPrimitive(symbolType->primitiveType);
        return Result::Failure;
    }
    const Symbol* symbol = symbolTable.lookup(symbolType->name);
    if (!symbol) {
        logError(type, "Unknown type '{}'", symbolType->name);
        return Result::Failure;
    }
    if (!symbol->type) {
        logError(type, "'{}' is not a valid type", symbolType->name);
        return Result::Failure;
    }
    type->semanticType = symbol->type;
    return Result::Success;
}

auto analyzer::visit(ast::TypeofType* type) -> typevisit_t {
    const ast::TypeofType* typeofType = static_cast<const ast::TypeofType*>(type);
    if (visit(typeofType->expression) == Result::Failure) { return Result::Failure; }
    type->semanticType = typeofType->expression->semanticType;
    return Result::Success;
}

}  // namespace semantic
}  // namespace Manganese
