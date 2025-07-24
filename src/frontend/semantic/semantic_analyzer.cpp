/**
 * @file semantic_analyzer.cpp
 * @brief Implementation of some core methods for the semantic analyzer
 */

#include <frontend/ast.h>
#include <frontend/semantic/semantic_analyzer.h>
#include <global_macros.h>

#include <format>

namespace Manganese {
namespace semantic {

void SemanticAnalyzer::analyze(parser::ParsedFile& parsedFile) {
    // checkImports(parsedFile.imports);
    currentModule = parsedFile.moduleName;
    for (const auto& statement : parsedFile.program) {
        checkStatement(statement.get());
    }
}

}  // namespace semantic
}  // namespace Manganese
