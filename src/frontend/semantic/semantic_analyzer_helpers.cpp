/**
 * @file semantic_analyzer_helpers.cpp
 * @brief Helper functions for semantic analysis of AST nodes
 */

#include <frontend/ast.h>
#include <frontend/semantic/semantic_analyzer.h>
#include <frontend/semantic/semantic_type_helpers.h>
#include <global_macros.h>

namespace Manganese {

namespace semantic {

bool SemanticAnalyzer::typeExists(const ast::TypeSPtr_t& type) {
    using ast::TypeKind;
    if (!type) {
        return false;
    }
    switch (type->kind()) {
        case TypeKind::SymbolType: {
            if (ast::isPrimitiveType(type)) {
                return true;
            }
            ast::SymbolType* symbolType = static_cast<ast::SymbolType*>(type.get());
            Symbol* symbol = symbolTable.lookupInCurrentScope(symbolType->getName());
            return symbol != nullptr;
        }
        case TypeKind::ArrayType: {
            auto* arrType = static_cast<const ast::ArrayType*>(type.get());
            return typeExists(arrType->elementType);
        }
        case TypeKind::FunctionType: {
            logError("Aliasing function types is not currently supported", type.get());
            return false;
            // // e.g. alias func(int) -> int as fii;
            // // not aliasing a function declaration
            // auto* funcType = static_cast<const ast::FunctionType*>(type.get());
            // bool isValid = true;
            // if (funcType->returnType) {
            //     isValid = typeExists(funcType->returnType);
            // }
            // for (const auto& paramType : funcType->parameterTypes) {
            // }
        }
        case TypeKind::PointerType: {
            auto* ptrType = static_cast<const ast::PointerType*>(type.get());
            return typeExists(ptrType->baseType);
        }

        case TypeKind::GenericType:
            logError("Generic types cannot be aliased", type.get());
            return false;

        default:
            ASSERT_UNREACHABLE(
                std::format("Cannot alias type of kind {}",
                            static_cast<int>(type->kind())));
            return false;
    }
}

bool SemanticAnalyzer::areTypesEqual(const ast::Type* type1, const ast::Type* type2) const noexcept_if_release {
    if (!type1 || !type2) {
        return false;  // If either type is null, they are not compatible
    }
    if (type1 == type2) {
        return true;  // Same type instance is always compatible
    }
    if (type1->kind() != type2->kind()) {
        return false;  // Different kinds of types are not compatible
    }

    using ast::TypeKind;

    // Static casts are ok here since we know they're of the same kind
    switch (type1->kind()) {
        case TypeKind::ArrayType: {
            auto type1Array = static_cast<const ast::ArrayType*>(type1);
            auto type2Array = static_cast<const ast::ArrayType*>(type2);
            if (!areTypesEqual(type1Array->elementType.get(), type2Array->elementType.get())) {
                return false;  // Element types must match
            }
            if (type1Array->lengthExpression && type2Array->lengthExpression) {
                // If both have length expressions, they must be equal
                // TODO: Replace with expression equality check
                return type1Array->lengthExpression->toString() == type2Array->lengthExpression->toString();
            }
            return true;
        }
        case TypeKind::FunctionType: {
            auto type1Func = static_cast<const ast::FunctionType*>(type1);
            auto type2Func = static_cast<const ast::FunctionType*>(type2);
            if (!areTypesEqual(type1Func->returnType.get(), type2Func->returnType.get())) {
                return false;  // Return types must match
            }
            const auto& type1Params = type1Func->parameterTypes;
            const auto& type2Params = type2Func->parameterTypes;
            if (type1Params.size() != type2Params.size()) {
                return false;  // Different number of parameters
            }
            for (size_t i = 0; i < type1Params.size(); ++i) {
                if (type1Params[i].isConst != type2Params[i].isConst) {
                    return false;
                }
                if (!areTypesEqual(type1Params[i].type.get(), type2Params[i].type.get())) {
                    return false;
                }
            }
            return true;
        }
        case TypeKind::GenericType: {
            auto type1Generic = static_cast<const ast::GenericType*>(type1);
            auto type2Generic = static_cast<const ast::GenericType*>(type2);
            if (!areTypesEqual(type1Generic->baseType.get(), type2Generic->baseType.get())) {
                return false;
            }
            const auto& type1Params = type1Generic->typeParameters;
            const auto& type2Params = type2Generic->typeParameters;
            if (type1Params.size() != type2Params.size()) {
                return false;  // Different number of type parameters
            }
            for (size_t i = 0; i < type1Params.size(); ++i) {
                if (!areTypesEqual(type1Params[i].get(), type2Params[i].get())) {
                    return false;  // Mismatched type parameters
                }
            }
            return true;  // All type parameters match
        }
        case TypeKind::PointerType:
            return areTypesEqual(
                static_cast<const ast::PointerType*>(type1)->baseType.get(),
                static_cast<const ast::PointerType*>(type2)->baseType.get());
        case TypeKind::SymbolType:
            return static_cast<const ast::SymbolType*>(type1)->name == static_cast<const ast::SymbolType*>(type2)->name;
        default:
            ASSERT_UNREACHABLE(std::format("No type compatibility check for type kind {}", static_cast<int>(type1->kind())));
    }
}

bool SemanticAnalyzer::areTypesPromotableOrDemotable(const ast::Type* from, const ast::Type* to) const noexcept_if_release {
    if (!from || !to) {
        return false;  // If either type is null, they are not compatible
    }
    if (!ast::isPrimitiveType(from) || !ast::isPrimitiveType(to)) {
        return false;  // Only primitive types can be promoted or demoted
    }

    std::string fromName = static_cast<const ast::SymbolType*>(from)->getName();
    std::string toName = static_cast<const ast::SymbolType*>(to)->getName();

    if (fromName == toName) {
        return true;  // Same type is always compatible
    }
    if (validImplicitConversions.find(fromName) != validImplicitConversions.end() &&
        validImplicitConversions.at(fromName) == toName) {
        // Check if the from type can be promoted to the to type
        return true;
    }
    if (validImplicitConversionsWithWarnings.find(fromName) != validImplicitConversionsWithWarnings.end() && validImplicitConversionsWithWarnings.at(fromName) == toName) {
        logging::logWarning(
            std::format("Implicit conversion from {} to {} may result in data loss", fromName, toName));
        return true;
    }
    return false;  // No valid promotion or demotion found
}

}  // namespace semantic

}  // namespace Manganese