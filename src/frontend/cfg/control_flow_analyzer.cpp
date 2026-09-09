#include <frontend/cfg/control_flow_analyzer.hpp>
#include <utils/result.hpp>

namespace Manganese::cfg {

Result ControlFlowAnalyzer::analyze() {
    // Don't want cascading errors
    if (semanticAnalyzer.analyze() == Result::Failure) { return Result::Failure; }
    for (const auto& file : files) {
        for (const ast::Statement* statement : file.program) { visit(statement); }
    }
    return flags.hasError ? Result::Failure : Result::Success;
}

}  // namespace Manganese::cfg