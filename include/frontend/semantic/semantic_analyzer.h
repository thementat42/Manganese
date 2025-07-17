/**
 * @file semantic_analyzer.h
 * @brief Header for the SemanticAnalyzer class
 */
#ifndef MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SEMANTIC_ANALYZER_H
#define MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SEMANTIC_ANALYZER_H

#include <frontend/ast.h>
#include <frontend/parser.h>
#include <io/logging.h>

#include "symbol_table.h"

namespace Manganese {
namespace semantic {

class SemanticAnalyzer {
   private:
    SymbolTable symbolTable;
    std::string currentModule;
    bool hasError_ = false;

   public:
    explicit SemanticAnalyzer() noexcept = default;

    /**
     * @brief The entry point for semantic analysis.
     */
    void analyze(const parser::ParsedFile& parsedFile);

    [[nodiscard]] bool hasError() const noexcept {
        return hasError_;
    }

   private:

   // ===== Misc Helpers =====
    void enterScope();
    void exitScope();
    inline void logError(const std::string& message) {
        hasError_ = true;
        logging::logError(message);
    }

    // ===== Basic AST Traversal =====
    void checkImports(const std::vector<parser::Import>& imports);
    void checkBlock(const ast::Block& block);
    void checkStatement(const ast::Statement* statement);
    void checkExpression(const ast::Expression* expression);

    // ===== Type Helpers =====
    bool areTypesCompatible(const TypeInfo& type1, const TypeInfo& type2) const;

    // Note: These methods take in raw pointers, which should be passed using .get() on the unique_ptrs
    // These methods will not take ownership of the pointers and are not responsible for their deletion.
    // ===== Specific Expression Checks =====
    void checkArrayLiteralExpression(const ast::ArrayLiteralExpression* expression);
    void checkAssignmentExpression(const ast::AssignmentExpression* expression);
    void checkBinaryExpression(const ast::BinaryExpression* expression);
    void checkBoolLiteralExpression(const ast::BoolLiteralExpression* expression);
    void checkBundleInstantiationExpression(const ast::BundleInstantiationExpression* expression);
    void checkCharLiteralExpression(const ast::CharLiteralExpression* expression);
    void checkFunctionCallExpression(const ast::FunctionCallExpression* expression);
    void checkGenericExpression(const ast::GenericExpression* expression);
    void checkIdentifierExpression(const ast::IdentifierExpression* expression);
    void checkIndexExpression(const ast::IndexExpression* expression);
    void checkMemberAccessExpression(const ast::MemberAccessExpression* expression);
    void checkNumberLiteralExpression(const ast::NumberLiteralExpression* expression);
    void checkPostfixExpression(const ast::PostfixExpression* expression);
    void checkPrefixExpression(const ast::PrefixExpression* expression);
    void checkScopeResolutionExpression(const ast::ScopeResolutionExpression* expression);
    void checkStringLiteralExpression(const ast::StringLiteralExpression* expression);
    void checkTypeCastExpression(const ast::TypeCastExpression* expression);

    // ===== Specific Statement Checks =====
    void checkAliasStatement(const ast::AliasStatement* statement);
    void checkBreakStatement(const ast::BreakStatement* statement);
    void checkBundleDeclarationStatement(const ast::BundleDeclarationStatement* statement);
    void checkContinueStatement(const ast::ContinueStatement* statement);
    void checkEnumDeclarationStatement(const ast::EnumDeclarationStatement* statement);
    void checkExpressionStatement(const ast::ExpressionStatement* statement);
    void checkFunctionDeclarationStatement(const ast::FunctionDeclarationStatement* statement);
    void checkIfStatement(const ast::IfStatement* statement);
    void checkReturnStatement(const ast::ReturnStatement* statement);
    void checkSwitchStatement(const ast::SwitchStatement* statement);
    void checkVariableDeclarationStatement(const ast::VariableDeclarationStatement* statement);
    void checkWhileLoopStatement(const ast::WhileLoopStatement* statement);
};

}  // namespace semantic

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SEMANTIC_ANALYZER_H