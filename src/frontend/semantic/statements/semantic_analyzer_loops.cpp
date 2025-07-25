#include <frontend/semantic/semantic_analyzer.h>

namespace Manganese {

namespace semantic {

void SemanticAnalyzer::checkRepeatLoopStatement(ast::RepeatLoopStatement* statement) {
    DISCARD(statement);
    PRINT_LOCATION;
    throw std::runtime_error("Not implemented");
}

void SemanticAnalyzer::checkWhileLoopStatement(ast::WhileLoopStatement* statement) {
    DISCARD(statement);
    PRINT_LOCATION;
    throw std::runtime_error("Not implemented");
}

}  // namespace semantic

}  // namespace Manganese