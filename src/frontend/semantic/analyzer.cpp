#include <core.hpp>
#include <cstddef>
#include <format>
#include <frontend/ast.hpp>
#include <frontend/semantic/analyzer.hpp>
#include <frontend/semantic/type_context.hpp>
#include <mnstl/fold_result.hxx>
#include <string>
#include <utility>
#include <utils/result.hpp>
#include <utils/type_names.hpp>

namespace Manganese::semantic {

constexpr static inline uint8_t f32MantissaWidth = 24;
constexpr static inline uint8_t f64MantissaWidth = 53;

Result analyzer::analyze() {
    Result isSemanticallyValid = Result::Success;
    if (collectTypes() == Result::Failure) { isSemanticallyValid = Result::Failure; }
    if (collectGlobals() == Result::Failure) { isSemanticallyValid = Result::Failure; }
    if (collectAndSpecializeGenerics() == Result::Failure) { isSemanticallyValid = Result::Failure; }
    // Don't want errors cascading because of conflicting redeclarations
    if (isSemanticallyValid == Result::Failure) { return isSemanticallyValid; }

    symbolTable.switchToCheckingMode();
    isSemanticallyValid = checkStatements();
    return isSemanticallyValid;
}

// Placeholders to satisfy the linker
// TODO: implement these
Result analyzer::collectGlobals() { return Result::Success; }
Result analyzer::collectAndSpecializeGenerics() { return Result::Success; }

const SemanticType* analyzer::promoteNumericTypes(const SemanticType* lhs, const SemanticType* rhs) const {
    DISCARD(lhs);
    DISCARD(rhs);
    return nullptr;
}

Result analyzer::analyzePointerArithmetic(const SemanticType* lhs, const SemanticType* rhs) const {
    DISCARD(lhs);
    DISCARD(rhs);
    return Result::Success;
}
auto analyzer::areTypesComparable(const SemanticType* lhs, const SemanticType* rhs) const -> typeCompatibilityResult {
    DISCARD(lhs);
    DISCARD(rhs);
    return {.result = Compatible_t::Valid};
}

Result analyzer::checkStatements() {  // semantic analysis pass (this can also check the generic specializations)
    Result programIsSemanticallyValid = Result::Success;
    for (ast::Statement* stmt : parsedFile.program) {
        if (this->visit(stmt) == Result::Failure) { programIsSemanticallyValid = Result::Failure; }
    }
    return programIsSemanticallyValid;
}

// Generics

const SemanticType* analyzer::getInstantiatedFunctionType(const ast::FunctionDeclarationStatement* decl,
                                                          const std::vector<const SemanticType*>& typeArgs) {
    if (!decl) [[unlikely]] { return nullptr; }
    InstantiationKey key{.declNode = decl, .typeArgs = typeArgs};
    const InstantiationResult* cachedResult = instantiationCache.find(key);
    if (!cachedResult || cachedResult->state != ResolutionStatus::Success) {
        return nullptr;  // Not instantiated or failed
    }

    const SemanticType* resolvedReturnType = cachedResult->returnType;

    std::vector<Parameter> instantiatedParams;
    instantiatedParams.reserve(decl->parameters.size());

    for (const ast::FunctionParameter& paramNode : decl->parameters) {
        visit(paramNode.type);
        const SemanticType* paramType = paramNode.type->semanticType;
        if (!paramType) { return nullptr; }
        instantiatedParams.push_back(Parameter{.isMutable = paramNode.isMutable, .type = paramType});
    }
    return typeContext.getFunction(std::move(instantiatedParams), resolvedReturnType);
}

const SemanticType* analyzer::getInstantiatedAggregateType(const ast::AggregateDeclarationStatement* decl,
                                                           const std::vector<const SemanticType*>& typeArgs) {
    if (!decl) [[unlikely]] { return nullptr; }
    InstantiationKey key{.declNode = decl, .typeArgs = typeArgs};
    const InstantiationResult* cachedResult = instantiationCache.find(key);
    if (!cachedResult || cachedResult->state != ResolutionStatus::Success) {
        return nullptr;  // Not instantiated or failed
    }
    std::vector<AggregateField> instantiatedFields;
    instantiatedFields.reserve(decl->fields.size());

    for (const ast::AggregateField& fieldNode : decl->fields) {
        // resolveType substitutes generic parameters (e.g. T -> float) using genericStack.top()
        visit(fieldNode.type);
        const SemanticType* fieldType = fieldNode.type->semanticType;
        if (!fieldType) { return nullptr; }
        instantiatedFields.push_back(AggregateField{.name = fieldNode.name, .type = fieldType});
    }

    std::string instantiatedName = decl->name + "$";
    for (std::size_t i = 0; i < typeArgs.size(); ++i) {
        if (i > 0) { instantiatedName += "$"; }
        instantiatedName += typeArgs[i]->toString();
    }

    return typeContext.getNamedAggregate(instantiatedName, std::move(instantiatedFields));
}

// Type compatibility

namespace {
struct PrimitiveInfo {
    enum class Category {
        Int,
        UInt,
        Float,
        Char,
        Bool,
        String
    };
    Category category;
    int bitWidth = 0;
};

inline PrimitiveInfo getPrimitiveInfo(ast::PrimitiveType_t type) {
    using enum ast::PrimitiveType_t;
    using Cat = PrimitiveInfo::Category;

    switch (type) {
        case i8: return {.category = Cat::Int, .bitWidth = 8};
        case i16: return {.category = Cat::Int, .bitWidth = 16};
        case i32: return {.category = Cat::Int, .bitWidth = 32};
        case i64: return {.category = Cat::Int, .bitWidth = 64};
        case i128: return {.category = Cat::Int, .bitWidth = 128};

        case u8: return {.category = Cat::UInt, .bitWidth = 8};
        case u16: return {.category = Cat::UInt, .bitWidth = 16};
        case u32: return {.category = Cat::UInt, .bitWidth = 32};
        case u64: return {.category = Cat::UInt, .bitWidth = 64};
        case u128: return {.category = Cat::UInt, .bitWidth = 128};

        case f32: return {.category = Cat::Float, .bitWidth = 32};
        case f64: return {.category = Cat::Float, .bitWidth = 64};

        case character: return {.category = Cat::Char, .bitWidth = 8};
        case boolean: return {.category = Cat::Bool, .bitWidth = 1};
        case str: return {.category = Cat::String, .bitWidth = 0};
        default: break;
    }
    return {Cat::Int, 0};
}
}  // namespace

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
            for (size_t i = 0; i < genericFrom->typeArguments.size(); ++i) {
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
        default: ASSERT_UNREACHABLE("Unknown semantic type kind in areTypesCompatible");
    }
}

}  // namespace Manganese::semantic
