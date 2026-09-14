#include <frontend/ast.hpp>
#include <frontend/cfg/control_flow_analyzer.hpp>
#include <frontend/semantic/symbol_table.hpp>

namespace Manganese::cfg {

void ControlFlowAnalyzer::visit(const ast::AggregateInstantiationExpression* expression) noexcept {
    visit(expression->base);
    for (const ast::AggregateInstantiationField& field : expression->fields) { visit(field.value); }
}

void ControlFlowAnalyzer::visit(const ast::AggregateLiteralExpression* expression) noexcept {
    for (const ast::Expression* element : expression->elements) { visit(element); }
}

void ControlFlowAnalyzer::visit(const ast::AlignofExpression* /*unused*/) noexcept {}

void ControlFlowAnalyzer::visit(const ast::ArrayLiteralExpression* expression) noexcept {
    for (const ast::Expression* element : expression->elements) { visit(element); }
}

void ControlFlowAnalyzer::visit(const ast::AssignmentExpression* expression) noexcept {
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

void ControlFlowAnalyzer::visit(const ast::BinaryExpression* expression) noexcept {
    visit(expression->left);
    visit(expression->right);
}

void ControlFlowAnalyzer::visit(const ast::BoolLiteralExpression* /*unused*/) noexcept {}

void ControlFlowAnalyzer::visit(const ast::CharLiteralExpression* /*unused*/) noexcept {}

void ControlFlowAnalyzer::visit(const ast::FunctionCallExpression* expression) noexcept {
    visit(expression->callee);
    for (const ast::Expression* argument : expression->arguments) { visit(argument); }
}

void ControlFlowAnalyzer::visit(const ast::GenericInstantiationExpression* expression) noexcept {
    visit(expression->identifier);
}

void ControlFlowAnalyzer::visit(const ast::IdentifierExpression* expression) noexcept {
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

void ControlFlowAnalyzer::visit(const ast::IndexExpression* expression) noexcept {
    visit(expression->variable);
    visit(expression->index);
}

void ControlFlowAnalyzer::visit(const ast::MemberAccessExpression* expression) noexcept { visit(expression->object); }

void ControlFlowAnalyzer::visit(const ast::NumberLiteralExpression* /*unused*/) noexcept {}

void ControlFlowAnalyzer::visit(const ast::PostfixExpression* expression) noexcept { visit(expression->left); }

void ControlFlowAnalyzer::visit(const ast::PrefixExpression* expression) noexcept { visit(expression->right); }

void ControlFlowAnalyzer::visit(const ast::ScopeResolutionExpression* expression) noexcept {
    visit(expression->scope);
    visit(expression->element);
}

void ControlFlowAnalyzer::visit(const ast::SizeofExpression* /*unused*/) noexcept {}

void ControlFlowAnalyzer::visit(const ast::StringLiteralExpression* /*unused*/) noexcept {}

void ControlFlowAnalyzer::visit(const ast::TypeCastExpression* expression) noexcept {
    visit(expression->originalValue);
}

void ControlFlowAnalyzer::visit(const ast::PoisonedExpression* /*unused*/) noexcept {}

}  // namespace Manganese::cfg