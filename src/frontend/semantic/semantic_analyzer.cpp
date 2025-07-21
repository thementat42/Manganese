/**
 * @file semantic_analyzer.cpp
 * @brief Implementation of some core methods for the semantic analyzer
 */

#include <frontend/ast.h>
#include <frontend/semantic/semantic_analyzer.h>
#include <global_macros.h>

#include <format>

namespace Manganese {
namespace semantic {

void SemanticAnalyzer::analyze(const parser::ParsedFile& parsedFile) {
    // checkImports(parsedFile.imports);
    currentModule = parsedFile.moduleName;
    for (const auto& statement : parsedFile.program) {
        checkStatement(statement.get());
    }
}

bool SemanticAnalyzer::areTypesCompatible(const ast::Type* type1, const ast::Type* type2) const noexcept_if_release {
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
            if (!areTypesCompatible(type1Array->elementType.get(), type2Array->elementType.get())) {
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
            if (!areTypesCompatible(type1Func->returnType.get(), type2Func->returnType.get())) {
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
                if (!areTypesCompatible(type1Params[i].type.get(), type2Params[i].type.get())) {
                    return false;
                }
            }
            return true;
        }
        case TypeKind::GenericType: {
            auto type1Generic = static_cast<const ast::GenericType*>(type1);
            auto type2Generic = static_cast<const ast::GenericType*>(type2);
            if (!areTypesCompatible(type1Generic->baseType.get(), type2Generic->baseType.get())) {
                return false;
            }
            const auto& type1Params = type1Generic->typeParameters;
            const auto& type2Params = type2Generic->typeParameters;
            if (type1Params.size() != type2Params.size()) {
                return false;  // Different number of type parameters
            }
            for (size_t i = 0; i < type1Params.size(); ++i) {
                if (!areTypesCompatible(type1Params[i].get(), type2Params[i].get())) {
                    return false;  // Mismatched type parameters
                }
            }
            return true;  // All type parameters match
        }
        case TypeKind::PointerType:
            return areTypesCompatible(
                static_cast<const ast::PointerType*>(type1)->baseType.get(),
                static_cast<const ast::PointerType*>(type2)->baseType.get());
        case TypeKind::SymbolType:
            return static_cast<const ast::SymbolType*>(type1)->name == static_cast<const ast::SymbolType*>(type2)->name;
        default:
            ASSERT_UNREACHABLE(std::format("No type compatibility check for type kind {}", static_cast<int>(type1->kind())));
    }
}

}  // namespace semantic
}  // namespace Manganese
