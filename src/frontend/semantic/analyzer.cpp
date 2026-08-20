#include <core.hpp>
#include <cstddef>
#include <format>
#include <frontend/ast.hpp>
#include <frontend/lexer/token.hpp>
#include <frontend/semantic/analyzer.hpp>
#include <frontend/semantic/type_context.hpp>
#include <mnstl/enum_matches.hxx>
#include <mnstl/fold_result.hxx>
#include <string>
#include <utility>
#include <utils/result.hpp>
#include <utils/type_names.hpp>

namespace Manganese::semantic {

constexpr static inline std::uint8_t f32MantissaWidth = 24;
constexpr static inline std::uint8_t f64MantissaWidth = 53;

Result analyzer::analyze() {
    // Don't want errors cascading because of conflicting redeclarations
    if (collectTypes() == Result::Failure) { return Result::Failure; }
    symbolTable.switchToCheckingMode();
    if (collectGlobals() == Result::Failure) { return Result::Failure; }
    return checkStatements();
}

Result analyzer::collectGlobals() {
    Result result = Result::Success;
    for (ast::Statement* statement : parsedFile.program) {
        if (statement->kind != ast::StatementKind::AggregateDeclarationStatement) { continue; }
        auto* aggregate = static_cast<ast::AggregateDeclarationStatement*>(statement);
        if (collectGlobalAggregate(aggregate) == Result::Failure) { result = Result::Failure; }
    }

    // Do this on a separate pass in case a function uses an aggregate in its signature
    for (ast::Statement* statement : parsedFile.program) {
        if (statement->kind != ast::StatementKind::FunctionDeclarationStatement) { continue; }
        auto* function = static_cast<ast::FunctionDeclarationStatement*>(statement);
        if (collectGlobalFunction(function) == Result::Failure) { result = Result::Failure; }
    }

    return result;
}

bool analyzer::isMutableExpression(const ast::Expression* expr) {
    if (!expr) { return false; }
    using enum ast::ExpressionKind;
    switch (expr->kind) {
        case IdentifierExpression: {
            const auto* id = static_cast<const ast::IdentifierExpression*>(expr);
            const Symbol* symbol = symbolTable.lookup(id->name);
            return symbol ? symbol->isMutable : false;
        }
        case PrefixExpression: {
            const auto* prefix = static_cast<const ast::PrefixExpression*>(expr);
            if (prefix->op == lexer::TokenType::Dereference) {
                const SemanticType* opType = prefix->right->semanticType;
                if (!opType || !opType->isPointer()) { return false; }
                return static_cast<const Pointer*>(opType)->isMutable;
            }
            return false;
        }
        case MemberAccessExpression: {
            const auto* mem = static_cast<const ast::MemberAccessExpression*>(expr);
            return isMutableExpression(mem->object);
        }
        case IndexExpression: {
            const auto* index = static_cast<const ast::IndexExpression*>(expr);
            return isMutableExpression(index->variable);
        }
        case ScopeResolutionExpression: {
            const auto* scope = static_cast<const ast::ScopeResolutionExpression*>(expr);
            Symbol* sym = symbolTable.scopedLookup(scope->scope, scope->element);
            return sym ? sym->isMutable : false;
        }
        default: return false;
    }
}

Result analyzer::collectGlobalAggregate(ast::AggregateDeclarationStatement* aggregate) {
    // Skip uninstantiated generics
    if (!aggregate->genericTypes.empty()) { return Result::Success; }

    Symbol* symbol = symbolTable.lookup(aggregate->name);
    if (!symbol) {
        ASSERT_UNREACHABLE(std::format("Aggregate '{}' was not recorded during type collection", aggregate->name));
    }

    // Skip if already processed
    if (symbol->type) { return Result::Success; }

    Result result = Result::Success;
    std::vector<AggregateField> fields;
    fields.reserve(aggregate->fields.size());

    for (const auto& field : aggregate->fields) {
        const typevisit_t fieldResult = visit(field.type);
        if (fieldResult == typevisit_t::Failure) {
            logError(aggregate, "Unknown type '{}' for field '{}' in aggregate '{}'", field.type->toString(),
                     field.name, aggregate->name);
            result = Result::Failure;
        }

        const SemanticType* fieldType
            = (fieldResult == typevisit_t::Failure ? typeContext.getVoid() : field.type->semanticType);
        fields.push_back(AggregateField{.name = field.name, .type = fieldType});
    }

    symbol->type = typeContext.getNamedAggregate(aggregate->name, std::move(fields));
    symbol->status = ResolutionStatus::NotStarted;

    return result;
}

Result analyzer::collectGlobalFunction(ast::FunctionDeclarationStatement* function) {
    // Skip uninstantiated generics
    if (!function->genericTypes.empty()) { return Result::Success; }

    Symbol* symbol = symbolTable.lookup(function->name);
    if (!symbol) {
        ASSERT_UNREACHABLE(std::format("Function '{}' was not recorded during type collection", function->name));
    }

    // Detect direct signature cycles (e.g. infinite parameter expansion)
    if (symbol->status == ResolutionStatus::InProgress) {
        logError(function, "Cyclic dependency detected in signature of function '{}'", function->name);
        symbol->status = ResolutionStatus::Failure;
        return Result::Failure;
    }

    symbol->status = ResolutionStatus::InProgress;

    std::vector<Parameter> paramTypes;
    paramTypes.reserve(function->parameters.size());
    Result funcResult = Result::Success;

    for (const auto& param : function->parameters) {
        const typevisit_t paramResult = visit(param.type);
        if (paramResult == typevisit_t::Failure) {
            logError(function, "Unknown parameter type '{}' for parameter '{}' in function '{}'",
                     param.type->toString(), param.name, function->name);
            funcResult = Result::Failure;
        }

        const SemanticType* paramType
            = (paramResult == typevisit_t::Failure ? typeContext.getVoid() : param.type->semanticType);
        paramTypes.push_back(
            Parameter{.isMutable = param.isMutable, .isVariadic = param.isVariadic, .type = paramType});
    }

    const SemanticType* resolvedReturnType = typeContext.getVoid();
    if (function->returnType) {
        const typevisit_t returnResult = visit(function->returnType);
        if (returnResult == typevisit_t::Failure) {
            logError(function, "Unknown return type '{}' in function '{}'", function->returnType->toString(),
                     function->name);
            funcResult = Result::Failure;
        } else {
            resolvedReturnType = function->returnType->semanticType;
        }
    }

    symbol->type = typeContext.getFunction(std::move(paramTypes), resolvedReturnType);
    symbol->status = ResolutionStatus::NotStarted;

    return funcResult;
}

const SemanticType* analyzer::promoteNumericTypes(const SemanticType* lhs, const SemanticType* rhs) const {
    if (!lhs || !rhs) { return nullptr; }
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
    return nullptr;
}

Result analyzer::analyzePointerArithmetic(ast::BinaryExpression* expr) const {
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
        const auto* lhsPtr = static_cast<const Pointer*>(lhsType);
        const auto* rhsPtr = static_cast<const Pointer*>(rhsType);
        if (lhsPtr->baseType == rhsPtr->baseType) { return Result::Success; }
    }
    return Result::Failure;
}

