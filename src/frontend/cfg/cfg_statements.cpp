#include <frontend/ast.hpp>
#include <frontend/cfg/control_flow_analyzer.hpp>

namespace Manganese::cfg {

FlowStatus ControlFlowAnalyzer::visit(const ast::AggregateDeclarationStatement* /*unused*/) {
    return FlowStatus::FallsThrough;
}

FlowStatus ControlFlowAnalyzer::visit(const ast::AliasStatement* /*unused*/) { return FlowStatus::FallsThrough; }

FlowStatus ControlFlowAnalyzer::visit(const ast::BreakStatement* /*unused*/) { return FlowStatus::Breaks; }

FlowStatus ControlFlowAnalyzer::visit(const ast::ContinueStatement* /*unused*/) { return FlowStatus::ContinuesLoop; }

FlowStatus ControlFlowAnalyzer::visit(const ast::EmptyStatement* /*unused*/) { return FlowStatus::FallsThrough; }

FlowStatus ControlFlowAnalyzer::visit(const ast::EnumDeclarationStatement* /*unused*/) {
    return FlowStatus::FallsThrough;
}

FlowStatus ControlFlowAnalyzer::visit(const ast::ExpressionStatement* /*unused*/) { return FlowStatus::FallsThrough; }

FlowStatus ControlFlowAnalyzer::visit([[maybe_unused]] const ast::ForLoopStatement* statement) {
    return FlowStatus::Returns;
}

FlowStatus ControlFlowAnalyzer::visit([[maybe_unused]] const ast::FunctionDeclarationStatement* statement) {
    return FlowStatus::Returns;
}

FlowStatus ControlFlowAnalyzer::visit([[maybe_unused]] const ast::IfStatement* statement) {
    return FlowStatus::Returns;
}

FlowStatus ControlFlowAnalyzer::visit(const ast::ImportStatement* /*unused*/) { return FlowStatus::FallsThrough; }

FlowStatus ControlFlowAnalyzer::visit(const ast::ModuleDeclarationStatement* /*unused*/) {
    return FlowStatus::FallsThrough;
}

FlowStatus ControlFlowAnalyzer::visit(const ast::NamespaceStatement* /*unused*/) { return FlowStatus::FallsThrough; }

FlowStatus ControlFlowAnalyzer::visit(const ast::NestedBlockStatement* statement) { return visit(statement->block); }

FlowStatus ControlFlowAnalyzer::visit(const ast::ReturnStatement* /*unused*/) { return FlowStatus::Returns; }

FlowStatus ControlFlowAnalyzer::visit([[maybe_unused]] const ast::SwitchStatement* statement) {
    return FlowStatus::Returns;
}

FlowStatus ControlFlowAnalyzer::visit(const ast::VariableDeclarationStatement* /*unused*/) {
    return FlowStatus::FallsThrough;
}

FlowStatus ControlFlowAnalyzer::visit([[maybe_unused]] const ast::WhileLoopStatement* statement) {
    return FlowStatus::Returns;
}

FlowStatus ControlFlowAnalyzer::visit([[maybe_unused]] const ast::Block& block) {
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

}  // namespace Manganese::cfg