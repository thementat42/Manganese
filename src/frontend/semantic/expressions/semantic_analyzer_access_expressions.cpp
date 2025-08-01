#include <frontend/semantic/semantic_analyzer.h>

namespace Manganese {

using ast::toStringOr;

namespace semantic {

void SemanticAnalyzer::checkIndexExpression(ast::IndexExpression* expression) {
    checkExpression(expression->variable.get());
    checkExpression(expression->index.get());
    ast::Type* currentType = expression->variable->getType();
    if (!currentType) {
        logError("Cannot index into variable {} -- it has no computed type", expression,
                 toStringOr(expression->variable));
        return;
    }
    if (currentType->kind() != ast::TypeKind::ArrayType) {
        logError("{} cannot be indexed since it is of type {}, not an array type", expression,
                 toStringOr(expression->variable), toStringOr(currentType));
        return;
    }

    auto* arrayType = static_cast<ast::ArrayType*>(currentType);
    expression->setType(arrayType->elementType);

    // ===== Bounds Checking =====
    ast::Expression* lengthExpression = arrayType->lengthExpression.get();
    if (!lengthExpression) {
        logWarning("Indexing into an array of unknown length: {}. This may lead to out-of-bounds access", expression,
                   toStringOr(expression->variable));
    }
    ast::Expression* indexValue = expression->index.get();
    if (indexValue->kind() != ast::ExpressionKind::NumberLiteralExpression) {
        logError("Indexing into an array requires a numeric index; {} cannot be used as an index", expression,
                 toStringOr(indexValue));
        return;
    }
}

void SemanticAnalyzer::checkMemberAccessExpression(ast::MemberAccessExpression* expression) {
    DISCARD(expression);
    NOT_IMPLEMENTED;
}

void SemanticAnalyzer::checkScopeResolutionExpression(ast::ScopeResolutionExpression* expression) {
    DISCARD(expression);
    NOT_IMPLEMENTED;
}

}  // namespace semantic

}  // namespace Manganese