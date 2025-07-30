#include <frontend/semantic/semantic_analyzer.h>

namespace Manganese {

namespace semantic {

void SemanticAnalyzer::checkBreakStatement(ast::BreakStatement* statement) {
    if (!context.isLoopContext() && !context.isSwitchContext()) {
        logError("break statements can only be used inside loops or switch statements", statement);
    }
}

void SemanticAnalyzer::checkContinueStatement(ast::ContinueStatement* statement) {
    if (!context.isLoopContext()) {
        logError("continue statements can only be used inside loops", statement);
    }
}

void SemanticAnalyzer::checkIfStatement(ast::IfStatement* statement) {
    checkExpression(statement->condition.get());
    if (!ast::isPrimitiveType(statement->condition->getType())) {
        logError("Could not convert {} to a boolean", statement, statement->condition->toString());
    }
    enterScope();
    ++context.ifStatement;
    checkBlock(statement->body);
    --context.ifStatement;
    exitScope();
    if (!statement->elifs.empty()) {
        for (auto& elif: statement->elifs) {
            checkExpression(elif.condition.get());
            if (!ast::isPrimitiveType(elif.condition->getType())) {
                logError("Could not convert {} to a boolean", statement, elif.condition->toString());
            }
            enterScope();
            ++context.ifStatement;
            checkBlock(elif.body);
            --context.ifStatement;
            exitScope();
        }
    }
    if (!statement->elseBody.empty()) {
        enterScope();
        ++context.ifStatement;
        checkBlock(statement->elseBody);
        --context.ifStatement;
        exitScope();
    }
}

void SemanticAnalyzer::checkReturnStatement(ast::ReturnStatement* statement) {
    if (!context.isFunctionContext()) {
        logError("return statements can only be used inside functions", statement);
    }
    checkExpression(statement->value.get());

    // Function is null, return is null -- ok
    // Function is null, return is not null -- error
    // Function is not null, return is not null -- ok
    // Function is not null, return is null -- error

    auto returnType = context.currentFunctionReturnType;

    if (returnType) {
        auto returnedType = statement->value->getType();
        if (!areTypesCompatible(returnedType, returnType.get())) {

            logError("Return type mismatch in function: expected {}, got {}",
                     statement, returnType->toString(),
                     (returnedType ? returnedType->toString() : "no return type"));
        }
    } else {
        if (statement->value) {
            logError("Function does not have a return type, but this returns a {}",
                     statement, statement->value->getType()->toString());
        }
        return;  // Both the function and the return value are null, so the types are compatible
    }
}
void SemanticAnalyzer::checkSwitchStatement(ast::SwitchStatement* statement) {
    DISCARD(statement);
        NOT_IMPLEMENTED;
}

}  // namespace semantic

}  // namespace Manganese