#include <core.hpp>
#include <frontend/ast.hpp>
#include <frontend/lexer/token.hpp>
#include <frontend/semantic/analyzer.hpp>
#include <frontend/semantic/symbol_table.hpp>
#include <frontend/semantic/type_context.hpp>
#include <mnstl/number.hxx>
#include <utility>
#include <utils/result.hpp>
#include <vector>

namespace Manganese::semantic {

auto Analyzer::visit(ast::AggregateLiteralExpression* expression) -> exprvisit_t {
    auto result = exprvisit_t::Success;
    TypeList elementTypes;
    elementTypes.reserve(expression->elements.size());

    for (ast::Expression* element : expression->elements) {
        if (visit(element) == exprvisit_t::Failure) { result = exprvisit_t::Failure; }
        if (!element->semanticType) {
            result = exprvisit_t::Failure;
        } else if (element->semanticType->isVoid()) {
            logError(element, "Cannot use 'void' expression in aggregate literal");
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

auto Analyzer::visit(ast::ArrayLiteralExpression* expression) -> exprvisit_t {
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
        if (element->semanticType->isVoid()) {
            logError(element, "Cannot use 'void' expression in array literal");
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

auto Analyzer::visit(ast::BoolLiteralExpression* expression) -> exprvisit_t {
    expression->semanticType = typeContext.getPrimitive(ast::PrimitiveType_t::boolean);
    return exprvisit_t::Success;
}
auto Analyzer::visit(ast::CharLiteralExpression* expression) -> exprvisit_t {
    expression->semanticType = typeContext.getPrimitive(ast::PrimitiveType_t::character);
    return exprvisit_t::Success;
}

auto Analyzer::visit(ast::IdentifierExpression* expression) -> exprvisit_t {
    const Symbol* symbol = symbolTable.lookup(expression->name);
    if (!symbol) {
        logError(expression, "Identifier '{}' was not found in the current scope", expression->name);
        return exprvisit_t::Failure;
    }
    if (!symbol->type) [[unlikely]] {
        logError(expression, "Identifier '{}' used before its type could be determined", expression->name);
        return exprvisit_t::Failure;
    }
    expression->semanticType = symbol->type;
    return exprvisit_t::Success;
}

auto Analyzer::visit(ast::NumberLiteralExpression* expression) -> exprvisit_t {
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

auto Analyzer::visit(ast::StringLiteralExpression* expression) -> exprvisit_t {
    expression->semanticType = typeContext.getPrimitive(ast::PrimitiveType_t::str);
    return exprvisit_t::Success;
}

}  // namespace Manganese::semantic