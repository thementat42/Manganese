#include <frontend/ast.hpp>
#include <frontend/cfg/control_flow_analyzer.hpp>

namespace Manganese::cfg {

void ControlFlowAnalyzer::visit(const ast::AggregateInstantiationExpression* /*unused*/) noexcept {}

void ControlFlowAnalyzer::visit(const ast::AggregateLiteralExpression* /*unused*/) noexcept {}

void ControlFlowAnalyzer::visit(const ast::AlignofExpression* /*unused*/) noexcept {}

void ControlFlowAnalyzer::visit(const ast::ArrayLiteralExpression* /*unused*/) noexcept {}

void ControlFlowAnalyzer::visit(const ast::AssignmentExpression* expression) noexcept {
    // e.g. assigning an element of an array
    if (expression->assignee->kind != ast::ExpressionKind::IdentifierExpression) { return; }

    const semantic::Symbol* symbol
        = symbolTable.lookup(static_cast<const ast::IdentifierExpression*>(expression->assignee)->name);
    if (symbol != nullptr) { symbolAssignmentStates[symbol->ID] = AssignmentState::Initialized; }
}

void ControlFlowAnalyzer::visit([[maybe_unused]] const ast::BinaryExpression* expression) noexcept {}

void ControlFlowAnalyzer::visit(const ast::BoolLiteralExpression* /*unused*/) noexcept {}

void ControlFlowAnalyzer::visit(const ast::CharLiteralExpression* /*unused*/) noexcept {}

void ControlFlowAnalyzer::visit([[maybe_unused]] const ast::FunctionCallExpression* expression) noexcept {}

void ControlFlowAnalyzer::visit(const ast::GenericInstantiationExpression* /*unused*/) noexcept {}

void ControlFlowAnalyzer::visit([[maybe_unused]] const ast::IdentifierExpression* expression) noexcept {}

void ControlFlowAnalyzer::visit([[maybe_unused]] const ast::IndexExpression* expression) noexcept {}

void ControlFlowAnalyzer::visit([[maybe_unused]] const ast::MemberAccessExpression* expression) noexcept {}

void ControlFlowAnalyzer::visit(const ast::NumberLiteralExpression* /*unused*/) noexcept {}

void ControlFlowAnalyzer::visit([[maybe_unused]] const ast::PostfixExpression* expression) noexcept {}

void ControlFlowAnalyzer::visit([[maybe_unused]] const ast::PrefixExpression* expression) noexcept {}

void ControlFlowAnalyzer::visit(const ast::ScopeResolutionExpression* /*unused*/) noexcept {}

void ControlFlowAnalyzer::visit(const ast::SizeofExpression* /*unused*/) noexcept {}

void ControlFlowAnalyzer::visit(const ast::StringLiteralExpression* /*unused*/) noexcept {}

void ControlFlowAnalyzer::visit([[maybe_unused]] const ast::TypeCastExpression* expression) noexcept {}

void ControlFlowAnalyzer::visit(const ast::PoisonedExpression* /*unused*/) noexcept {}

}  // namespace Manganese::cfg