#include <core.hpp>
#include <frontend/ast.hpp>
#include <frontend/lexer/token_base.hpp>
#include <frontend/lexer/token_type.hpp>
#include <frontend/semantic/analyzer.hpp>
#include <frontend/semantic/symbol_table.hpp>
#include <frontend/semantic/type_context.hpp>
#include <io/logging.hpp>
#include <mnstl/number.hxx>
#include <utils/result.hpp>
#include <vector>

namespace Manganese {
namespace semantic {

auto analyzer::visit(ast::AggregateInstantiationExpression* expression) -> exprvisit_t {
    auto result = Result::Success;
    const Symbol* symbol = symbolTable.lookup(expression->name);
    if (!symbol) {
        logError(expression, "Type '{}' was not found in the current scope.", expression->name);
        return Result::Failure;
    }

    if (!symbol->type || symbol->kind != SymbolKind::Aggregate) {
        logError(expression, "Type '{}' is not an aggregate type", expression->name);
        return Result::Failure;
    }

    const auto* targetType = static_cast<const Aggregate*>(symbol->type);

    if (!expression->genericTypes.empty()) {
        std::vector<const SemanticType*> resolvedGenerics;
        resolvedGenerics.reserve(expression->genericTypes.size());

        for (auto* genericAstType : expression->genericTypes) {
            visit(genericAstType);
            const SemanticType* resolved = genericAstType->semanticType;
            if (!resolved) {
                result = Result::Failure;
            } else {
                resolvedGenerics.push_back(resolved);
            }
        }
        if (result == Result::Failure) { return result; }

        targetType
            = static_cast<const Aggregate*>(typeContext.getGenericInstance(targetType, std::move(resolvedGenerics)));
    }

    expression->semanticType = targetType;

    for (auto& field : expression->fields) {
        if (visit(field.value) == Result::Failure) { result = Result::Failure; }
        if (!field.value->semanticType) { result = Result::Failure; }
    }

    if (result == Result::Failure) { return result; }

    std::unordered_set<std::string_view> initializedFields;
    initializedFields.reserve(expression->fields.size());

    for (const auto& field : expression->fields) {
        // e.g., Point { x = 1, x = 2 }
        if (!initializedFields.insert(field.name).second) {
            logError(field.value, "Duplicate initialization for field '{}' in '{}'", field.name, expression->name);
            result = Result::Failure;
            continue;
        }

        // Can't instantiate an undeclared field
        const SemanticType* expectedFieldType = targetType->getFieldType(field.name);
        if (!expectedFieldType) {
            logError(field.value, "Type '{}' has no field named '{}'", expression->name, field.name);
            result = Result::Failure;
            continue;
        }

        // Check that that field can be instantiated
        const auto compatibility = areTypesCompatible(expectedFieldType, field.value->semanticType);
        if (!compatibility) {
            logError(field.value, "Cannot initialize field '{}' of type {} with value of type {}", field.name,
                     expectedFieldType->toString(), field.value->semanticType->toString());
            result = Result::Failure;
        } else if (compatibility.result == Compatible_t::Warning) {
            logWarning(field.value, "{}", compatibility.message);
        }
    }

    if (initializedFields.size() < targetType->fields.size()) {
        for (const auto& declaredField : targetType->fields) {
            if (!initializedFields.contains(declaredField.name)) {
                logError(expression, "Missing field '{}' in instantiation of '{}'", declaredField.name,
                         expression->name);
                result = Result::Failure;
            }
        }
    }

    return result;
}

auto analyzer::visit(ast::AggregateLiteralExpression* expression) -> exprvisit_t {
    auto result = Result::Success;
    std::vector<const SemanticType*> elementTypes;
    elementTypes.reserve(expression->elements.size());

    for (auto& element : expression->elements) {
        if (visit(element) == Result::Failure) { result = Result::Failure; }
        if (!element->semanticType) {
            result = Result::Failure;
        } else {
            elementTypes.push_back(element->semanticType);
        }
    }

    // If sub expressions failed, bail early so getting a type doesn't fail
    if (result == Result::Failure) { return Result::Failure; }

    expression->semanticType = typeContext.getAnonymousAggregate(std::move(elementTypes));

    return result;
}

// auto analyzer::visit(ast::ArrayLiteralExpression* expression) -> exprvisit_t;

auto analyzer::visit(ast::AssignmentExpression* expression) -> exprvisit_t {
    auto result = Result::Success;
    if (visit(expression->assignee) == Result::Failure) { result = Result::Failure; }
    if (visit(expression->value) == Result::Failure) { result = Result::Failure; }

    if (!expression->assignee->semanticType || !expression->value->semanticType) { return Result::Failure; }

    // TODO: Check that the LHS can actually be assigned to

    const auto isAssignmentValid
        = areTypesCompatible(expression->assignee->semanticType, expression->value->semanticType);
    if (!isAssignmentValid) {
        logError(expression, "Cannot assign a value of type {} to a value of type {}",
                 expression->value->semanticType->toString(), expression->assignee->semanticType->toString());
        result = Result::Failure;
    } else if (isAssignmentValid.result == Compatible_t::Warning) {
        logWarning(expression, "{}", isAssignmentValid.message);
    }
    expression->semanticType = expression->assignee->semanticType;
    return result;
}

auto analyzer::visit(ast::BinaryExpression* expression) -> exprvisit_t {
    auto result = Result::Success;

    if (visit(expression->left) == Result::Failure) { result = Result::Failure; }
    if (visit(expression->right) == Result::Failure) { result = Result::Failure; }
    if (!expression->left->semanticType) {
        logError(expression, "Could not deduce type of expression {}", expression->left->toString());
        return Result::Failure;
    }
    if (!expression->right->semanticType) {
        logError(expression, "Could not deduce type of expression {}", expression->right->toString());
        return Result::Failure;
    }

    const auto* lhsType = expression->left->semanticType;
    const auto* rhsType = expression->right->semanticType;
    const lexer::TokenType op = expression->op;

    if (isLogicalOp(op)) {
        if (!lhsType->isBoolean() || !rhsType->isBoolean()) {
            logError(expression, "Operator '{}' requires boolean operands, got {} and {}", lexer::tokenTypeToString(op),
                     lhsType->toString(), rhsType->toString());
            result = Result::Failure;
        }
        expression->semanticType = typeContext.getPrimitive(ast::PrimitiveType_t::boolean);
        return result;
    } else if (isBitwiseOp(op)) {
        if (!isInteger(lhsType->primitiveType) || !isInteger(lhsType->primitiveType)) {
            logError(expression, "Bitwise operators require integer operands, got {} and {}", lhsType->toString(),
                     rhsType->toString());
            result = Result::Failure;
        }
        expression->semanticType = promoteNumericTypes(lhsType, rhsType);
        return result;
    } else if (isRelationalOp(op)) {
        if (!areTypesComparable(lhsType, rhsType)) {
            logError(expression, "Cannot compare incompatible types {} and {}", lhsType->toString(),
                     rhsType->toString());
            result = Result::Failure;
        }
        expression->semanticType = typeContext.getPrimitive(ast::PrimitiveType_t::boolean);
        return result;
    } else if (isArithmeticOp(op)) {
        if (lhsType->isPointer() || rhsType->isPointer()) { return analyzePointerArithmetic(lhsType, rhsType); }
        const SemanticType* commonType = promoteNumericTypes(lhsType, rhsType);
        if (!commonType) {
            logError(expression, "Invalid operands for arithmetic operator '{}': {} and {}",
                     lexer::tokenTypeToString(op), lhsType->toString(), rhsType->toString());
            return Result::Failure;
        }

        expression->semanticType = commonType;
        return result;
    }
    ASSERT_UNREACHABLE(
        std::format("Unhandled binary operator {} in visit(BinaryExpression)", lexer::tokenTypeToString(op)));
};

auto analyzer::visit(ast::BoolLiteralExpression* expression) -> exprvisit_t {
    expression->semanticType = typeContext.getPrimitive(ast::PrimitiveType_t::boolean);
    return Result::Success;
}
auto analyzer::visit(ast::CharLiteralExpression* expression) -> exprvisit_t {
    expression->semanticType = typeContext.getPrimitive(ast::PrimitiveType_t::character);
    return Result::Success;
}

// auto analyzer::visit(ast::FunctionCallExpression* expression) -> exprvisit_t;
// auto analyzer::visit(ast::GenericExpression* expression) -> exprvisit_t;

auto analyzer::visit(ast::IdentifierExpression* expression) -> exprvisit_t {
    const Symbol* symbol = symbolTable.lookup(expression->value);
    if (!symbol) {
        logError(expression, "Identifier '{}' was not found in the current scope", expression->value);
        return Result::Failure;
    }
    if (!symbol->type) [[unlikely]] {
        logError(expression, "Identifier '{}' used before its type could be determined", expression->value);
        return Result::Failure;
    }
    expression->semanticType = symbol->type;
    return Result::Success;
}

auto analyzer::visit(ast::IndexExpression* expression) -> exprvisit_t {
    auto result = Result::Success;
    if (visit(expression->variable) == Result::Failure) { result = Result::Failure; }
    if (visit(expression->index) == Result::Failure) { result = Result::Failure; }

    if (!expression->variable->semanticType) {
        logError(expression->variable, "Could not deduce type of expression {}", expression->variable->toString());
        return Result::Failure;
    }
    if (!expression->index->semanticType) {
        logError(expression->index, "Could not deduce type of expression {}", expression->index->toString());
        return Result::Failure;
    }

    if (!expression->variable->semanticType->isArray()) {
        logError(expression->variable, "Cannot index into non array type {}",
                 expression->variable->semanticType->toString());
        return Result::Failure;
    }

    if (!isInteger(expression->index->semanticType->primitiveType)) {
        logError(expression, "Index value should be an integer, not {}", expression->index->semanticType->toString());
        result = Result::Failure;
    }

    expression->semanticType = static_cast<const Array*>(expression->variable->semanticType)->elementType;

    return result;
}

// auto analyzer::visit(ast::MemberAccessExpression* expression) -> exprvisit_t;

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
            return Result::Failure;
        }
        case held_t::none: [[fallthrough]];
        default: ASSERT_UNREACHABLE("In analyzer: Number literal expression had no parser-deduced type");
    }
    return Result::Success;
}

