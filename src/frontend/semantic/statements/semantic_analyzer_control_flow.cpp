#include <frontend/semantic/semantic_analyzer.hpp>

namespace Manganese {
using ast::toStringOr;
namespace semantic {

void SemanticAnalyzer::checkBreakStatement(ast::BreakStatement* statement) {
    if (!context.isLoopContext() && !context.isSwitchContext()) {
        logError("break statements can only be used inside loops or switch statements", statement);
    }
}

void SemanticAnalyzer::checkContinueStatement(ast::ContinueStatement* statement) {
    if (!context.isLoopContext()) { logError("continue statements can only be used inside loops", statement); }
}

void SemanticAnalyzer::checkIfStatement(ast::IfStatement* statement) {
    checkExpression(statement->condition.get());
    if (!ast::isPrimitiveType(statement->condition->getType())) {
        logError("Could not convert {} to a boolean", statement, toStringOr(statement->condition));
    }
    enterScope();
    ++context.ifStatement;
    checkBlock(statement->body);
    --context.ifStatement;
    exitScope();

    for (auto& elif : statement->elifs) {
        checkExpression(elif.condition.get());
        if (!ast::isPrimitiveType(elif.condition->getType())) {
            logError("Could not convert {} to a boolean", statement, toStringOr(elif.condition));
        }
        enterScope();
        ++context.ifStatement;
        checkBlock(elif.body);
        --context.ifStatement;
        exitScope();
    }

    enterScope();
    ++context.ifStatement;
    checkBlock(statement->elseBody);
    --context.ifStatement;
    exitScope();
}

void SemanticAnalyzer::checkReturnStatement(ast::ReturnStatement* statement) {
    if (!context.isFunctionContext()) { logError("return statements can only be used inside functions", statement); }
    checkExpression(statement->value.get());

    // Function is null, return is null -- ok
    // Function is null, return is not null -- error
    // Function is not null, return is not null -- ok
    // Function is not null, return is null -- error

    auto returnType = context.currentFunctionReturnType;

    if (returnType) {
        auto returnedType = statement->value->getType();
        if (!areTypesCompatible(returnedType, returnType.get())) {
            logError("Return type mismatch in function: expected {}, got {}", statement, toStringOr(returnType),
            toStringOr(returnedType, "no return type"));
        }
    } else {
        if (statement->value) {
            logError("Function does not have a return type, but this returns a {}", statement,
                     toStringOr(statement->value->getType()));
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