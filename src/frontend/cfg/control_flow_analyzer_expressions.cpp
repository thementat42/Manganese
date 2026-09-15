#include <frontend/ast.hpp>
#include <frontend/cfg.hpp>

namespace Manganese::cfg {

auto ControlFlowAnalyzer::visit(const ast::AggregateInstantiationExpression* expression) noexcept -> exprvisit_t {
    for (const ast::AggregateInstantiationField& field : expression->fields) { visit(field.value); }
}

auto ControlFlowAnalyzer::visit(const ast::AggregateLiteralExpression* expression) noexcept -> exprvisit_t {
    for (const ast::Expression* element : expression->elements) { visit(element); }
}

auto ControlFlowAnalyzer::visit(const ast::AlignofExpression* /*unused*/) noexcept -> exprvisit_t {}

auto ControlFlowAnalyzer::visit(const ast::ArrayLiteralExpression* expression) noexcept -> exprvisit_t {
    for (const ast::Expression* element : expression->elements) { visit(element); }
}

auto ControlFlowAnalyzer::visit(const ast::AssignmentExpression* expression) noexcept -> exprvisit_t {
    visit(expression->assignee);
    visit(expression->value);
}

auto ControlFlowAnalyzer::visit(const ast::BinaryExpression* expression) noexcept -> exprvisit_t {
    visit(expression->left);
    visit(expression->right);
}

auto ControlFlowAnalyzer::visit(const ast::BoolLiteralExpression* /*unused*/) noexcept -> exprvisit_t {}

auto ControlFlowAnalyzer::visit(const ast::CharLiteralExpression* /*unused*/) noexcept -> exprvisit_t {}

auto ControlFlowAnalyzer::visit(const ast::FunctionCallExpression* expression) noexcept -> exprvisit_t {
    visit(expression->callee);
    for (const ast::Expression* argument : expression->arguments) { visit(argument); }
}

auto ControlFlowAnalyzer::visit(const ast::GenericInstantiationExpression* expression) noexcept -> exprvisit_t {
    visit(expression->identifier);
}

auto ControlFlowAnalyzer::visit(const ast::IdentifierExpression* /*unused*/) noexcept -> exprvisit_t {}

auto ControlFlowAnalyzer::visit(const ast::IndexExpression* expression) noexcept -> exprvisit_t {
    visit(expression->variable);
    visit(expression->index);
}

auto ControlFlowAnalyzer::visit(const ast::MemberAccessExpression* expression) noexcept -> exprvisit_t {
    visit(expression->object);
}

auto ControlFlowAnalyzer::visit(const ast::NumberLiteralExpression* /*unused*/) noexcept -> exprvisit_t {}

auto ControlFlowAnalyzer::visit(const ast::PostfixExpression* expression) noexcept -> exprvisit_t {
    visit(expression->left);
}

auto ControlFlowAnalyzer::visit(const ast::PrefixExpression* expression) noexcept -> exprvisit_t {
    visit(expression->right);
}

auto ControlFlowAnalyzer::visit(const ast::ScopeResolutionExpression* expression) noexcept -> exprvisit_t {
    visit(expression->scope);
    visit(expression->element);
}

auto ControlFlowAnalyzer::visit(const ast::SizeofExpression* /*unused*/) noexcept -> exprvisit_t {}

auto ControlFlowAnalyzer::visit(const ast::StringLiteralExpression* /*unused*/) noexcept -> exprvisit_t {}

auto ControlFlowAnalyzer::visit(const ast::TypeCastExpression* expression) noexcept -> exprvisit_t {
    visit(expression->originalValue);
}

auto ControlFlowAnalyzer::visit(const ast::PoisonedExpression* /*unused*/) noexcept -> exprvisit_t {}

}  // namespace Manganese::cfg