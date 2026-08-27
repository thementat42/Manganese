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
            return false;  // TODO
        }
        default: return false;
    }
}

}  // namespace Manganese::semantic
