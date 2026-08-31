#include <core.hpp>
#include <frontend/ast.hpp>
#include <frontend/lexer/token.hpp>
#include <frontend/semantic/analyzer.hpp>
#include <frontend/semantic/symbol_table.hpp>
#include <frontend/semantic/type_context.hpp>
#include <mnstl/number.hxx>
#include <string_view>
#include <utility>
#include <utils/result.hpp>
#include <vector>

namespace Manganese::semantic {

auto Analyzer::visit(ast::AggregateInstantiationExpression* expression) -> exprvisit_t {
    if (visit(expression->base) == exprvisit_t::Failure) { return exprvisit_t::Failure; }

    const SemanticType* baseType = expression->base->semanticType;
    if (!baseType) {
        logError(expression->base, "Cannot instantiate unresolvable aggregate type");
        return exprvisit_t::Failure;
    }

    if (!baseType->isAggregate()) {
        logError(expression->base, "Type '{}' is not an aggregate type", baseType->toString());
        return exprvisit_t::Failure;
    }

    const auto* aggregateType = static_cast<const Aggregate*>(baseType);
    std::unordered_set<std::string_view> initializedFields;
    initializedFields.reserve(expression->fields.size());

    exprvisit_t result = exprvisit_t::Success;
    for (auto& fieldInit : expression->fields) {
        // Visit field value expression
        if (visit(fieldInit.value) == exprvisit_t::Failure) {
            result = exprvisit_t::Failure;
            continue;
        }

        if (fieldInit.value->semanticType->isVoid()) {
            logError(fieldInit.value, "Cannot use 'void' expression in aggregate instantiation");
            result = exprvisit_t::Failure;
            continue;
        }

        if (!initializedFields.insert(fieldInit.name).second) {
            logError(expression, "Duplicate initialization of field '{}'", fieldInit.name);
            result = exprvisit_t::Failure;
            continue;
        }

        const SemanticType* expectedFieldType = aggregateType->getFieldType(fieldInit.name);
        if (!expectedFieldType) {
            logError(expression, "Aggregate '{}' has no field named '{}'", aggregateType->toString(), fieldInit.name);
            result = exprvisit_t::Failure;
            continue;
        }

        const SemanticType* actualFieldType = fieldInit.value->semanticType;
        if (!areTypesCompatible(expectedFieldType, actualFieldType)) {
            logError(fieldInit.value, "Type mismatch for field '{}': expected '{}', got '{}'", fieldInit.name,
                     expectedFieldType->toString(), actualFieldType->toString());
            result = exprvisit_t::Failure;
        }
    }

    if (result == exprvisit_t::Success && initializedFields.size() < aggregateType->fields.size()) {
        for (const auto& declaredField : aggregateType->fields) {
            if (!initializedFields.contains(declaredField.name)) {
                logError(expression, "Missing initialization for field '{}' of aggregate '{}'", declaredField.name,
                         aggregateType->toString());
                result = exprvisit_t::Failure;
            }
        }
    }

    expression->semanticType = aggregateType;
    return result;
}

auto Analyzer::visit(ast::FunctionCallExpression* expression) -> exprvisit_t {
    if (visit(expression->callee) == exprvisit_t::Failure) { return exprvisit_t::Failure; }
    const SemanticType* calleeType = expression->callee->semanticType;
    if (!calleeType) {
        logError(expression->callee, "Cannot call expression with unresolvable type");
        return exprvisit_t::Failure;
    }

    if (!calleeType->isFunction()) {
        logError(expression->callee, "Expression of type '{}' is not callable", calleeType->toString());
        return exprvisit_t::Failure;
    }

    const auto* functionType = static_cast<const Function*>(calleeType);

    if (expression->arguments.size() != functionType->parameterTypes.size()) {
        logError(expression, "Function expected {} arguments, but got {}", functionType->parameterTypes.size(),
                 expression->arguments.size());
        return exprvisit_t::Failure;
    }

    exprvisit_t result = exprvisit_t::Success;
    for (std::size_t i = 0; i < expression->arguments.size(); ++i) {
        ast::Expression* argExpr = expression->arguments[i];
        if (visit(argExpr) == exprvisit_t::Failure) {
            result = exprvisit_t::Failure;
            continue;
        }

        const SemanticType* expectedType = functionType->parameterTypes[i].type;
        const SemanticType* actualType = argExpr->semanticType;

        if (actualType && actualType->isVoid()) {
            logError(argExpr, "Cannot pass expression returning 'void' as argument {} to function", i + 1);
            result = exprvisit_t::Failure;
            continue;
        }

        if (!areTypesCompatible(expectedType, actualType)) {
            logError(argExpr, "Type mismatch for argument {}: expected '{}', got '{}'", i + 1, expectedType->toString(),
                     actualType->toString());
            result = exprvisit_t::Failure;
        }
    }

    expression->semanticType = functionType->returnType;
    return result;
}

auto Analyzer::visit(ast::IndexExpression* expression) -> exprvisit_t {
    auto result = exprvisit_t::Success;
    if (visit(expression->variable) == exprvisit_t::Failure) { result = exprvisit_t::Failure; }
    if (visit(expression->index) == exprvisit_t::Failure) { result = exprvisit_t::Failure; }

    if (!expression->variable->semanticType) {
        logError(expression->variable, "Could not deduce type of expression {}", expression->variable->toString());
        return exprvisit_t::Failure;
    }
    if (!expression->index->semanticType) {
        logError(expression->index, "Could not deduce type of expression {}", expression->index->toString());
        return exprvisit_t::Failure;
    }

    if (!expression->variable->semanticType->isArray()) {
        logError(expression->variable, "Cannot index into non array type {}",
                 expression->variable->semanticType->toString());
        return exprvisit_t::Failure;
    }

    if (!expression->index->semanticType->isInteger()) {
        logError(expression, "Index value should be an integer, not {}", expression->index->semanticType->toString());
        result = exprvisit_t::Failure;
    }

    expression->semanticType = static_cast<const Array*>(expression->variable->semanticType)->elementType;

    return result;
}

auto Analyzer::visit(ast::MemberAccessExpression* expression) -> exprvisit_t {
    DISCARD(visit(expression->object));
    const SemanticType* objectType = expression->object->semanticType;
    if (!objectType) {
        logError(expression->object, "Could not deduce type of expression {}", expression->object->toString());
        return exprvisit_t::Failure;
    }
    // accessing a mamber from a pointer to an aggregate is also done using x.y so auto-dereference if that's the case
    if (objectType->isPointer()) { objectType = static_cast<const Pointer*>(objectType)->baseType; }

    if (!objectType->isAggregate()) {
        logError(expression->object,
                 "Element access can only be performed on an aggregate type (or a pointer to an aggregate) not '{}'",
                 objectType->toString());
        return exprvisit_t::Failure;
    }

    const auto* aggregateType = static_cast<const Aggregate*>(objectType);

    for (const AggregateField& field : aggregateType->fields) {
        if (field.name == expression->property) {
            expression->semanticType = field.type;
            return exprvisit_t::Success;
        }
    }
    logError(expression, "Aggregate type '{}' has no field named '{}'", aggregateType->toString(),
             expression->property);

    return exprvisit_t::Failure;
}

auto Analyzer::visit(ast::ScopeResolutionExpression* expression) -> exprvisit_t {
    const Symbol* scopeSymbol = nullptr;

    if (expression->scope->kind == ast::ExpressionKind::IdentifierExpression) {
        // regular identifier (e.g. Foo::Bar)
        scopeSymbol = symbolTable.lookup(static_cast<ast::IdentifierExpression*>(expression->scope)->name);
    } else if (expression->scope->kind == ast::ExpressionKind::ScopeResolutionExpression) {
        // chained resolution (e.g. Foo::Bar::Baz): recursively resolve it
        if (visit(expression->scope) == exprvisit_t::Failure) { return exprvisit_t::Failure; }
        scopeSymbol = context.nestedScopeResolutionCurrentSymbol;
    }

    if (!scopeSymbol) {
        logError(expression, "Unknown scope");
        return exprvisit_t::Failure;
    }

    if (expression->element->kind != ast::ExpressionKind::IdentifierExpression) {
        logError(expression->element, "Expected an identifier in a scope resolution expression");
        return exprvisit_t::Failure;
    }
    const std::string_view memberName = static_cast<ast::IdentifierExpression*>(expression->element)->name;

    if (scopeSymbol->type && scopeSymbol->type->isEnum()) {
        const auto* enumType = static_cast<const Enum*>(scopeSymbol->type);
        for (const auto& variant : enumType->variants) {
            if (variant.name == memberName) {
                expression->semanticType = enumType;
                context.nestedScopeResolutionCurrentSymbol = scopeSymbol;
                return exprvisit_t::Success;
            }
        }
        logError(expression->element, "No variant named '{}' in enum '{}'", memberName, scopeSymbol->type->toString());
        return exprvisit_t::Failure;
    }

    if (!scopeSymbol->scopeDefined) {
        logError(expression, "'{}' is not a namespace, module or enum", scopeSymbol->node->toString());
        return exprvisit_t::Failure;
    }

    Symbol* memberSymbol = symbolTable.scopedLookup(scopeSymbol->scopeDefined, memberName);
    if (!memberSymbol) {
        logError(expression->element, "No member named '{}' in scope", memberName);
        return exprvisit_t::Failure;
    }
    expression->semanticType = memberSymbol->type;
    context.nestedScopeResolutionCurrentSymbol = memberSymbol;
    return exprvisit_t::Success;
}

}  // namespace Manganese::semantic