#include <frontend/semantic/semantic_analyzer.hpp>

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

    // TODO: Bounds checking
}

void SemanticAnalyzer::checkMemberAccessExpression(ast::MemberAccessExpression* expression) {
    /*
    1. Check the object whose member is being accessed (`foo` in `foo.bar`)
    2. Check that it has a computed type which is a SymbolType (e.g., you can't access a member of a function)
    3. Look up the symbol (`foo` in `foo.bar`) in the symbol table
    4. Check that the symbol is an aggregate (e.g., you can't access a member of an int32)
    5. Check that the member exists in the aggregate
    6. Set the type of the expression to the type of the member
    */
    checkExpression(expression->object.get());
    ast::Type* currentType = expression->object->getType();
    if (!currentType) {
        logError("Cannot access member {} of object {} (it either does not exist or has no computed type)", expression,
                 expression->property, toStringOr(expression->object));
        return;
    }
    if (currentType->kind() != ast::TypeKind::SymbolType) {
        logError("Cannot access member of an object of type {}", expression, toStringOr(currentType));
        return;
    }
    auto* symbolType = static_cast<ast::SymbolType*>(currentType);
    const Symbol* symbol = symbolTable.lookup(symbolType->name);
    if (!symbol) {
        logError("Symbol '{}' was not declared in any scope", expression, symbolType->name);
        return;
    }
    if (symbol->kind != SymbolKind::Aggregate) {
        logError("Cannot access member '{}' of symbol '{}' since it is not an aggregate", expression, expression->property,
                 symbolType->name);
        return;
    }
    auto aggregateDeclaration = static_cast<ast::AggregateDeclarationStatement*>(symbol->declarationNode);

    auto findFieldByName = [&expression](const ast::AggregateField& field) { return field.name == expression->property; };
    auto aggregateField = std::find_if(aggregateDeclaration->fields.begin(), aggregateDeclaration->fields.end(), findFieldByName);
    if (aggregateField == aggregateDeclaration->fields.end()) {
        logError("{} has no member named '{}'. Its aggregate type is:\n{}", expression, toStringOr(expression->object),
                 expression->property, toStringOr(aggregateDeclaration));
        return;
    }
    expression->setType(aggregateField->type);
}

void SemanticAnalyzer::checkScopeResolutionExpression(ast::ScopeResolutionExpression* expression) {
    DISCARD(expression);
    NOT_IMPLEMENTED;
}

}  // namespace semantic

}  // namespace Manganese