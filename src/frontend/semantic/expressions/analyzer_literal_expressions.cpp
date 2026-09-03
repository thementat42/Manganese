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
    expression->semanticType = typeContext.getPrimitive(ast::PrimitiveType::boolean);
    return exprvisit_t::Success;
}
auto Analyzer::visit(ast::CharLiteralExpression* expression) -> exprvisit_t {
    expression->semanticType = typeContext.getPrimitive(ast::PrimitiveType::character);
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
    using enum mnstl::number_t::held_type;
    using enum ast::PrimitiveType;
    switch (expression->value.underlying_type()) {
        case i8: expression->semanticType = typeContext.getPrimitive(int8); break;
        case i16: expression->semanticType = typeContext.getPrimitive(int16); break;
        case i32: expression->semanticType = typeContext.getPrimitive(int32); break;
        case i64: expression->semanticType = typeContext.getPrimitive(int64); break;
        case i128: expression->semanticType = typeContext.getPrimitive(int128); break;
        case u8: expression->semanticType = typeContext.getPrimitive(uint8); break;
        case u16: expression->semanticType = typeContext.getPrimitive(uint16); break;
        case u32: expression->semanticType = typeContext.getPrimitive(uint32); break;
        case u64: expression->semanticType = typeContext.getPrimitive(uint64); break;
        case u128: expression->semanticType = typeContext.getPrimitive(uint128); break;
        case f32: expression->semanticType = typeContext.getPrimitive(floata32); break;
        case f64: expression->semanticType = typeContext.getPrimitive(float64); break;
        case err: {
            logError(expression, "{}", expression->value.error_unchecked());
            expression->semanticType = nullptr;
            return exprvisit_t::Failure;
        }
        case none: break;
        default: ASSERT_UNREACHABLE("In analyzer: Number literal expression had no parser-deduced type");
    }
    return exprvisit_t::Success;
}

auto Analyzer::visit(ast::StringLiteralExpression* expression) -> exprvisit_t {
    expression->semanticType = typeContext.getPrimitive(ast::PrimitiveType::string);
    return exprvisit_t::Success;
}

auto Analyzer::visit(ast::PoisonedExpression*) -> exprvisit_t { return exprvisit_t::Failure; }

}  // namespace Manganese::semantic