auto analyzer::visit(ast::PostfixExpression* expression) -> exprvisit_t {
    auto result = visit(expression->left);
    if (result == Result::Failure) { return result; }
    // the only postfix operators are ++ and -- so the expression must be an integer
    if (!expression->left->semanticType) {
        logError(expression, "Could not deduce type of expression {}", expression->toString());
        return Result::Failure;
    }
    const ast::PrimitiveType_t primitiveType = expression->left->semanticType->primitiveType;

    // set the type here even if the expression is invalid so we don't have a bunch of propagating nulls
    //? should this implicitly promote? (probably not)
    expression->semanticType = typeContext.getPrimitive(primitiveType);

    if (!isInteger(primitiveType)) {
        logError(expression, "operator {} can only be applied to integer types",
                 lexer::tokenTypeToString(expression->op));
        return Result::Failure;
    }
    // TODO: check that the value has an address to store the inc/dec result

    return Result::Success;
}

auto analyzer::visit(ast::PrefixExpression* expression) -> exprvisit_t {
    auto result = visit(expression->right);
    if (result == Result::Failure) { return result; }
    if (!expression->right->semanticType) {
        logError(expression, "Could not deduce type of expression {}", expression->toString());
        return Result::Failure;
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
                return Result::Failure;
            }
            // TODO: check that the value has an address to store the inc/dec result
        } break;

        case BitNot: {
            expression->semanticType = typeContext.getPrimitive(primitiveType);
            if (!isInteger(primitiveType)) {
                logError(expression, "operator {} can only be applied to integer types",
                         lexer::tokenTypeToString(expression->op));
                return Result::Failure;
            }
        } break;

        case UnaryPlus:
        case UnaryMinus: {
            expression->semanticType = typeContext.getPrimitive(primitiveType);
            if (!isNumeric(primitiveType)) {
                logError(expression, "operator {} can only be applied to integer or floating point types",
                         lexer::tokenTypeToString(expression->op));
                return Result::Failure;
            }
            if (isUnsignedInteger(primitiveType) && expression->op == UnaryMinus) {
                // TODO: warn or convert to signed integer type?
            }
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
                return Result::Failure;
            }
            expression->semanticType = static_cast<const Pointer*>(expression->right->semanticType)->baseType;
        } break;

        default:
            ASSERT_UNREACHABLE(std::format("Unknown prefix operator {}", lexer::tokenTypeToString(expression->op)));
    }
    return Result::Success;
}

// auto analyzer::visit(ast::ScopeResolutionExpression* expression) -> exprvisit_t;

auto analyzer::visit(ast::StringLiteralExpression* expression) -> exprvisit_t {
    expression->semanticType = typeContext.getPrimitive(ast::PrimitiveType_t::str);
    return Result::Success;
}
auto analyzer::visit(ast::TypeCastExpression* expression) -> exprvisit_t {
    auto result = Result::Success;
    ContextGuard guard(context.typeCastDepth, static_cast<decltype(context.typeCastDepth)>(context.typeCastDepth + 1));
    if (visit(expression->originalValue) == Result::Failure) { result = Result::Failure; }
    if (!expression->originalValue->semanticType) {
        logError(expression, "Could not deduce type of expression {}", expression->originalValue->toString());
        return Result::Failure;
    }
    if (visit(expression->targetType) == Result::Failure) { result = Result::Failure; }

    return result;
}

}  // namespace semantic
}  // namespace Manganese