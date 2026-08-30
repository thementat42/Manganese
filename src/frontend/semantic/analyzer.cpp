#include <core.hpp>
#include <frontend/lexer/token.hpp>
#include <frontend/semantic/analyzer.hpp>
#include <frontend/semantic/type_context.hpp>
#include <utils/result.hpp>

namespace Manganese::semantic {

Result Analyzer::analyze() {
    // Don't want errors cascading because of conflicting redeclarations
    if (buildScopeTree() == Result::Failure) { return Result::Failure; }
    symbolTable.switchToCheckingMode();
    if (collectGlobals() == Result::Failure) { return Result::Failure; }
    return checkStatements();
}

const Symbol* Analyzer::resolveTypeSymbol(const ast::Type* typeNode) {
    if (!typeNode) return nullptr;

    if (typeNode->kind == ast::TypeKind::SymbolType) {
        const auto* symbolType = static_cast<const ast::SymbolType*>(typeNode);
        return symbolTable.lookup(symbolType->name);
    }

    if (typeNode->kind == ast::TypeKind::ScopedType) {
        const auto* scopedType = static_cast<const ast::ScopedType*>(typeNode);
        const Symbol* parentSymbol = resolveTypeSymbol(scopedType->scope);
        if (!parentSymbol || !parentSymbol->scopeDefined) {
            return nullptr;  // Parent symbol wasn't found or doesn't have a scope
        }

        if (scopedType->type->kind != ast::TypeKind::SymbolType) {
            ASSERT_UNREACHABLE(
                "In resolveTypeSymbol: base case for recursion of a scoped type resolution should be an identifier");
        }
        const auto* memberSymbolType = static_cast<const ast::SymbolType*>(scopedType->type);
        return symbolTable.scopedLookup(parentSymbol->scopeDefined, memberSymbolType->name);
    }

    return nullptr;
}

const Symbol* Analyzer::resolveScopeSymbol(const ast::Expression* expr) {
    if (!expr) { return nullptr; }
    if (expr->kind == ast::ExpressionKind::IdentifierExpression) {
        const auto* id = static_cast<const ast::IdentifierExpression*>(expr);
        return symbolTable.lookup(id->name);
    }
    if (expr->kind == ast::ExpressionKind::ScopeResolutionExpression) {
        const auto* scopeExpr = static_cast<const ast::ScopeResolutionExpression*>(expr);

        // recursively check the left hand side
        const Symbol* parentSymbol = resolveScopeSymbol(scopeExpr->scope);
        if (!parentSymbol || !parentSymbol->scopeDefined) { return nullptr; }

        if (scopeExpr->element->kind != ast::ExpressionKind::IdentifierExpression) { return nullptr; }
        const auto* memberId = static_cast<const ast::IdentifierExpression*>(scopeExpr->element);

        return symbolTable.scopedLookup(parentSymbol->scopeDefined, memberId->name);
    }
    return nullptr;
}

bool Analyzer::isMutableExpression(const ast::Expression* expr) {
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
            const Symbol* scopeSymbol = resolveScopeSymbol(scope->scope);
            if (!scopeSymbol || !scopeSymbol->scopeDefined) { return false; }
            if (scope->element->kind != ast::ExpressionKind::IdentifierExpression) { return false; }
            const auto* memberId = static_cast<const ast::IdentifierExpression*>(scope->element);
            const Symbol* memberSymbol = symbolTable.scopedLookup(scopeSymbol->scopeDefined, memberId->name);
            return memberSymbol ? memberSymbol->isMutable : false;
        }
        default: return false;
    }
}

}  // namespace Manganese::semantic
