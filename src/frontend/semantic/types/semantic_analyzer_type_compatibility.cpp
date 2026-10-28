#include <core.hpp>
#include <cstddef>
#include <format>
#include <frontend/ast.hpp>
#include <frontend/lexer/token.hpp>
#include <frontend/semantic/semantic_analyzer.hpp>
#include <frontend/semantic/type_context.hpp>
#include <string>
#include <utility>
#include <utils/enum_matches.hpp>
#include <utils/result.hpp>
#include <utils/type_names.hpp>

namespace Manganese::semantic {

constexpr static inline std::uint8_t f32MantissaWidth = 24;
constexpr static inline std::uint8_t f64MantissaWidth = 53;

Result SemanticAnalyzer::analyzePointerArithmetic(ast::BinaryExpression* expr) const {
    const SemanticType* lhsType = expr->left->semanticType;
    const SemanticType* rhsType = expr->right->semanticType;

    const bool lhsIsPtr = lhsType->isPointer();
    const bool rhsIsPtr = rhsType->isPointer();

    // Pointer +/- integer
    if ((lhsIsPtr && rhsType->isInteger()) || (rhsIsPtr && lhsType->isInteger())) {
        expr->semanticType = lhsIsPtr ? lhsType : rhsType;

        // Reject int - ptr
        if (expr->op == lexer::TokenType::Minus && !lhsIsPtr) {
            logError(expr, "Cannot subtract a pointer from an integer");
            return Result::Failure;
        }

        if (expr->op != lexer::TokenType::Plus && expr->op != lexer::TokenType::Minus) {
            logError(expr, "Only + and - are valid for pointer arithmetic, not '{}'",
                     lexer::tokenTypeToString(expr->op));
            return Result::Failure;
        }
        return Result::Success;
    }

    // Pointer - Pointer (distance)
    if (lhsIsPtr && rhsIsPtr) {
        expr->semanticType = typeContext.getSSizeType();
        if (expr->op != lexer::TokenType::Minus) {
            logError(expr, "Only - is valid for arithmetic between two pointers, not {}",
                     lexer::tokenTypeToString(expr->op));
            return Result::Failure;
        }
        const auto* lhsBase = static_cast<const Pointer*>(lhsType)->baseType;
        const auto* rhsBase = static_cast<const Pointer*>(rhsType)->baseType;

        if (lhsBase->isPoison()) {
            logError(expr, "Could not deduce type of pointer '{}'", expr->left->toString(interner));
            return Result::Failure;
        }

        if (rhsBase->isPoison()) {
            logError(expr, "Could not deduce type of pointer '{}'", expr->right->toString(interner));
            return Result::Failure;
        }

        if (lhsBase == rhsBase) { return Result::Success; }
        logError(expr, "Cannot subtract pointers of different types '{}' and '{}'", lhsBase->toString(),
                 rhsBase->toString());
        return Result::Failure;
    }
    return Result::Failure;
}

const SemanticType* SemanticAnalyzer::promoteNumericTypes(const SemanticType* lhs, const SemanticType* rhs) const {
    if (!lhs->isNumeric() || !rhs->isNumeric()) { return typeContext.getPoison(); }

    // direct match, don't need to promote
    if (lhs == rhs) { return lhs; }

    const PrimitiveInfo lhsInfo = getPrimitiveInfo(lhs->primitiveType);
    const PrimitiveInfo rhsInfo = getPrimitiveInfo(rhs->primitiveType);

    const bool lhsIsFloat = lhsInfo.category == PrimitiveInfo::Category::Float;
    const bool rhsIsFloat = rhsInfo.category == PrimitiveInfo::Category::Float;

    const bool lhsIsSignedInteger = (lhsInfo.category == PrimitiveInfo::Category::Int);
    const bool rhsIsSignedInteger = (rhsInfo.category == PrimitiveInfo::Category::Int);
    const bool lhsIsUnsignedInteger = (lhsInfo.category == PrimitiveInfo::Category::UInt);
    const bool rhsIsUnsignedInteger = (rhsInfo.category == PrimitiveInfo::Category::UInt);

    const bool lhsIsInteger = lhsIsSignedInteger || lhsIsUnsignedInteger;
    const bool rhsIsInteger = rhsIsSignedInteger || rhsIsUnsignedInteger;

    // If both floats, choose the wider one
    if (lhsIsFloat && rhsIsFloat) { return lhsInfo.bitWidth >= rhsInfo.bitWidth ? lhs : rhs; }

    // if at least one operand is a float, convert to a float
    if (lhsIsFloat && rhsIsInteger) { return lhs; }
    if (rhsIsFloat && lhsIsInteger) { return rhs; }

    if (lhsIsInteger && rhsIsInteger) {
        // Same sign, choose wider
        if (lhsIsSignedInteger == rhsIsSignedInteger) { return lhsInfo.bitWidth >= rhsInfo.bitWidth ? lhs : rhs; }
        const SemanticType* signedType = lhsIsSignedInteger ? lhs : rhs;
        const SemanticType* unsignedType = lhsIsSignedInteger ? rhs : lhs;

        const auto signedInfo = getPrimitiveInfo(signedType->primitiveType);
        const auto unsignedInfo = getPrimitiveInfo(unsignedType->primitiveType);

        if (signedInfo.bitWidth > unsignedInfo.bitWidth) { return signedType; }
        return unsignedType;
    }
    return typeContext.getPoison();
}

typeCompatibilityResult SemanticAnalyzer::areTypesCompatible(const SemanticType* from, const SemanticType* to) const {
    if (from->isVoid() || to->isVoid()) {
        return {.result = Compatible_t::Error,
                .message = "Cannot use an expression that evaluates to 'void' in this context"};
    }
    if (from->isPoison() || to->isPoison()) {
        return {.result = Compatible_t::Error, .message = "Could not deduce types"};
    }
    if (from->isUninitialized() || to->isUninitialized()) {
        return {.result = Compatible_t::Error,
                .message = "Cannot use keyword 'uninitialized' as part of an expression"};
    }

    // arrays can become slices (but not vice versa)
    if (from->kind == SemanticTypeKind::Array && to->kind == SemanticTypeKind::Slice) {
        const auto* arrFrom = static_cast<const Array*>(from);
        const auto* sliceTo = static_cast<const Slice*>(to);
        return areTypesCompatible(arrFrom->elementType, sliceTo->elementType);
    }

    // Special cross-kind promotion: arrays can become slices
    if (from->kind == SemanticTypeKind::Array && to->kind == SemanticTypeKind::Slice) {
        const auto* arrFrom = static_cast<const Array*>(from);
        const auto* sliceTo = static_cast<const Slice*>(to);
        return areTypesCompatible(arrFrom->elementType, sliceTo->elementType);
    }

    // Fast path for identical type pointers
    if (from == to) { return {.result = Compatible_t::Valid}; }

    const std::string conversionError = std::format("Cannot convert {} to {}", from->toString(), to->toString());

    if (from->kind != to->kind) { return {.result = Compatible_t::Error, .message = conversionError}; }

    // Dispatch to kind-specific helper functions
    switch (from->kind) {
        case SemanticTypeKind::Aggregate:
            return {.result = Compatible_t::Error,
                    .message = conversionError + " (aggregates cannot be converted to other types)."};
        case SemanticTypeKind::Array: return checkArrayCompatibility(from, to, conversionError);
        case SemanticTypeKind::Enum:
            return typeCompatibilityResult{.result = Compatible_t::Error,
                                           .message = conversionError + " (cannot convert between different enums)"};
        case SemanticTypeKind::Function: return checkFunctionCompatibility(from, to, conversionError);
        case SemanticTypeKind::Pointer: return checkPointerCompatibility(from, to, conversionError);
        case SemanticTypeKind::Primitive: return arePrimitivesCompatible(from, to);
        case SemanticTypeKind::Slice: return checkSliceCompatibility(from, to, conversionError);
        case SemanticTypeKind::Void:
        case SemanticTypeKind::Poison:
        case SemanticTypeKind::Uninitialized: break;  // Handled by guards
    }

    ASSERT_UNREACHABLE("Unknown semantic type kind in areTypesCompatible");
}
typeCompatibilityResult SemanticAnalyzer::checkArrayCompatibility(const SemanticType* from, const SemanticType* to,
                                                                  const std::string& conversionError) const {
    const auto* arrFrom = static_cast<const Array*>(from);
    const auto* arrTo = static_cast<const Array*>(to);

    if (arrFrom->length != arrTo->length) {
        return {.result = Compatible_t::Error,
                .message = conversionError + " (cannot convert between arrays of different lengths)."};
    }
    return areTypesCompatible(arrFrom->elementType, arrTo->elementType);
}

typeCompatibilityResult SemanticAnalyzer::checkFunctionCompatibility(const SemanticType* from, const SemanticType* to,
                                                                     const std::string& conversionError) const {
    const auto* funcFrom = static_cast<const Function*>(from);
    const auto* funcTo = static_cast<const Function*>(to);

    if (funcFrom->parameterTypes.size() != funcTo->parameterTypes.size()) {
        return {.result = Compatible_t::Error, .message = conversionError + " (different number of parameters)."};
    }

    for (std::size_t i = 0; i < funcFrom->parameterTypes.size(); ++i) {
        const Parameter& funcFromParam = funcFrom->parameterTypes[i];
        const Parameter& funcToParam = funcTo->parameterTypes[i];

        if (!areTypesCompatible(funcFromParam.type, funcToParam.type)) {
            const std::string error
                = std::format(" (mismatch in position {}: parameter type {} cannot be converted to parameter type {}).",
                              i, funcFromParam.toString(), funcToParam.toString());

            return {.result = Compatible_t::Error, .message = conversionError + error};
        }

        if (funcFromParam.isMutable != funcToParam.isMutable) {
            const std::string error
                = std::format(" (parameter in position {} in {} is {} but is {} in {})", i, funcFrom->toString(),
                              (funcFromParam.isMutable ? "mutable" : "immutable"),
                              (funcToParam.isMutable ? "mutable" : "immutable"), funcToParam.toString());

            return {.result = Compatible_t::Error, .message = conversionError + error};
        }
    }

    if (funcFrom->returnType->isVoid() && funcTo->returnType->isVoid()) { return {.result = Compatible_t::Valid}; }
    return areTypesCompatible(funcFrom->returnType, funcTo->returnType);
}

typeCompatibilityResult SemanticAnalyzer::checkPointerCompatibility(const SemanticType* from, const SemanticType* to,
                                                                    const std::string& conversionError) const {
    const auto* ptrFrom = static_cast<const Pointer*>(from);
    const auto* ptrTo = static_cast<const Pointer*>(to);

    // Making an immutable pointer mutable is not allowed
    if (!ptrFrom->isMutable && ptrTo->isMutable) {
        return {.result = Compatible_t::Error,
                .message = conversionError + " (cannot convert an immutable pointer to a mutable pointer)."};
    }
    return areTypesCompatible(ptrFrom->baseType, ptrTo->baseType);
}

typeCompatibilityResult SemanticAnalyzer::checkSliceCompatibility(const SemanticType* from, const SemanticType* to,
                                                                  const std::string& conversionError) const {
    if (to->kind != SemanticTypeKind::Slice) { return {.result = Compatible_t::Error, .message = conversionError}; }
    const auto* sliceFrom = static_cast<const Slice*>(from);
    const auto* sliceTo = static_cast<const Slice*>(to);
    return areTypesCompatible(sliceFrom->elementType, sliceTo->elementType);
}

auto SemanticAnalyzer::arePrimitivesCompatible(const SemanticType* from, const SemanticType* to) const
    -> typeCompatibilityResult {
    using Cat = PrimitiveInfo::Category;
    if (from->primitiveType == to->primitiveType) { return {.result = Compatible_t::Valid}; }

    const PrimitiveInfo src = getPrimitiveInfo(from->primitiveType);
    const PrimitiveInfo dest = getPrimitiveInfo(to->primitiveType);

    if (dest.category == Cat::Bool && (context.inCondition || context.inTernary)) {
        return {.result = Compatible_t::Valid};
    }

    auto yield_warning = [this](std::string&& msg) -> typeCompatibilityResult {
        if (context.typeCastDepth) { return {.result = Compatible_t::Valid}; }
        return {.result = Compatible_t::Warning, .message = std::move(msg)};
    };

    // String conversions
    if (src.category == Cat::String) {
        if (dest.category == Cat::Bool) {
            return yield_warning(std::format("Implicit conversion from '{}' to '{}'", string_str, bool_str));
        }
        return {.result = Compatible_t::Error,
                .message = std::format("Cannot convert '{}' to non-boolean type", string_str)};
    }

    if (dest.category == Cat::String) {
        if (src.category == Cat::Char) {
            return {.result = Compatible_t::Valid};  // char -> string is fine
        }
        return {.result = Compatible_t::Error,
                .message = std::format("Cannot convert non-char type to '{}'", string_str)};
    }

    // Bool and char to numeric
    if (src.category == Cat::Bool || dest.category == Cat::Bool) {
        // Non-conditional conversions involving bool warrant a warning
        return yield_warning(
            std::format("Conversion between '{}' and '{}' can alter semantics", from->toString(), to->toString()));
    }

    // Float <-> integer
    if ((src.category == Cat::Int || src.category == Cat::UInt) && dest.category == Cat::Float) {
        // Int to float can lose precision if the int width >= float mantissa width
        if (src.bitWidth >= (dest.bitWidth == 32 ? f32MantissaWidth : f64MantissaWidth)) {
            return yield_warning(std::format("Conversion from '{}' to '{}' may lose precision digits", from->toString(),
                                             to->toString()));
        }
        return {.result = Compatible_t::Valid};  // e.g. i16 -> f32 is completely safe
    }

    if (src.category == Cat::Float && (dest.category == Cat::Int || dest.category == Cat::UInt)) {
        return yield_warning(
            std::format("Conversion from '{}' to '{}' truncates decimal components", from->toString(), to->toString()));
    }

    // Integer <-> Integer or Float <-> Float
    if (src.category != dest.category && src.category != Cat::Float && dest.category != Cat::Float) {
        return yield_warning(
            std::format("Sign mismatch: conversion between '{}' and '{}' may cause data loss or sign-flipping",
                        from->toString(), to->toString()));
    }
    if (src.bitWidth > dest.bitWidth) {
        return yield_warning(std::format("Narrowing conversion: potential data loss converting from '{}' to '{}'",
                                         from->toString(), to->toString()));
    }
    return {.result = Compatible_t::Valid};  // Widening conversion is fine
}

auto SemanticAnalyzer::areTypesComparable(const SemanticType* lhs, const SemanticType* rhs) -> typeCompatibilityResult {
    if (lhs->isVoid() || rhs->isVoid()) {
        return {.result = Compatible_t::Error, .message = "Cannot compare void types"};
    }
    if (lhs->isPoison() || rhs->isPoison()) {
        return {.result = Compatible_t::Error, .message = "Unable to deduce types in comparison"};
    }
    if (lhs->isString() && rhs->isString()) { return {.result = Compatible_t::Valid}; }

    if (lhs == rhs) { return {.result = Compatible_t::Valid}; }

    if (lhs->isNumeric() && rhs->isNumeric()) { return {.result = Compatible_t::Valid}; }

    if (lhs->isPointer() && rhs->isPointer()) {
        const auto* lhsBase = static_cast<const Pointer*>(lhs)->baseType;
        const auto* rhsBase = static_cast<const Pointer*>(rhs)->baseType;

        if (lhsBase == rhsBase) { return {.result = Compatible_t::Valid}; }

        return {.result = Compatible_t::Error,
                .message
                = std::format("Cannot compare distinct pointer types '{}' and '{}'", lhs->toString(), rhs->toString())};
    }

    if (lhs->isEnum() && rhs->isEnum()) {
        if (lhs != rhs) {
            return {.result = Compatible_t::Error,
                    .message = std::format("Cannot compare distinct enum types '{}' and '{}'", lhs->toString(),
                                           rhs->toString())};
        }
        return {.result = Compatible_t::Valid};
    }

    return {
        .result = Compatible_t::Error,
        .message = std::format("Incompatible types for comparison: '{}' and '{}'", lhs->toString(), rhs->toString())};
}

Result SemanticAnalyzer::checkArrayElementCompatibility(const SemanticType* targetType, std::size_t i,
                                                        ast::Expression* element) {
    const auto compat = areTypesCompatible(targetType, element->semanticType);
    if (!compat) {
        logError(element,
                 "Array element '{}' (position {} in literal) of type '{}' is not compatible with expected type '{}'",
                 element->toString(interner), i + 1, element->semanticType->toString(), targetType->toString());
        return exprvisit_t::Failure;
    }
    if (compat.result == Compatible_t::Warning) { logWarning(element, "{}", compat.message); }
    return exprvisit_t::Success;
}

}  // namespace Manganese::semantic