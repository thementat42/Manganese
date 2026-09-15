#include <frontend/cfg/control_flow_analyzer.hpp>

namespace Manganese::cfg {

Result ControlFlowAnalyzer::analyze() {
    symbolTable.resetToRoot();
    for (const auto& file : files) {
        for (const ast::Statement* statement : file.program) { visit(statement); }
    }
    return flags.hasError ? Result::Failure : Result::Success;
}

}  // namespace Manganese::cfg