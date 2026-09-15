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
    visit(expression->value);

    // e.g. assigning an element of an array
    if (expression->assignee->kind != ast::ExpressionKind::IdentifierExpression) { return; }

    const semantic::Symbol* assigneeSymbol
        = symbolTable.lookup(static_cast<const ast::IdentifierExpression*>(expression->assignee)->name);

    if (assigneeSymbol != nullptr) {
        // the variable gets marked as initialized even if the value isn't initialized.
        // because the recursive visit() call to the value will catch the use of an uninitialized value, the visitor as
        // a whole will still eventually error out, so we don't need to bubble up errors
        setAssignmentState(*assigneeSymbol, AssignmentState::Initialized);
    }
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

auto ControlFlowAnalyzer::visit(const ast::IdentifierExpression* expression) noexcept -> exprvisit_t {
    const semantic::Symbol* symbol = symbolTable.lookup(expression->name);
    if (symbol == nullptr) { return; }  // this was a semantic error
    using enum semantic::SymbolKind;

    if (const auto state = getAssignmentState(*symbol); symbol->kind == Variable) {
        if (state == AssignmentState::Uninitialized) {
            logError(expression, "Identifier '{}' does not have a value", expression->name);
        } else if (state == AssignmentState::MaybeInitialized) {
            logError(expression, "Identifier '{}' may not have a value on all control paths", expression->name);
        }
    }
}

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