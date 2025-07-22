#ifndef MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SEMANTIC_ANALYZER_H
#define MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SEMANTIC_ANALYZER_H

#include <frontend/ast.h>
#include <frontend/parser.h>
#include <global_macros.h>

#include "symbol_table.h"

//! The semantic analyzer should never "fix" errors -- only report them

namespace Manganese {

namespace semantic {
class SemanticAnalyzer {
   private:
    SymbolTable symbolTable;
    std::string currentModule;
    bool hasError_ = false;

   public:
    explicit SemanticAnalyzer() noexcept = default;

    void analyze(parser::ParsedFile& parsedFile);

    bool hasError() const noexcept {
        return hasError_;
    }

    bool areTypesCompatible(const ast::Type* type1, const ast::Type* type2) const noexcept_if_release;

   private:
    // ===== Basic AST Traversal =====
    void checkImports(std::vector<parser::Import>& imports);
    void checkStatement(ast::Statement* statement) noexcept_if_release;
    void checkExpression(ast::Expression* expression) noexcept_if_release;

    // ===== Misc Helpers =====
    inline void enterScope() {symbolTable.enterScope(); }
    inline void exitScope() {symbolTable.exitScope(); }
    inline void logError(const std::string& message, size_t line = 0, size_t col = 0) noexcept {
        logging::logError(message, line, col);
        hasError_ = true;
    }

    inline void checkBlock(ast::Block& block) noexcept(noexcept(checkStatement)) {
        for (auto& statement : block) {
            checkStatement(statement.get());
        }
    }

    // ===== Specific Expression Checks =====
    void checkArrayLiteralExpression(ast::ArrayLiteralExpression* expression);
    void checkAssignmentExpression(ast::AssignmentExpression* expression);
    void checkBinaryExpression(ast::BinaryExpression* expression);
    void checkBoolLiteralExpression(ast::BoolLiteralExpression* expression);
    void checkBundleInstantiationExpression(ast::BundleInstantiationExpression* expression);
    void checkCharLiteralExpression(ast::CharLiteralExpression* expression);
    void checkFunctionCallExpression(ast::FunctionCallExpression* expression);
    void checkGenericExpression(ast::GenericExpression* expression);
    void checkIdentifierExpression(ast::IdentifierExpression* expression);
    void checkIndexExpression(ast::IndexExpression* expression);
    void checkMemberAccessExpression(ast::MemberAccessExpression* expression);
    void checkNumberLiteralExpression(ast::NumberLiteralExpression* expression);
    void checkPostfixExpression(ast::PostfixExpression* expression);
    void checkPrefixExpression(ast::PrefixExpression* expression);
    void checkScopeResolutionExpression(ast::ScopeResolutionExpression* expression);
    void checkStringLiteralExpression(ast::StringLiteralExpression* expression);
    void checkTypeCastExpression(ast::TypeCastExpression* expression);

    // ===== Specific Statement Checks =====
    void checkAliasStatement(ast::AliasStatement* statement);
    void checkBreakStatement(ast::BreakStatement* statement);
    void checkBundleDeclarationStatement(ast::BundleDeclarationStatement* statement);
    void checkContinueStatement(ast::ContinueStatement* statement);
    void checkEnumDeclarationStatement(ast::EnumDeclarationStatement* statement);
    void checkExpressionStatement(ast::ExpressionStatement* statement);
    void checkFunctionDeclarationStatement(ast::FunctionDeclarationStatement* statement);
    void checkIfStatement(ast::IfStatement* statement);
    void checkImportStatement(ast::ImportStatement* statement);
    void checkModuleDeclarationStatement(ast::ModuleDeclarationStatement* statement);
    void checkRepeatLoopStatement(ast::RepeatLoopStatement* statement);
    void checkReturnStatement(ast::ReturnStatement* statement);
    void checkSwitchStatement(ast::SwitchStatement* statement);
    void checkVariableDeclarationStatement(ast::VariableDeclarationStatement* statement);
    void checkWhileLoopStatement(ast::WhileLoopStatement* statement);
};
}  // namespace semantic

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SEMANTIC_ANALYZER_H