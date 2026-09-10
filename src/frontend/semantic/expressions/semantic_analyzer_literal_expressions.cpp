#include <core.hpp>
#include <frontend/ast.hpp>
#include <frontend/lexer/token.hpp>
#include <frontend/semantic/semantic_analyzer.hpp>
#include <frontend/semantic/symbol_table.hpp>
#include <frontend/semantic/type_context.hpp>
#include <utility>
#include <utils/result.hpp>
#include <vector>

namespace Manganese::semantic {

auto SemanticAnalyzer::visit(ast::AggregateLiteralExpression* expression) -> exprvisit_t {
    expression->semanticType = typeContext.getPoison();
    auto result = exprvisit_t::Success;
    TypeList elementTypes;
    elementTypes.reserve(expression->elements.size());

    for (ast::Expression* element : expression->elements) {
        if (visit(element) == exprvisit_t::Failure) { result = exprvisit_t::Failure; }
        if (element->semanticType->isPoison()) {
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

auto SemanticAnalyzer::visit(ast::ArrayLiteralExpression* expression) -> exprvisit_t {
    expression->semanticType = typeContext.getPoison();
    if (expression->elements.empty()) {
        if (context.currentVariableDeclarationType != nullptr && context.currentVariableDeclarationType->isArray()) {
            expression->semanticType = context.currentVariableDeclarationType;
            return exprvisit_t::Success;
        }
        logError(expression, "Cannot infer element type for empty array literal without type annotation");
        return exprvisit_t::Failure;
    }

    const SemanticType* expectedElementType = nullptr;
    if (context.currentVariableDeclarationType != nullptr && context.currentVariableDeclarationType->isArray()) {
        expectedElementType = static_cast<const Array*>(context.currentVariableDeclarationType)->elementType;
    }

    auto result = exprvisit_t::Success;
    const SemanticType* synthesizedElementType = nullptr;
    const SemanticType* targetElementType = expectedElementType;

    for (std::size_t elementIndex = 0; elementIndex < expression->elements.size(); ++elementIndex) {
        ast::Expression* element = expression->elements[elementIndex];

        const SemanticType* outerVarType = context.currentVariableDeclarationType;
        context.currentVariableDeclarationType = expectedElementType;
        auto elemVisitResult = visit(element);
        context.currentVariableDeclarationType = outerVarType;

        if (elemVisitResult == exprvisit_t::Failure || element->semanticType == nullptr) {
            result = exprvisit_t::Failure;
            continue;
        }
        if (element->semanticType->isVoid()) {
            logError(element, "Cannot use 'void' expression in array literal");
            result = exprvisit_t::Failure;
            continue;
        }

        // If no explicit type annotation exists, use the first element as the type of the array
        if (targetElementType == nullptr && elementIndex == 0) {
            synthesizedElementType = element->semanticType;
            targetElementType = synthesizedElementType;
            continue;
        }

        if (checkArrayElementCompatibility(targetElementType, elementIndex, element) == exprvisit_t::Failure) {
            result = exprvisit_t::Failure;
        }
    }

    if (result == exprvisit_t::Failure) { return exprvisit_t::Failure; }

    const SemanticType* finalElementType
        = (expectedElementType != nullptr) ? expectedElementType : synthesizedElementType;
    expression->semanticType = typeContext.getArray(finalElementType, expression->elements.size());
    return exprvisit_t::Success;
}

auto SemanticAnalyzer::visit(ast::BoolLiteralExpression* expression) -> exprvisit_t {
    expression->semanticType = typeContext.getPoison();
    expression->semanticType = typeContext.getPrimitive(ast::PrimitiveType::boolean);
    return exprvisit_t::Success;
}
auto SemanticAnalyzer::visit(ast::CharLiteralExpression* expression) -> exprvisit_t {
    expression->semanticType = typeContext.getPoison();
    expression->semanticType = typeContext.getPrimitive(ast::PrimitiveType::character);
    return exprvisit_t::Success;
}

auto SemanticAnalyzer::visit(ast::IdentifierExpression* expression) -> exprvisit_t {
    expression->semanticType = typeContext.getPoison();
    const Symbol* symbol = symbolTable.lookup(expression->name);
    if (symbol == nullptr) {
        logError(expression, "Identifier '{}' was not found in the current scope", expression->name);
        return exprvisit_t::Failure;
    }
    if (symbol->type->isPoison()) [[unlikely]] {
        logError(expression, "Identifier '{}' used before its type could be determined", expression->name);
        return exprvisit_t::Failure;
    }
    expression->semanticType = symbol->type;
    return exprvisit_t::Success;
}

auto SemanticAnalyzer::visit(ast::NumberLiteralExpression* expression) -> exprvisit_t {
    expression->semanticType = typeContext.getPoison();
    const std::string_view lexeme = expression->value;
    using enum ast::PrimitiveType;
    ast::PrimitiveType deducedPrimitiveType;

    if (lexeme.ends_with("i128") || lexeme.ends_with("I128")) {
        deducedPrimitiveType = int128;
    } else if (lexeme.ends_with("i64") || lexeme.ends_with("I64")) {
        deducedPrimitiveType = int64;
    } else if (lexeme.ends_with("i32") || lexeme.ends_with("I32")) {
        deducedPrimitiveType = int32;
    } else if (lexeme.ends_with("i16") || lexeme.ends_with("I16")) {
        deducedPrimitiveType = int16;
    } else if (lexeme.ends_with("i8") || lexeme.ends_with("I8")) {
        deducedPrimitiveType = int8;
    } else if (lexeme.ends_with("u128") || lexeme.ends_with("U128")) {
        deducedPrimitiveType = uint128;
    } else if (lexeme.ends_with("u64") || lexeme.ends_with("U64")) {
        deducedPrimitiveType = uint64;
    } else if (lexeme.ends_with("u32") || lexeme.ends_with("U32")) {
        deducedPrimitiveType = uint32;
    } else if (lexeme.ends_with("u16") || lexeme.ends_with("U16")) {
        deducedPrimitiveType = uint16;
    } else if (lexeme.ends_with("u8") || lexeme.ends_with("U8")) {
        deducedPrimitiveType = uint8;
    } else if (lexeme.ends_with("f32") || lexeme.ends_with("F32")) {
        deducedPrimitiveType = float32;
    } else if (lexeme.ends_with("f64") || lexeme.ends_with("F64")) {
        deducedPrimitiveType = float64;
    } else {
        // TODO: need to dynamically adjust int32 based on literal
        deducedPrimitiveType = expression->isFloat ? float64 : int32;
    }
    expression->semanticType = typeContext.getPrimitive(deducedPrimitiveType);
    return exprvisit_t::Success;
}

auto SemanticAnalyzer::visit(ast::StringLiteralExpression* expression) -> exprvisit_t {
    expression->semanticType = typeContext.getPoison();
    expression->semanticType = typeContext.getPrimitive(ast::PrimitiveType::string);
    return exprvisit_t::Success;
}

auto SemanticAnalyzer::visit(ast::PoisonedExpression* expression) -> exprvisit_t {
    expression->semanticType = typeContext.getPoison();
    return exprvisit_t::Failure;
}

}  // namespace Manganese::semantic