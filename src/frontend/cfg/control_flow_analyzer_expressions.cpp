#include <frontend/ast.hpp>
#include <frontend/cfg/control_flow_analyzer.hpp>
#include "frontend/ast/ast_expressions.hpp"

namespace Manganese::cfg {

void ControlFlowAnalyzer::visit([[maybe_unused]] const ast::AggregateInstantiationExpression* expression) noexcept {}

void ControlFlowAnalyzer::visit([[maybe_unused]] const ast::AggregateLiteralExpression* expression) noexcept {}

void ControlFlowAnalyzer::visit([[maybe_unused]] const ast::AlignofExpression* expression) noexcept {}

void ControlFlowAnalyzer::visit([[maybe_unused]] const ast::ArrayLiteralExpression* expression) noexcept {}

void ControlFlowAnalyzer::visit([[maybe_unused]] const ast::AssignmentExpression* expression) noexcept {}

void ControlFlowAnalyzer::visit([[maybe_unused]] const ast::BinaryExpression* expression) noexcept {}

void ControlFlowAnalyzer::visit([[maybe_unused]] const ast::BoolLiteralExpression* expression) noexcept {}

void ControlFlowAnalyzer::visit([[maybe_unused]] const ast::CharLiteralExpression* expression) noexcept {}

void ControlFlowAnalyzer::visit([[maybe_unused]] const ast::FunctionCallExpression* expression) noexcept {}

void ControlFlowAnalyzer::visit([[maybe_unused]] const ast::GenericInstantiationExpression* expression) noexcept {}

void ControlFlowAnalyzer::visit([[maybe_unused]] const ast::IdentifierExpression* expression) noexcept {}

void ControlFlowAnalyzer::visit([[maybe_unused]] const ast::IndexExpression* expression) noexcept {}

void ControlFlowAnalyzer::visit([[maybe_unused]] const ast::MemberAccessExpression* expression) noexcept {}

void ControlFlowAnalyzer::visit([[maybe_unused]] const ast::NumberLiteralExpression* expression) noexcept {}

void ControlFlowAnalyzer::visit([[maybe_unused]] const ast::PostfixExpression* expression) noexcept {}

void ControlFlowAnalyzer::visit([[maybe_unused]] const ast::PrefixExpression* expression) noexcept {}

void ControlFlowAnalyzer::visit([[maybe_unused]] const ast::ScopeResolutionExpression* expression) noexcept {}

void ControlFlowAnalyzer::visit([[maybe_unused]] const ast::SizeofExpression* expression) noexcept {}

void ControlFlowAnalyzer::visit([[maybe_unused]] const ast::StringLiteralExpression* expression) noexcept {}

void ControlFlowAnalyzer::visit([[maybe_unused]] const ast::TypeCastExpression* expression) noexcept {}

void ControlFlowAnalyzer::visit([[maybe_unused]] const ast::PoisonedExpression* expression) noexcept {}

}  // namespace Manganese::cfg