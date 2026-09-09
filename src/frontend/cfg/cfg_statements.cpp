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

FlowStatus ControlFlowAnalyzer::visit([[maybe_unused]] const ast::NestedBlockStatement* statement) {
    return FlowStatus::Returns;
}

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

}  // namespace Manganese::cfg