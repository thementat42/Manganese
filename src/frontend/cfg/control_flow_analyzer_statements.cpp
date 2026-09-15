#include <format>
#include <frontend/ast.hpp>
#include <frontend/cfg/control_flow_analyzer.hpp>
#include <frontend/semantic/symbol_table.hpp>
#include <utils/expression_folding.hpp>

namespace Manganese::cfg {

FlowStatus ControlFlowAnalyzer::visit(const ast::AggregateDeclarationStatement* /*unused*/) noexcept {
    return FlowStatus::FallsThrough;
}

FlowStatus ControlFlowAnalyzer::visit(const ast::AliasStatement* /*unused*/) noexcept {
    return FlowStatus::FallsThrough;
}

FlowStatus ControlFlowAnalyzer::visit(const ast::BreakStatement* /*unused*/) noexcept { return FlowStatus::Breaks; }

FlowStatus ControlFlowAnalyzer::visit(const ast::ContinueStatement* /*unused*/) noexcept {
    return FlowStatus::ContinuesLoop;
}

FlowStatus ControlFlowAnalyzer::visit(const ast::EmptyStatement* /*unused*/) noexcept {
    return FlowStatus::FallsThrough;
}

FlowStatus ControlFlowAnalyzer::visit(const ast::EnumDeclarationStatement* /*unused*/) noexcept {
    return FlowStatus::FallsThrough;
}

FlowStatus ControlFlowAnalyzer::visit(const ast::ExpressionStatement* statement) noexcept {
    visit(statement->expression);
    return FlowStatus::FallsThrough;
}

FlowStatus ControlFlowAnalyzer::visit(const ast::ForLoopStatement* statement) noexcept {
    const bool shouldEnterScope = statement->initializationStep == nullptr;
    if (statement->initializationStep != nullptr) {
        symbolTable.enterScope();
        visit(statement->initializationStep);
    }
    if (statement->postExpression != nullptr) { visit(statement->postExpression); }
    if (statement->stopCondition != nullptr) { visit(statement->stopCondition); }

    // snapshot after parameters so initialization variable is correctly marked as initialized
    auto preLoopStates = symbolAssignmentStates;

    if (statement->stopCondition == nullptr) {
        FlowStatus bodyStatus = visit(statement->body, shouldEnterScope);

        // symbol assignment states updated by loop body visitor

        mergeStates(symbolAssignmentStates, preLoopStates);

        if (bodyStatus == FlowStatus::Returns) { return FlowStatus::Returns; }
        if (bodyStatus == FlowStatus::Breaks) { return FlowStatus::FallsThrough; }
        return FlowStatus::InfiniteLoop;
    }

    auto conditionValue = utils::computeExpression<bool>(
        statement->stopCondition, targetInfo,
        [this]<class... Args>(const auto* expr, std::format_string<Args...> fmt, Args&&... args) {
            this->logError(expr, fmt, std::forward<Args>(args)...);
        });

    if (conditionValue.has_value()) {
        // never executes
        if (!*conditionValue) { return FlowStatus::FallsThrough; }

        FlowStatus bodyStatus = visit(statement->body, shouldEnterScope);
        mergeStates(symbolAssignmentStates, preLoopStates);

        if (bodyStatus == FlowStatus::Returns) { return FlowStatus::Returns; }
        if (bodyStatus == FlowStatus::Breaks) { return FlowStatus::FallsThrough; }
        return FlowStatus::InfiniteLoop;
    }

    // Couldn't fold value, runtime loop
    visit(statement->body, shouldEnterScope);

    mergeStates(symbolAssignmentStates, preLoopStates);

    return FlowStatus::FallsThrough;
}

FlowStatus ControlFlowAnalyzer::visit(const ast::FunctionDeclarationStatement* statement) noexcept {
    FlowStatus bodyStatus = visit(statement->body);
    if (!statement->returnType->semanticType->isVoid() && bodyStatus == FlowStatus::FallsThrough) {
        logError(statement, "non-void function '{}' does not return a value on all control paths", statement->name);
    }
    return FlowStatus::FallsThrough;  // A function declaration itself doesn't affect control flow
}

FlowStatus ControlFlowAnalyzer::visit(const ast::IfStatement* statement) noexcept {
    visit(statement->condition);
    auto preBranchStates = symbolAssignmentStates;

    FlowStatus ifBodyStatus = visit(statement->body);
    auto ifStates = symbolAssignmentStates;
    bool allBranchesTerminate = (ifBodyStatus != FlowStatus::FallsThrough);
    FlowStatus result = ifBodyStatus;

    auto mergedStates = ifStates;

    for (const auto& elif : statement->elifs) {
        symbolAssignmentStates = preBranchStates;
        FlowStatus elifStatus = visit(elif.body);
        if (elifStatus == FlowStatus::FallsThrough || (allBranchesTerminate && elifStatus != result)) {
            allBranchesTerminate = false;
        }
        mergeStates(mergedStates, symbolAssignmentStates);
    }

    if (statement->elseBody.empty()) {
        allBranchesTerminate = false;
        mergeStates(mergedStates, preBranchStates);

    } else {
        symbolAssignmentStates = preBranchStates;
        FlowStatus elseStatus = visit(statement->elseBody);
        if ((elseStatus == FlowStatus::FallsThrough) || (allBranchesTerminate && elseStatus != result)) {
            allBranchesTerminate = false;
        }
        mergeStates(mergedStates, symbolAssignmentStates);
    }
    symbolAssignmentStates = std::move(mergedStates);
    return allBranchesTerminate ? result : FlowStatus::FallsThrough;
}

FlowStatus ControlFlowAnalyzer::visit(const ast::ImportStatement* /*unused*/) noexcept {
    return FlowStatus::FallsThrough;
}

FlowStatus ControlFlowAnalyzer::visit(const ast::ModuleDeclarationStatement* /*unused*/) noexcept {
    return FlowStatus::FallsThrough;
}

FlowStatus ControlFlowAnalyzer::visit(const ast::NamespaceStatement* statement) noexcept {
    visit(statement->block);
    return FlowStatus::FallsThrough;
}

FlowStatus ControlFlowAnalyzer::visit(const ast::NestedBlockStatement* statement) noexcept {
    return visit(statement->block);
}

FlowStatus ControlFlowAnalyzer::visit(const ast::ReturnStatement* /*unused*/) noexcept { return FlowStatus::Returns; }

FlowStatus ControlFlowAnalyzer::visit(const ast::SwitchStatement* statement) noexcept {
    visit(statement->target);
    auto preSwitchStates = symbolAssignmentStates;
    states_t mergedStates;

    const bool hasDefault = statement->defaultBody.empty();
    bool allCasesTerminate = hasDefault;

    if (hasDefault) {
        FlowStatus defaultStatus = visit(statement->defaultBody);
        mergedStates = symbolAssignmentStates;
        if (defaultStatus == FlowStatus::FallsThrough) { allCasesTerminate = false; }
    } else {
        // no default means it's possible the switch is skipped
        mergedStates = preSwitchStates;
        allCasesTerminate = false;
    }
    for (const auto& caseClause : statement->cases) {
        symbolAssignmentStates = preSwitchStates;
        FlowStatus caseStatus = visit(caseClause.body);
        mergeStates(mergedStates, symbolAssignmentStates);

        if (caseStatus == FlowStatus::FallsThrough) { allCasesTerminate = false; }
    }

    // If there is no default, merge preSwitchStates to account for the path where no case matches
    if (!hasDefault) { mergeStates(mergedStates, preSwitchStates); }

    symbolAssignmentStates = std::move(mergedStates);
    return allCasesTerminate ? FlowStatus::Returns : FlowStatus::FallsThrough;
}

FlowStatus ControlFlowAnalyzer::visit(const ast::VariableDeclarationStatement* statement) noexcept {
    const semantic::Symbol* symbol = symbolTable.lookup(statement->name);
    if (symbol != nullptr) {
        setAssignmentState(*symbol,
                           statement->value == nullptr ? AssignmentState::Uninitialized : AssignmentState::Initialized);
    }
    return FlowStatus::FallsThrough;
}

FlowStatus ControlFlowAnalyzer::visit(const ast::WhileLoopStatement* statement) noexcept {
    visit(statement->condition);
    // Try to detect infinite loops
    auto conditionValue = utils::computeExpression<bool>(
        statement->condition, targetInfo,
        [this]<class... Args>(const auto* expr, std::format_string<Args...> fmt, Args&&... args) {
            this->logError(expr, fmt, std::forward<Args>(args)...);
        });

    if (conditionValue.has_value()) {
        if (!*conditionValue) {
            // while (false) so the body never executes
            return FlowStatus::FallsThrough;
        }
        // while (true) so infinite loop
        auto preLoopStates = symbolAssignmentStates;
        FlowStatus bodyStatus = visit(statement->body);
        mergeStates(symbolAssignmentStates, preLoopStates);

        if (bodyStatus == FlowStatus::Returns) { return FlowStatus::Returns; }
        return FlowStatus::InfiniteLoop;
    }
    // Condition could not be folded (runtime value): could run 0 times
    auto preLoopStates = symbolAssignmentStates;
    FlowStatus bodyStatus = visit(statement->body);
    // updated states (e.g. a variable assigned inside the loop)

    mergeStates(symbolAssignmentStates, preLoopStates);

    if (bodyStatus == FlowStatus::Returns) { return bodyStatus; }
    return FlowStatus::FallsThrough;
}

FlowStatus ControlFlowAnalyzer::visit(const ast::Block& block, bool shouldEnterScope) {
    if (shouldEnterScope) {symbolTable.enterScope();}
    FlowStatus blockStatus = FlowStatus::FallsThrough;
    bool isUnreachable = false;
    for (const ast::Statement* stmt : block) {
        if (isUnreachable) { logWarning(stmt, "Statement '{}' is unreachable", stmt->toString()); }
        // if we visit the statement first, we'd get false positives (e.g. a return statement in a function would always
        // be marked as unreachable)
        FlowStatus stmtStatus = visit(stmt);

        // if this statement isn't unreachable, we need to update the status
        // so that subsequent statements can be marked as unreachable
        if (!isUnreachable) {
            blockStatus = stmtStatus;
            isUnreachable = blockStatus != FlowStatus::FallsThrough;
        }
    }
    symbolTable.exitScope();
    return blockStatus;
}

FlowStatus ControlFlowAnalyzer::visit(const ast::PoisonedStatement* /*unused*/) noexcept {
    return FlowStatus::FallsThrough;
}

}  // namespace Manganese::cfg