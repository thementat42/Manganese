#include <frontend/cfg/control_flow_analyzer.hpp>

namespace Manganese::cfg {

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

}  // namespace Manganese::cfg