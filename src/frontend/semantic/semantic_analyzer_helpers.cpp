/**
 * @file semantic_analyzer_helpers.cpp
 * @brief Helper functions for semantic analysis of AST nodes
 */

#include <frontend/ast.hpp>
#include <frontend/semantic/semantic_analyzer.hpp>
#include <frontend/semantic/semantic_type_helpers.hpp>
#include <global_macros.hpp>

namespace Manganese {
namespace semantic {

bool SemanticAnalyzer::areTypesPromotableOrDemotable(const ast::Type* from,
                                                     const ast::Type* to) const noexcept_if_release {
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
    if (validImplicitConversions.find(fromName) != validImplicitConversions.end()
        && validImplicitConversions.at(fromName) == toName) {
        // Check if the from type can be promoted to the to type
        return true;
    }
    if (validImplicitConversionsWithWarnings.find(fromName) != validImplicitConversionsWithWarnings.end()
        && validImplicitConversionsWithWarnings.at(fromName) == toName) {
        logging::logWarning(std::format("Implicit conversion from {} to {} may result in data loss", fromName, toName));
        return true;
    }
    return false;  // No valid promotion or demotion found
}

const ast::Type* SemanticAnalyzer::resolveAlias(const ast::Type* type) const noexcept_if_release {
    if (!type) return nullptr;
    // Only SymbolType can be aliased
    if (type->kind() == ast::TypeKind::SymbolType) {
        const auto* symbolType = static_cast<const ast::SymbolType*>(type);
        const Symbol* symbol = symbolTable.lookup(symbolType->getName());
        if (symbol && symbol->kind == SymbolKind::TypeAlias && symbol->type.get() != type) {
            // Recursively resolve in case of chained aliases
            return resolveAlias(symbol->type.get());
        }
    }
    return type;
}

bool SemanticAnalyzer::typeExists(const ast::TypeSPtr_t& type) {
    using ast::TypeKind;
    if (!type) { return false; }
    switch (type->kind()) {
        case TypeKind::SymbolType: {
            if (ast::isPrimitiveType(type)) { return true; }
            ast::SymbolType* symbolType = static_cast<ast::SymbolType*>(type.get());
            const Symbol* symbol = symbolTable.lookupInCurrentScope(symbolType->getName());
            return symbol != nullptr;
        }
        case TypeKind::ArrayType: {
            auto* arrType = static_cast<const ast::ArrayType*>(type.get());
            return typeExists(arrType->elementType);
        }
        case TypeKind::FunctionType: {
            // e.g. let x: func(int) -> int = foo (some function with that signature);
            // this doesn't apply to function declarations.
            auto* funcType = static_cast<const ast::FunctionType*>(type.get());
            if (funcType->returnType && !typeExists(funcType->returnType)) {
                return false;  // A non-null return type must exist
            }
            for (const auto& paramType : funcType->parameterTypes) {
                if (!typeExists(paramType.type)) {
                    return false;  // All parameter types must exist
                }
            }
            return true;  // Function type is valid if all parameter types and return type exist
        }
        case TypeKind::PointerType: {
            auto* ptrType = static_cast<const ast::PointerType*>(type.get());
            return typeExists(ptrType->baseType);
        }

        case TypeKind::GenericType:
            ASSERT_UNREACHABLE("Existence of generic types cannot be checked in typeExists. Perform a different check")
            return false;

        default: ASSERT_UNREACHABLE(std::format("Unknown type kind {}", static_cast<int>(type->kind()))); return false;
    }
}

}  // namespace semantic

}  // namespace Manganese