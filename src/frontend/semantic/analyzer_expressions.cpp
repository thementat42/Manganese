#include <algorithm>
#include <core.hpp>
#include <frontend/ast.hpp>
#include <frontend/lexer/token_base.hpp>
#include <frontend/lexer/token_type.hpp>
#include <frontend/semantic/analyzer.hpp>
#include <frontend/semantic/symbol_table.hpp>
#include <frontend/semantic/type_context.hpp>
#include <mnstl/number.hxx>
#include <string_view>
#include <utility>
#include <utils/result.hpp>
#include <vector>

namespace Manganese::semantic {

auto analyzer::instantiateGenericAggregateIfNeeded(ast::Expression* baseExpr) -> exprvisit_t {
    ast::Expression* targetExpression = unwrapBaseDeclaration(baseExpr);
    if (!targetExpression || targetExpression->kind != ast::ExpressionKind::IdentifierExpression) {
        return exprvisit_t::Success;
    }

    auto* identifier = static_cast<ast::IdentifierExpression*>(targetExpression);
    const Symbol* sym = symbolTable.lookup(identifier->value);
    if (!sym || !sym->node) { return exprvisit_t::Success; }

    if (sym->kind != SymbolKind::Aggregate && sym->kind != SymbolKind::GenericType) { return exprvisit_t::Success; }

    auto* aggregate = static_cast<ast::AggregateDeclarationStatement*>(sym->node);
    if (aggregate->genericTypes.empty()) { return exprvisit_t::Success; }

    // Invoke generic overload to instantiate and type-check the generic aggregate
    if (visit(aggregate, generic_tag) == stmtvisit_t::Failure) { return exprvisit_t::Failure; }

    return exprvisit_t::Success;
}

auto analyzer::visit(ast::AggregateInstantiationExpression* expression) -> exprvisit_t {
    auto result = exprvisit_t::Success;

    if (visit(expression->base) == exprvisit_t::Failure) { return exprvisit_t::Failure; }

    if (instantiateGenericAggregateIfNeeded(expression->base) == exprvisit_t::Failure) { return exprvisit_t::Failure; }

    const SemanticType* baseType = expression->base->semanticType;
    if (!baseType) {
        logError(expression->base, "Could not determine type of base expression in aggregate instantiation.");
        return exprvisit_t::Failure;
    }

    if (!baseType->isAggregate()) {
        logError(expression->base, "Type '{}' is not an aggregate type", baseType->toString());
        return exprvisit_t::Failure;
    }
    const auto* targetType = static_cast<const Aggregate*>(baseType);
    expression->semanticType = targetType;

    for (ast::AggregateInstantiationField& field : expression->fields) {
        if (visit(field.value) == exprvisit_t::Failure) { result = exprvisit_t::Failure; }
    }

    if (result == exprvisit_t::Failure) { return result; }

    for (const ast::AggregateInstantiationField& field : expression->fields) {
        if (!field.value->semanticType) {
            result = exprvisit_t::Failure;
            continue;
        }
        const SemanticType* expectedFieldType = targetType->getFieldType(field.name);
        if (!expectedFieldType) {
            logError(field.value, "Type '{}' has no field named '{}'", targetType->toString(), field.name);
            result = exprvisit_t::Failure;
            continue;
        }
        const typeCompatibilityResult compatibility = areTypesCompatible(expectedFieldType, field.value->semanticType);
        if (!compatibility) {
            logError(field.value, "Cannot initialize field '{}' of type '{}' with value of type '{}'", field.name,
                     expectedFieldType->toString(), field.value->semanticType->toString());
            result = exprvisit_t::Failure;
        } else if (compatibility.result == Compatible_t::Warning) {
            logWarning(field.value, "{}", compatibility.message);
        }
    }

    if (expression->fields.size() < targetType->fields.size()) {
        for (const AggregateField& declaredField : targetType->fields) {
            if (std::ranges::find(expression->fields, declaredField.name, &ast::AggregateInstantiationField::name)
                == expression->fields.end()) {
                logError(expression, "Missing field '{}' in instantiation of aggregate '{}'", declaredField.name,
                         targetType->toString());
                result = exprvisit_t::Failure;
            }
        }
    }
    return result;
}

auto analyzer::visit(ast::AggregateLiteralExpression* expression) -> exprvisit_t {
    auto result = exprvisit_t::Success;
    std::vector<const SemanticType*> elementTypes;
    elementTypes.reserve(expression->elements.size());

    for (ast::Expression* element : expression->elements) {
        if (visit(element) == exprvisit_t::Failure) { result = exprvisit_t::Failure; }
        if (!element->semanticType) {
            result = exprvisit_t::Failure;
        } else {
            elementTypes.push_back(element->semanticType);
        }
    }

    // If sub expressions failed, bail early so getting a type doesn't fail
    if (result == exprvisit_t::Failure) { return exprvisit_t::Failure; }

    expression->semanticType = typeContext.getAnonymousAggregate(std::move(elementTypes));

    return result;
}

auto analyzer::visit(ast::AlignofExpression* expression) -> exprvisit_t {
    if (visit(expression->type) == exprvisit_t::Failure) { return exprvisit_t::Failure; }
    const SemanticType* targetSemanticType = expression->type->semanticType;
    if (!targetSemanticType) {
        logError(expression, "Invalid type in alignof expression {}", expression->toString());
        return exprvisit_t::Failure;
    }
    expression->semanticType = typeContext.getSizeType();
    return exprvisit_t::Success;
}

auto analyzer::visit(ast::ArrayLiteralExpression* expression) -> exprvisit_t {
    if (expression->elements.empty()) {
        if (context.currentVariableDeclarationType && context.currentVariableDeclarationType->isArray()) {
            expression->semanticType = context.currentVariableDeclarationType;
            return exprvisit_t::Success;
        }
        logError(expression, "Cannot infer element type for empty array literal without type annotation");
    }

    auto result = exprvisit_t::Success;
    const SemanticType* expectedElementType = nullptr;
    if (context.currentVariableDeclarationType && context.currentVariableDeclarationType->isArray()) {
        expectedElementType = static_cast<const Array*>(context.currentVariableDeclarationType)->elementType;
    }

    const SemanticType* synthesizedElementType = nullptr;

    for (ast::Expression* element : expression->elements) {
        if (visit(element) == exprvisit_t::Failure) {
            result = exprvisit_t::Failure;
            continue;
        }
        if (!element->semanticType) {
            result = exprvisit_t::Failure;
            continue;
        }
        // top-down type deduction
        if (expectedElementType) {
            const auto canConvertElementType = areTypesCompatible(expectedElementType, element->semanticType);
            if (!canConvertElementType) {
                logError(element, "Array element of type '{}' is not compatible with expected array element type '{}'",
                         element->semanticType->toString(), expectedElementType->toString());
                result = exprvisit_t::Failure;
            } else if (canConvertElementType.result == Compatible_t::Warning) {
                logWarning(element, "{}", canConvertElementType.message);
            }
            continue;
        }
        // use the first element to set the type
        if (!synthesizedElementType) {
            synthesizedElementType = element->semanticType;
            continue;
        }
        const auto canConvertElementType = areTypesCompatible(synthesizedElementType, element->semanticType);
        if (!canConvertElementType) {
            logError(element, "Mismatched element types in array literal: expected '{}', got '{}'",
                     synthesizedElementType->toString(), element->semanticType->toString());
            result = exprvisit_t::Failure;
        } else if (canConvertElementType.result == Compatible_t::Warning) {
            logWarning(element, "{}", canConvertElementType.message);
        }
    }
    if (result == exprvisit_t::Failure) { return exprvisit_t::Failure; }

    const SemanticType* elementType = expectedElementType ? expectedElementType : synthesizedElementType;
    expression->semanticType = typeContext.getArray(elementType, expression->elements.size());
    return exprvisit_t::Success;
}

auto analyzer::visit(ast::AssignmentExpression* expression) -> exprvisit_t {
    auto result = exprvisit_t::Success;
    if (visit(expression->assignee) == exprvisit_t::Failure) { result = exprvisit_t::Failure; }
    if (visit(expression->value) == exprvisit_t::Failure) { result = exprvisit_t::Failure; }

    if (!expression->assignee->semanticType || !expression->value->semanticType) { return exprvisit_t::Failure; }

    // TODO: Check that the LHS can actually be assigned to

    const typeCompatibilityResult isAssignmentValid
        = areTypesCompatible(expression->assignee->semanticType, expression->value->semanticType);
    if (!isAssignmentValid) {
        logError(expression, "Cannot assign a value of type {} to a value of type {}",
                 expression->value->semanticType->toString(), expression->assignee->semanticType->toString());
        result = exprvisit_t::Failure;
    } else if (isAssignmentValid.result == Compatible_t::Warning) {
        logWarning(expression, "{}", isAssignmentValid.message);
    }
    expression->semanticType = expression->assignee->semanticType;
    return result;
}

auto analyzer::visit(ast::BinaryExpression* expression) -> exprvisit_t {
    auto result = exprvisit_t::Success;

    if (visit(expression->left) == exprvisit_t::Failure) { result = exprvisit_t::Failure; }
    if (visit(expression->right) == exprvisit_t::Failure) { result = exprvisit_t::Failure; }
    if (!expression->left->semanticType) {
        logError(expression, "Could not deduce type of expression {}", expression->left->toString());
        return exprvisit_t::Failure;
    }
    if (!expression->right->semanticType) {
        logError(expression, "Could not deduce type of expression {}", expression->right->toString());
        return exprvisit_t::Failure;
    }

    const SemanticType* lhsType = expression->left->semanticType;
    const SemanticType* rhsType = expression->right->semanticType;
    const lexer::TokenType op = expression->op;

    if (isLogicalOp(op)) {
        if (!lhsType->isBoolean() || !rhsType->isBoolean()) {
            logError(expression, "Operator '{}' requires boolean operands, got {} and {}", lexer::tokenTypeToString(op),
                     lhsType->toString(), rhsType->toString());
            result = exprvisit_t::Failure;
        }
        expression->semanticType = typeContext.getPrimitive(ast::PrimitiveType_t::boolean);
        return result;
    } else if (isBitwiseOp(op)) {
        if (!isInteger(lhsType->primitiveType) || !isInteger(lhsType->primitiveType)) {
            logError(expression, "Bitwise operators require integer operands, got {} and {}", lhsType->toString(),
                     rhsType->toString());
            result = exprvisit_t::Failure;
        }
        expression->semanticType = promoteNumericTypes(lhsType, rhsType);
        return result;
    } else if (isRelationalOp(op)) {
        if (!areTypesComparable(lhsType, rhsType)) {
            logError(expression, "Cannot compare incompatible types {} and {}", lhsType->toString(),
                     rhsType->toString());
            result = exprvisit_t::Failure;
        }
        expression->semanticType = typeContext.getPrimitive(ast::PrimitiveType_t::boolean);
        return result;
    } else if (isArithmeticOp(op)) {
        if (lhsType->isPointer() || rhsType->isPointer()) { return analyzePointerArithmetic(lhsType, rhsType); }
        const SemanticType* commonType = promoteNumericTypes(lhsType, rhsType);
        if (!commonType) {
            logError(expression, "Invalid operands for arithmetic operator '{}': {} and {}",
                     lexer::tokenTypeToString(op), lhsType->toString(), rhsType->toString());
            return exprvisit_t::Failure;
        }

        expression->semanticType = commonType;
        return result;
    }
    ASSERT_UNREACHABLE(
        std::format("Unhandled binary operator {} in visit(BinaryExpression)", lexer::tokenTypeToString(op)));
};

auto analyzer::visit(ast::BoolLiteralExpression* expression) -> exprvisit_t {
    expression->semanticType = typeContext.getPrimitive(ast::PrimitiveType_t::boolean);
    return exprvisit_t::Success;
}
auto analyzer::visit(ast::CharLiteralExpression* expression) -> exprvisit_t {
    expression->semanticType = typeContext.getPrimitive(ast::PrimitiveType_t::character);
    return exprvisit_t::Success;
}

auto analyzer::visit([[maybe_unused]] ast::FunctionCallExpression* expression) -> exprvisit_t {
    return exprvisit_t::Success;
}

auto analyzer::visit([[maybe_unused]] ast::GenericExpression* expression) -> exprvisit_t {
    return exprvisit_t::Success;
}

auto analyzer::visit(ast::IdentifierExpression* expression) -> exprvisit_t {
    const Symbol* symbol = symbolTable.lookup(expression->value);
    if (!symbol) {
        logError(expression, "Identifier '{}' was not found in the current scope", expression->value);
        return exprvisit_t::Failure;
    }
    if (!symbol->type) [[unlikely]] {
        logError(expression, "Identifier '{}' used before its type could be determined", expression->value);
        return exprvisit_t::Failure;
    }
    expression->semanticType = symbol->type;
    return exprvisit_t::Success;
}

auto analyzer::visit(ast::IndexExpression* expression) -> exprvisit_t {
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

    if (!isInteger(expression->index->semanticType->primitiveType)) {
        logError(expression, "Index value should be an integer, not {}", expression->index->semanticType->toString());
        result = exprvisit_t::Failure;
    }

    expression->semanticType = static_cast<const Array*>(expression->variable->semanticType)->elementType;

    return result;
}

auto analyzer::visit(ast::MemberAccessExpression* expression) -> exprvisit_t {
    visit(expression->object);
    const SemanticType* objectType = expression->object->semanticType;
    if (!objectType) {
        logError(expression->object, "Could not deduce type of expression {}", expression->object->toString());
        return exprvisit_t::Failure;
    }
    // accessing a mamber from a pointer to an aggregate is also done using x.y so auto-dereference if that's the case
    if (objectType->isPointer()) { objectType = static_cast<const Pointer*>(objectType)->baseType; }

    if (!objectType->isAggregate()) {
        logError(expression->object,
                 "Element access can only be performed on an aggregate type (ora  pointer to an aggregate) not '{}'",
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

auto analyzer::visit(ast::NumberLiteralExpression* expression) -> exprvisit_t {
    using held_t = mnstl::number_t::held_type;
    using prim_t = ast::PrimitiveType_t;
    switch (expression->value.underlying_type()) {
        case held_t::int8: expression->semanticType = typeContext.getPrimitive(prim_t::i8); break;
        case held_t::int16: expression->semanticType = typeContext.getPrimitive(prim_t::i16); break;
        case held_t::int32: expression->semanticType = typeContext.getPrimitive(prim_t::i32); break;
        case held_t::int64: expression->semanticType = typeContext.getPrimitive(prim_t::i64); break;
        case held_t::int128: expression->semanticType = typeContext.getPrimitive(prim_t::i128); break;
        case held_t::uint8: expression->semanticType = typeContext.getPrimitive(prim_t::u8); break;
        case held_t::uint16: expression->semanticType = typeContext.getPrimitive(prim_t::u16); break;
        case held_t::uint32: expression->semanticType = typeContext.getPrimitive(prim_t::u32); break;
        case held_t::uint64: expression->semanticType = typeContext.getPrimitive(prim_t::u64); break;
        case held_t::uint128: expression->semanticType = typeContext.getPrimitive(prim_t::u128); break;
        case held_t::float32: expression->semanticType = typeContext.getPrimitive(prim_t::f32); break;
        case held_t::float64: expression->semanticType = typeContext.getPrimitive(prim_t::f64); break;
        case held_t::error: {
            logError(expression, "{}", expression->value.error_unchecked());
            expression->semanticType = nullptr;
            return exprvisit_t::Failure;
        }
        case held_t::none: [[fallthrough]];
        default: ASSERT_UNREACHABLE("In analyzer: Number literal expression had no parser-deduced type");
    }
    return exprvisit_t::Success;
}

auto analyzer::visit(ast::PostfixExpression* expression) -> exprvisit_t {
    auto result = visit(expression->left);
    if (result == exprvisit_t::Failure) { return result; }
    // the only postfix operators are ++ and -- so the expression must be an integer
    if (!expression->left->semanticType) {
        logError(expression, "Could not deduce type of expression {}", expression->toString());
        return exprvisit_t::Failure;
    }
    const ast::PrimitiveType_t primitiveType = expression->left->semanticType->primitiveType;

    // set the type here even if the expression is invalid so we don't have a bunch of propagating nulls
    //? should this implicitly promote? (probably not)
    expression->semanticType = typeContext.getPrimitive(primitiveType);

    if (!isInteger(primitiveType)) {
        logError(expression, "operator {} can only be applied to integer types",
                 lexer::tokenTypeToString(expression->op));
        return exprvisit_t::Failure;
    }
    // TODO: check that the value has an address to store the inc/dec result

    return exprvisit_t::Success;
}

auto analyzer::visit(ast::PrefixExpression* expression) -> exprvisit_t {
    auto result = visit(expression->right);
    if (result == exprvisit_t::Failure) { return result; }
    if (!expression->right->semanticType) {
        logError(expression, "Could not deduce type of expression {}", expression->toString());
        return exprvisit_t::Failure;
    }

    const ast::PrimitiveType_t primitiveType = expression->right->semanticType->primitiveType;

    using enum lexer::TokenType;
    switch (expression->op) {
        case Inc:
        case Dec: {
            expression->semanticType = typeContext.getPrimitive(primitiveType);
            if (!isInteger(primitiveType)) {
                logError(expression, "operator {} can only be applied to integer types",
                         lexer::tokenTypeToString(expression->op));
                return exprvisit_t::Failure;
            }
            // TODO: check that the value has an address to store the inc/dec result
        } break;

        case BitNot: {
            expression->semanticType = typeContext.getPrimitive(primitiveType);
            if (!isInteger(primitiveType)) {
                logError(expression, "operator {} can only be applied to integer types",
                         lexer::tokenTypeToString(expression->op));
                return exprvisit_t::Failure;
            }
        } break;

        case UnaryPlus:
        case UnaryMinus: {
            if (visit(expression->right) == exprvisit_t::Failure) { return exprvisit_t::Failure; }
            const SemanticType* opType = expression->right->semanticType;
            if (!opType) { return exprvisit_t::Failure; }
            if (!isNumeric(opType->primitiveType)) {
                logError(expression, "Operator '{}' can only be applied to numeric types (got '{}')",
                         lexer::tokenTypeToString(expression->op), opType->toString());
                return exprvisit_t::Failure;
            }
            if (expression->op == UnaryMinus && isUnsignedInteger(opType->primitiveType)) {
                logWarning(expression, "Applying a '-' to an unsigned integer type  ('{}') causes wrapping",
                           opType->toString());
            }
            expression->semanticType = opType;
        } break;

        case AddressOf: {
            // TODO: Check that the expression has an address that can be taken
            // TODO: is there some way to determine mutability of pointer?
            expression->semanticType = typeContext.getPointer(expression->right->semanticType, true);
        } break;

        case Dereference: {
            if (!expression->right->semanticType->isPointer()) {
                logError(expression, "Dereferencing cannot be applied to a non-pointer type");
                // dummy (figure out a better option later)
                expression->semanticType = typeContext.getPrimitive(ast::PrimitiveType_t::u8);
                return exprvisit_t::Failure;
            }
            expression->semanticType = static_cast<const Pointer*>(expression->right->semanticType)->baseType;
        } break;

        default:
            ASSERT_UNREACHABLE(std::format("Unknown prefix operator {}", lexer::tokenTypeToString(expression->op)));
    }
    return exprvisit_t::Success;
}

auto analyzer::visit([[maybe_unused]] ast::ScopeResolutionExpression* expression) -> exprvisit_t {
    return exprvisit_t::Success;
}

auto analyzer::visit(ast::SizeofExpression* expression) -> exprvisit_t {
    if (visit(expression->type) == exprvisit_t::Failure) { return exprvisit_t::Failure; }
    const SemanticType* targetSemanticType = expression->type->semanticType;
    if (!targetSemanticType) {
        logError(expression, "Invalid type in sizeof expression {}", expression->toString());
        return exprvisit_t::Failure;
    }
    expression->semanticType = typeContext.getSizeType();
    return exprvisit_t::Success;
}

auto analyzer::visit(ast::StringLiteralExpression* expression) -> exprvisit_t {
    expression->semanticType = typeContext.getPrimitive(ast::PrimitiveType_t::str);
    return exprvisit_t::Success;
}

auto analyzer::visit(ast::TypeCastExpression* expression) -> exprvisit_t {
    auto result = exprvisit_t::Success;
    ContextGuard guard(context.typeCastDepth, static_cast<decltype(context.typeCastDepth)>(context.typeCastDepth + 1));
    if (visit(expression->originalValue) == exprvisit_t::Failure) { result = exprvisit_t::Failure; }
    if (!expression->originalValue->semanticType) {
        logError(expression, "Could not deduce type of expression {}", expression->originalValue->toString());
        return exprvisit_t::Failure;
    }
    if (visit(expression->targetType) == exprvisit_t::Failure) { result = exprvisit_t::Failure; }

    return result;
}

}  // namespace Manganese::semantic