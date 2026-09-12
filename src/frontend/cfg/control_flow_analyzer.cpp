#include <format>
#include <frontend/ast.hpp>
#include <frontend/cfg/control_flow_analyzer.hpp>
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

FlowStatus ControlFlowAnalyzer::visit(const ast::ExpressionStatement* /*unused*/) noexcept {
    return FlowStatus::FallsThrough;
}

FlowStatus ControlFlowAnalyzer::visit(const ast::ForLoopStatement* statement) noexcept {
    if (statement->stopCondition == nullptr) {
        // no stop condition, whether or not the loop exits depends on whether the body breaks/returns
        FlowStatus bodyStatus = visit(statement->body);
        if (bodyStatus == FlowStatus::Returns) { return FlowStatus::Returns; }
        if (bodyStatus == FlowStatus::Breaks) { return FlowStatus::Breaks; }
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
        FlowStatus bodyStatus = visit(statement->body);

        if (bodyStatus == FlowStatus::Returns) { return FlowStatus::Returns; }
        if (bodyStatus == FlowStatus::Breaks) { return FlowStatus::Breaks; }
        return FlowStatus::InfiniteLoop;
    }
    // Couldn't fold value, assume it termintes
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
    FlowStatus ifBodyStatus = visit(statement->body);
    bool allBranchesTerminate = (ifBodyStatus != FlowStatus::FallsThrough);
    FlowStatus result = ifBodyStatus;

    for (const auto& elif : statement->elifs) {
        FlowStatus elifStatus = visit(elif.body);
        if (elifStatus == FlowStatus::FallsThrough || (allBranchesTerminate && elifStatus != result)) {
            allBranchesTerminate = false;
        }
    }

    if (!statement->elseBody.empty()) {
        allBranchesTerminate = false;
    } else {
        FlowStatus elseStatus = visit(statement->elseBody);
        if ((elseStatus == FlowStatus::FallsThrough) || (allBranchesTerminate && elseStatus != result)) {
            allBranchesTerminate = false;
        }
    }
    return allBranchesTerminate ? result : FlowStatus::FallsThrough;
}

FlowStatus ControlFlowAnalyzer::visit(const ast::ImportStatement* /*unused*/) noexcept {
    return FlowStatus::FallsThrough;
}

FlowStatus ControlFlowAnalyzer::visit(const ast::ModuleDeclarationStatement* /*unused*/) noexcept {
    return FlowStatus::FallsThrough;
}

FlowStatus ControlFlowAnalyzer::visit(const ast::NamespaceStatement* /*unused*/) noexcept {
    return FlowStatus::FallsThrough;
}

FlowStatus ControlFlowAnalyzer::visit(const ast::NestedBlockStatement* statement) noexcept {
    return visit(statement->block);
}

FlowStatus ControlFlowAnalyzer::visit(const ast::ReturnStatement* /*unused*/) noexcept { return FlowStatus::Returns; }

FlowStatus ControlFlowAnalyzer::visit(const ast::SwitchStatement* statement) noexcept {
    bool allCasesTerminate = true;
    if (statement->defaultBody.empty()) {
        allCasesTerminate = false;
    } else {
        FlowStatus defaultStatus = visit(statement->defaultBody);
        if (defaultStatus == FlowStatus::FallsThrough) { allCasesTerminate = false; }
    }
    for (const auto& caseClause : statement->cases) {
        FlowStatus caseStatus = visit(caseClause.body);
        if (caseStatus == FlowStatus::FallsThrough) { allCasesTerminate = false; }
    }
    return allCasesTerminate ? FlowStatus::Returns : FlowStatus::FallsThrough;
}

FlowStatus ControlFlowAnalyzer::visit(const ast::VariableDeclarationStatement* /*unused*/) noexcept {
    return FlowStatus::FallsThrough;
}

FlowStatus ControlFlowAnalyzer::visit(const ast::WhileLoopStatement* statement) noexcept {
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
        FlowStatus bodyStatus = visit(statement->body);
        if (bodyStatus == FlowStatus::Returns) { return FlowStatus::Returns; }
        return FlowStatus::InfiniteLoop;
    }
    // Condition could not be folded (runtime value), so we assume it terminates
    return FlowStatus::FallsThrough;
}

FlowStatus ControlFlowAnalyzer::visit(const ast::Block& block) {
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
    return blockStatus;
}

FlowStatus ControlFlowAnalyzer::visit(const ast::PoisonedStatement* /*unused*/) noexcept {
    return FlowStatus::FallsThrough;
}

}  // namespace Manganese::cfg