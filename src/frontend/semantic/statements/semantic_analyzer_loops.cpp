#include <frontend/ast.hpp>
#include <frontend/semantic/semantic_analyzer.hpp>
#include "frontend/ast/ast_base.hpp"


namespace Manganese {

namespace semantic {

void SemanticAnalyzer::checkRepeatLoopStatement(ast::RepeatLoopStatement* statement) {
    checkExpression(statement->numIterations.get());
    const ast::Type* iterationType = statement->numIterations->getType();

    if (!ast::isPrimitiveType(iterationType)) {
        logError("Repeat loop iteration count must be a numeric type, not {}", statement,
                 ast::toStringOr(iterationType));
        return;
    }
    if (!isAnyInt(iterationType)) {
        logError("Repeat loops must have an integer type, not {}", statement, ast::toStringOr(iterationType));
        // if (isFloat(iterationType)) {
        //     logWarning("Repeat loop iteration count should be an integer, not {}. Value will be rounded up",
        //     statement,
        //                ast::toStringOr(iterationType));
        // ! Round up
        // } else {
        //     logError("Repeat loop iteration count must be an integer type, not {}", statement,
        //              ast::toStringOr(iterationType));
        //     return;
        // }
    }
    enterScope();
    ++context.repeatLoop;
    checkBlock(statement->body);
    --context.repeatLoop;
    exitScope();
}

void SemanticAnalyzer::checkWhileLoopStatement(ast::WhileLoopStatement* statement) {
    checkExpression(statement->condition.get());
    if (!ast::isPrimitiveType(statement->condition->getType())) {
        logError("Could not convert {} to a boolean", statement, ast::toStringOr(statement->condition));
    }
    enterScope();
    ++context.whileLoop;
    checkBlock(statement->body);
    --context.whileLoop;
    exitScope();
}

}  // namespace semantic

}  // namespace Manganese