#include <frontend/cfg/control_flow_analyzer.hpp>

namespace Manganese::cfg {

Result ControlFlowAnalyzer::analyze() {
    symbolTable.resetToRoot();
    for (const auto& file : files) {
        for (const ast::Statement* statement : file.program) { visit(statement); }
    }
    return flags.hasError ? Result::Failure : Result::Success;
}

AssignmentState ControlFlowAnalyzer::getAssignmentState(const semantic::Symbol& symbol) const noexcept {
    const std::size_t wordIdx = symbol.ID / 64;
    const std::size_t bitIdx = symbol.ID % 64;
    const std::uint64_t mask = 1ULL << bitIdx;

    if ((symbolAssignmentStates.isUnInitialized[wordIdx] & mask) != 0) { return AssignmentState::Uninitialized; }
    if ((symbolAssignmentStates.isMaybeInitialized[wordIdx] & mask) != 0) { return AssignmentState::MaybeInitialized; }
    return AssignmentState::Initialized;
}

void ControlFlowAnalyzer::setAssignmentState(const semantic::Symbol& symbol, AssignmentState newState) noexcept {
    const std::size_t wordIdx = symbol.ID / 64;
    const std::size_t bitIdx = symbol.ID % 64;
    const std::uint64_t mask = 1ULL << bitIdx;

    // Clear bits first
    symbolAssignmentStates.isUnInitialized[wordIdx] &= ~mask;
    symbolAssignmentStates.isMaybeInitialized[wordIdx] &= ~mask;

    // Set the appropriate bit based on state
    if (newState == AssignmentState::Uninitialized) {
        symbolAssignmentStates.isUnInitialized[wordIdx] |= mask;
    } else if (newState == AssignmentState::MaybeInitialized) {
        symbolAssignmentStates.isMaybeInitialized[wordIdx] |= mask;
    }
}

void ControlFlowAnalyzer::mergeStates(states_t& dest, const states_t& src) {
    for (std::size_t i = 0; i < dest.isUnInitialized.size(); ++i) {
        std::uint64_t a_uninit = dest.isUnInitialized[i];
        std::uint64_t a_maybe = dest.isMaybeInitialized[i];
        std::uint64_t b_uninit = src.isUnInitialized[i];
        std::uint64_t b_maybe = src.isMaybeInitialized[i];

        dest.isUnInitialized[i] = a_uninit & b_uninit;
        dest.isMaybeInitialized[i] = (a_maybe | b_maybe) | (a_uninit ^ b_uninit);
    }
}

bool ControlFlowAnalyzer::isNoReturnCall(const ast::FunctionCallExpression* callExpr) noexcept {
    if (callExpr->callee == nullptr) { return false; }

    ast::Expression* unwrapped = semantic::SemanticAnalyzer::unwrapBaseDeclaration(callExpr->callee);

    if (unwrapped->kind != ast::ExpressionKind::IdentifierExpression) { return false; }
    const auto* idExpr = static_cast<const ast::IdentifierExpression*>(unwrapped);
    const semantic::Symbol* symbol = symbolTable.lookup(idExpr->name);
    if (symbol != nullptr && symbol->node != nullptr) {
        if (static_cast<const ast::Statement*>(symbol->node)->kind
            == ast::StatementKind::FunctionDeclarationStatement) {
            const auto* funcDecl = static_cast<const ast::FunctionDeclarationStatement*>(symbol->node);
            return ast::getAttribute(funcDecl->attributes, ast::AttributeKind::Noreturn) != nullptr;
        }
    }
    return false;
}

}  // namespace Manganese::cfg