#include <core.hpp>
#include <frontend/semantic/analyzer.hpp>
#include <frontend/semantic/type_context.hpp>

namespace Manganese {
namespace semantic {

Result analyzer::analyze() {
    Result isSemanticallyValid = Result::Success;
    if (collectTypes() == Result::Failure) { isSemanticallyValid = Result::Failure; };
    if (collectGlobals() == Result::Failure) { isSemanticallyValid = Result::Failure; };
    if (collectAndSpecializeGenerics() == Result::Failure) { isSemanticallyValid = Result::Failure; }
    // Don't want errors cascading because of conflicting redeclarations
    if (isSemanticallyValid == Result::Failure) { return isSemanticallyValid; }

    symbolTable.switchToCheckingMode();
    isSemanticallyValid = checkStatements();
    return isSemanticallyValid;
}

// Placeholders to satisfy the linker
Result analyzer::collectGlobals() { return Result::Success; };
Result analyzer::collectAndSpecializeGenerics() { return Result::Success; };

Result analyzer::checkStatements() {  // semantic analysis pass (this can also check the generic specializations)
    Result programIsSemanticallyValid = Result::Success;
    for (auto& stmt : parsedFile.program) {
        if (this->visit(stmt) == Result::Failure) { programIsSemanticallyValid = Result::Failure; }
    }
    return programIsSemanticallyValid;
}

auto analyzer::areTypesCompatible(const SemanticType* from, const SemanticType* to) const -> typeCompatibilityResult {
    using result_t = typeCompatibilityResult::result_t;
    // Null pointer means something went wrong in type deduction
    if (!from || !to) { return {.result = result_t::Error, .message = "Could not deduce types"}; }

    // Duplicated types point to the same underlying value so we can just do a fast pointer comparison
    if (from == to) { return {.result = result_t::Valid}; }

    const std::string conversionError = std::format("Cannot convert {} to {}", from->toString(), to->toString());

    // Totally distinct types are not interconvertible
    if (from->kind != to->kind) { return {.result = result_t::Error, .message = conversionError}; }

    // Same structure but different instances (e.g. pointers to different types)
    switch (from->kind) {
        case Kind::Aggregate:
            return {.result = result_t::Error,
                    .message = conversionError + " (aggregates cannot be converted to other types)."};
        case Kind::Array: {
            auto* arrFrom = static_cast<const Array*>(from);
            auto* arrTo = static_cast<const Array*>(to);
            if (arrFrom->length != arrTo->length) {
                return {.result = result_t::Error,
                        .message = conversionError + " (cannot convert between arrays of different lengths)."};
            }
            auto baseCompatible = areTypesCompatible(arrFrom->elementType, arrTo->elementType);
            if (!baseCompatible) {
                return {.result = result_t::Error,
                        .message = conversionError
                            + std::format(" (cannot convert an array of {} to an array of {})",
                                          arrFrom->elementType->toString(), arrTo->elementType->toString())};
            }
            return baseCompatible;
        } break;

        case Kind::Function: {
            auto* funcFrom = static_cast<const Function*>(from);
            auto* funcTo = static_cast<const Function*>(to);
            if (funcFrom->parameterTypes.size() != funcTo->parameterTypes.size()) {
                return {.result = result_t::Error, .message = conversionError + " (different number of parameters)."};
            }
            for (std::size_t i = 0; i < funcFrom->parameterTypes.size(); ++i) {
                const Parameter& funcFromParam = funcFrom->parameterTypes[i];
                const Parameter& funcToParam = funcTo->parameterTypes[i];
                if (!areTypesCompatible(funcFromParam.type, funcToParam.type)) {
                    const std::string error = std::format(
                        " (mismatch in position {}: parameter type {} is cannot be converted to parameter type {}).", i,
                        funcFromParam.toString(), funcToParam.toString());

                    return {.result = result_t::Error, .message = conversionError + error};
                };
                if (funcFromParam.isMutable != funcToParam.isMutable) {
                    std::string error
                        = std::format(" (parameter in position {} in {} is {} but is {} in {})", i,
                                      funcFrom->toString(), (funcFromParam.isMutable ? "mutable" : "immutable"),
                                      (funcToParam.isMutable ? "mutable" : "immutable"), funcToParam.toString());

                    return {.result = result_t::Error, .message = conversionError + error};
                }
            }
            return areTypesCompatible(funcFrom->returnType, funcTo->returnType);
        } break;

        case Kind::Generic: {
            auto* genericFrom = static_cast<const GenericInstance*>(from);
            auto* genericTo = static_cast<const GenericInstance*>(to);
            if (genericFrom->baseType != genericTo->baseType) { return {.result = result_t::Error}; }
            if (genericFrom->typeArguments.size() != genericTo->typeArguments.size()) {
                return {.result = result_t::Error,
                        .message = conversionError + " (different number of type parameters)."};
            }
            for (size_t i = 0; i < genericFrom->typeArguments.size(); ++i) {
                const SemanticType* fromArgument = genericFrom->typeArguments[i];
                const SemanticType* toArgument = genericTo->typeArguments[i];
                if (!areTypesCompatible(fromArgument, toArgument)) {
                    return {.result = result_t::Error,
                            .message = conversionError
                                + std::format(" (mismatch in position {}: {} cannot convert to {})", i,
                                              fromArgument->toString(), toArgument->toString())};
                }
            }
            return {.result = result_t::Valid};
        };

        case Kind::Pointer: {
            auto* ptrFrom = static_cast<const Pointer*>(from);
            auto* ptrTo = static_cast<const Pointer*>(to);
            // making an immutable pointer (ptr int) mutable (ptr mut int) is not allowed
            // but making a mutable pointer (ptr mut int) mutable (ptr int) is fine
            if (!ptrFrom->isMutable && ptrTo->isMutable) {
                return {.result = result_t::Error,
                        .message = conversionError + " (cannot convert an immutable pointer to a mutable pointer)."};
            }
            return areTypesCompatible(ptrFrom->baseType, ptrTo->baseType);
        }; break;

        case Kind::Primitive: {
            return {.result = result_t::Error};
        }; break;
        default: ASSERT_UNREACHABLE("Unknown semantic type kind in areTypesCompatible");
    }
}

}  // namespace semantic
}  // namespace Manganese