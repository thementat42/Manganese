#include <core.hpp>
#include <frontend/semantic/analyzer.hpp>
#include <frontend/semantic/type_context.hpp>

namespace Manganese {
namespace semantic {

Result analyzer::analyze() {
    Result isSemanticallyValid = Result::Success;
    if (collectTypes() == Result::Failure) { isSemanticallyValid = Result::Failure; };
    if (collectGlobals() == Result::Failure) { isSemanticallyValid = Result::Failure; };
    if (collectAndSpecializeGenerics() == Result::Failure) { isSemanticallyValid = Result::Failure; }
    // Don't want errors cascading because of conflicting redeclarations
    if (isSemanticallyValid == Result::Failure) { return isSemanticallyValid; }

    symbolTable.switchToCheckingMode();
    isSemanticallyValid = checkStatements();
    return isSemanticallyValid;
}

Result analyzer::checkStatements() {  // semantic analysis pass (this can also check the generic specializations)
    Result programIsSemanticallyValid = Result::Success;
    for (auto& stmt : parsedFile.program) {
        if (this->visit(stmt) == Result::Failure) { programIsSemanticallyValid = Result::Failure; }
    }
    return programIsSemanticallyValid;
}
}  // namespace semantic
}  // namespace Manganese