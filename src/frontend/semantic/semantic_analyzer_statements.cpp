#include <frontend/ast.h>
#include <frontend/semantic.h>
#include <io/logging.h>

namespace Manganese {
namespace semantic {

void SemanticAnalyzer::checkStatement(const ast::Statement* statement) {
    if (!statement) [[unlikely]] {
        logging::logInternal("Null pointer passed to checkStatement", logging::LogLevel::Error);
        return;
    }
    using ast::StatementKind;
    switch (statement->kind()) {
        case StatementKind::AliasStatement:
            checkAliasStatement(static_cast<const ast::AliasStatement*>(statement));
            break;
        case StatementKind::BreakStatement:
            checkBreakStatement(static_cast<const ast::BreakStatement*>(statement));
            break;
        case StatementKind::BundleDeclarationStatement:
            checkBundleDeclarationStatement(static_cast<const ast::BundleDeclarationStatement*>(statement));
            break;
        case StatementKind::ContinueStatement:
            checkContinueStatement(static_cast<const ast::ContinueStatement*>(statement));
            break;
        case StatementKind::EnumDeclarationStatement:
            checkEnumDeclarationStatement(static_cast<const ast::EnumDeclarationStatement*>(statement));
            break;
        case StatementKind::ExpressionStatement:
            checkExpressionStatement(static_cast<const ast::ExpressionStatement*>(statement));
            break;
        case StatementKind::FunctionDeclarationStatement:
            checkFunctionDeclarationStatement(static_cast<const ast::FunctionDeclarationStatement*>(statement));
            break;
        case StatementKind::IfStatement:
            checkIfStatement(static_cast<const ast::IfStatement*>(statement));
            break;
        case StatementKind::ReturnStatement:
            checkReturnStatement(static_cast<const ast::ReturnStatement*>(statement));
            break;
        case StatementKind::SwitchStatement:
            checkSwitchStatement(static_cast<const ast::SwitchStatement*>(statement));
            break;
        case StatementKind::VariableDeclarationStatement:
            checkVariableDeclarationStatement(static_cast<const ast::VariableDeclarationStatement*>(statement));
            break;
        case StatementKind::WhileLoopStatement:
            checkWhileLoopStatement(static_cast<const ast::WhileLoopStatement*>(statement));
            break;
        default:
            ASSERT_UNREACHABLE("Unknown statement kind: " + std::to_string(static_cast<int>(statement->kind())));
    }
}

// ===== Specific Statement Checks =====
void SemanticAnalyzer::checkAliasStatement(const ast::AliasStatement* statement) {
    DISCARD(statement);  // Avoid unused variable warning, for now
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkBreakStatement(const ast::BreakStatement* statement) {
    DISCARD(statement);  // Avoid unused variable warning, for now
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkBundleDeclarationStatement(const ast::BundleDeclarationStatement* statement) {
    DISCARD(statement);  // Avoid unused variable warning, for now
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkContinueStatement(const ast::ContinueStatement* statement) {
    DISCARD(statement);  // Avoid unused variable warning, for now
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkEnumDeclarationStatement(const ast::EnumDeclarationStatement* statement) {
    DISCARD(statement);  // Avoid unused variable warning, for now
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkExpressionStatement(const ast::ExpressionStatement* statement) {
    checkExpression(statement->expression.get());
}
void SemanticAnalyzer::checkFunctionDeclarationStatement(const ast::FunctionDeclarationStatement* statement) {
    DISCARD(statement);  // Avoid unused variable warning, for now
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkIfStatement(const ast::IfStatement* statement) {
    DISCARD(statement);  // Avoid unused variable warning, for now
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkReturnStatement(const ast::ReturnStatement* statement) {
    DISCARD(statement);  // Avoid unused variable warning, for now
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkSwitchStatement(const ast::SwitchStatement* statement) {
    DISCARD(statement);  // Avoid unused variable warning, for now
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkVariableDeclarationStatement(const ast::VariableDeclarationStatement* statement) {
    DISCARD(statement);  // Avoid unused variable warning, for now
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkWhileLoopStatement(const ast::WhileLoopStatement* statement) {
    DISCARD(statement);  // Avoid unused variable warning, for now
    throw std::runtime_error("Not implemented");
}

}  // namespace semantic

}  // namespace Manganese