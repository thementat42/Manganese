#include <frontend/semantic/semantic_analyzer.h>

namespace Manganese {

namespace semantic {

void SemanticAnalyzer::checkRepeatLoopStatement(ast::RepeatLoopStatement* statement) {
    DISCARD(statement);
    NOT_IMPLEMENTED;
}

void SemanticAnalyzer::checkWhileLoopStatement(ast::WhileLoopStatement* statement) {
    checkExpression(statement->condition.get());
    if (!ast::isPrimitiveType(statement->condition->getType())) {
        logError("Could not convert {} to a boolean", statement, statement->condition->toString());
    }
    enterScope();
    ++context.whileLoop;
    checkBlock(statement->body);
    --context.whileLoop;
    exitScope();
}

}  // namespace semantic

}  // namespace Manganese