auto analyzer::areTypesComparable(const SemanticType* lhs, const SemanticType* rhs) const -> typeCompatibilityResult {
    if (lhs->isVoid() || rhs->isVoid()) {
        return {.result = Compatible_t::Error, .message = "Cannot compare void types"};
    }
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

Result analyzer::checkStatements() {  // semantic analysis pass (this can also check the generic specializations)
    Result programIsSemanticallyValid = Result::Success;
    for (ast::Statement* stmt : parsedFile.program) {
        if (this->visit(stmt) == Result::Failure) { programIsSemanticallyValid = Result::Failure; }
    }
    return programIsSemanticallyValid;
}

// Type compatibility

auto analyzer::arePrimitivesCompatible(const SemanticType* from, const SemanticType* to) const
    -> typeCompatibilityResult {
    using Cat = PrimitiveInfo::Category;
    if (from->primitiveType == to->primitiveType) { return {.result = Compatible_t::Valid}; }

    const PrimitiveInfo src = getPrimitiveInfo(from->primitiveType);
    const PrimitiveInfo dest = getPrimitiveInfo(to->primitiveType);
    const bool is_conditional_context = context.ifStatementDepth || context.forLoopDepth || context.whileLoopDepth;

    if (dest.category == Cat::Bool && is_conditional_context) { return {.result = Compatible_t::Valid}; }

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

auto analyzer::areTypesCompatible(const SemanticType* from, const SemanticType* to) const -> typeCompatibilityResult {
    // Null pointer means something went wrong in type deduction
    if (!from || !to) { return {.result = Compatible_t::Error, .message = "Could not deduce types"}; }

    if (from->isVoid() || to->isVoid()) {
        return {.result = Compatible_t::Error,
                .message = "Cannot use an expression that evaluates to 'void' in this context"};
    }

    // Duplicated types point to the same underlying value so we can just do a fast pointer comparison
    if (from == to) { return {.result = Compatible_t::Valid}; }

    const std::string conversionError = std::format("Cannot convert {} to {}", from->toString(), to->toString());

    // Totally distinct types are not interconvertible
    if (from->kind != to->kind) { return {.result = Compatible_t::Error, .message = conversionError}; }

    // Same structure but different instances (e.g. pointers to different types)
    switch (from->kind) {
        case Kind::Aggregate:
            return {.result = Compatible_t::Error,
                    .message = conversionError + " (aggregates cannot be converted to other types)."};
        case Kind::Array: {
            const auto* arrFrom = static_cast<const Array*>(from);
            const auto* arrTo = static_cast<const Array*>(to);
            if (arrFrom->length != arrTo->length) {
                return {.result = Compatible_t::Error,
                        .message = conversionError + " (cannot convert between arrays of different lengths)."};
            }
            typeCompatibilityResult baseCompatible = areTypesCompatible(arrFrom->elementType, arrTo->elementType);

            if (!baseCompatible) {
                return {.result = Compatible_t::Error,
                        .message = conversionError
                            + std::format(" (cannot convert an array of {} to an array of {})",
                                          arrFrom->elementType->toString(), arrTo->elementType->toString())};
            }
            return baseCompatible;
        } break;

        case Kind::Function: {
            const auto* funcFrom = static_cast<const Function*>(from);
            const auto* funcTo = static_cast<const Function*>(to);

            if (funcFrom->parameterTypes.size() != funcTo->parameterTypes.size()) {
                return {.result = Compatible_t::Error,
                        .message = conversionError + " (different number of parameters)."};
            }
            for (std::size_t i = 0; i < funcFrom->parameterTypes.size(); ++i) {
                const Parameter& funcFromParam = funcFrom->parameterTypes[i];
                const Parameter& funcToParam = funcTo->parameterTypes[i];
                if (!areTypesCompatible(funcFromParam.type, funcToParam.type)) {
                    const std::string error = std::format(
                        " (mismatch in position {}: parameter type {} is cannot be converted to parameter type {}).", i,
                        funcFromParam.toString(), funcToParam.toString());

                    return {.result = Compatible_t::Error, .message = conversionError + error};
                }
                if (funcFromParam.isMutable != funcToParam.isMutable) {
                    const std::string error
                        = std::format(" (parameter in position {} in {} is {} but is {} in {})", i,
                                      funcFrom->toString(), (funcFromParam.isMutable ? "mutable" : "immutable"),
                                      (funcToParam.isMutable ? "mutable" : "immutable"), funcToParam.toString());

                    return {.result = Compatible_t::Error, .message = conversionError + error};
                }
                if (funcFromParam.isVariadic != funcToParam.isVariadic) {
                    const std::string error
                        = std::format(" (parameter in position {} is {} in {} but {} in {})", i,
                                      (funcFromParam.isVariadic ? "variadic" : "non-variadic"), funcFrom->toString(),
                                      (funcToParam.isVariadic ? "variadic" : "non-variadic"), funcTo->toString());

                    return {.result = Compatible_t::Error, .message = conversionError + error};
                }
            }
            if (funcFrom->returnType->isVoid() && funcTo->returnType->isVoid()) {
                return {.result = Compatible_t::Valid};
            }
            return areTypesCompatible(funcFrom->returnType, funcTo->returnType);
        } break;

        case Kind::Generic: {
            const auto* genericFrom = static_cast<const GenericInstance*>(from);
            const auto* genericTo = static_cast<const GenericInstance*>(to);

            if (genericFrom->baseType != genericTo->baseType) { return {.result = Compatible_t::Error}; }
            if (genericFrom->typeArguments.size() != genericTo->typeArguments.size()) {
                return {.result = Compatible_t::Error,
                        .message = conversionError + " (different number of type parameters)."};
            }
            for (std::size_t i = 0; i < genericFrom->typeArguments.size(); ++i) {
                const SemanticType* fromArgument = genericFrom->typeArguments[i];
                const SemanticType* toArgument = genericTo->typeArguments[i];
                if (!areTypesCompatible(fromArgument, toArgument)) {
                    return {.result = Compatible_t::Error,
                            .message = conversionError
                                + std::format(" (mismatch in position {}: {} cannot convert to {})", i,
                                              fromArgument->toString(), toArgument->toString())};
                }
            }
            return {.result = Compatible_t::Valid};
        };

        case Kind::Pointer: {
            const auto* ptrFrom = static_cast<const Pointer*>(from);
            const auto* ptrTo = static_cast<const Pointer*>(to);

            // making an immutable pointer (ptr int) mutable (ptr mut int) is not allowed
            // but making a mutable pointer (ptr mut int) mutable (ptr int) is fine
            if (!ptrFrom->isMutable && ptrTo->isMutable) {
                return {.result = Compatible_t::Error,
                        .message = conversionError + " (cannot convert an immutable pointer to a mutable pointer)."};
            }
            return areTypesCompatible(ptrFrom->baseType, ptrTo->baseType);
        }; break;

        case Kind::Primitive: {
            return arePrimitivesCompatible(from, to);
        }; break;
        case Kind::Void:
            return {.result = Compatible_t::Error, .message = "Cannot use 'void' expression in this context"};

        default: ASSERT_UNREACHABLE("Unknown semantic type kind in areTypesCompatible");
    }
}

}  // namespace Manganese::semantic
