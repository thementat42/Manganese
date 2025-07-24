#ifndef MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SEMANTIC_ANALYZER_H
#define MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SEMANTIC_ANALYZER_H

#include <frontend/ast.h>
#include <frontend/parser.h>
#include <global_macros.h>

#include "symbol_table.h"

//! The semantic analyzer should never "fix" errors -- only report them

namespace Manganese {

namespace semantic {

/**
 * @brief Some statements are only valid in certain contexts (e.g. return in function bodies, break/continue in loops).
 * This struct keeps track of current context flags to determine if a statement is valid in the current scope.
 */
struct ContextCounters {
    int64_t functionBody = 0;
    int64_t ifStatement = 0;
    int64_t whileLoop = 0;
    int64_t repeatLoop = 0;
    int64_t forLoop = 0;
    int64_t switchStatement = 0;

    bool isFunctionContext() const noexcept { return functionBody > 0; }
    bool isIfContext() const noexcept { return ifStatement > 0; }
    bool isWhileLoopContext() const noexcept { return whileLoop > 0; }
    bool isRepeatLoopContext() const noexcept { return repeatLoop > 0; }
    bool isForLoopContext() const noexcept { return forLoop > 0; }
    bool isSwitchContext() const noexcept { return switchStatement > 0; }
    bool isLoopContext() const noexcept {
        return isWhileLoopContext() || isRepeatLoopContext() || isForLoopContext();
    }
};

class SemanticAnalyzer {
   private:
    SymbolTable symbolTable;
    std::string currentModule;
    bool hasError_;
    bool hasWarning_;
    ContextCounters context;

   public:
    explicit SemanticAnalyzer() noexcept : hasError_(false), hasWarning_(false) {
        symbolTable.enterScope();
    }

    void analyze(parser::ParsedFile& parsedFile);

    bool hasError() const noexcept { return hasError_; }
    bool hasWarning() const noexcept { return hasWarning_; }

   private:
    // ===== Basic AST Traversal =====
    void checkImports(std::vector<parser::Import>& imports);
    void checkStatement(ast::Statement* statement) noexcept_if_release;
    void checkExpression(ast::Expression* expression) noexcept_if_release;

    // ===== Misc Helpers =====
    inline void enterScope() {symbolTable.enterScope(); }
    inline void exitScope() {symbolTable.exitScope(); }

    template <typename ... Args>
    inline void logWarning(const std::format_string<Args...>& fmt, ast::ASTNode* node, Args&&... args) noexcept {
        logging::logWarning(std::format(fmt, std::forward<Args>(args)...), node->getLine(), node->getColumn());
        hasWarning_ = true;
    }

    template <typename... Args>
    inline void logError(const std::format_string<Args...>& fmt, ast::ASTNode* node, Args&&... args) noexcept {
        logging::logError(std::format(fmt, std::forward<Args>(args)...), node->getLine(), node->getColumn());
        hasError_ = true;
    }

    inline void checkBlock(ast::Block& block) noexcept(noexcept(checkStatement(std::declval<ast::Statement*>()))) {
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

    // ===== Helpers for Specific Checks =====
    bool typeExists(const ast::TypeSPtr_t& type);
    bool areTypesEqual(const ast::Type* type1, const ast::Type* type2) const noexcept_if_release;
    /**
     * @brief Checks if one type can be promoted or demoted to another type. (e.g. int32 <-> int64)
     * @note Issues a warning on demotion
     * @note This should not allow implicit conversions (e.g. char-> int),only the same "basic" type with different widths
     */
    bool areTypesPromotableOrDemotable(const ast::Type* from, const ast::Type* to) const noexcept_if_release;
    inline bool areTypesCompatible(const ast::Type* type1, const ast::Type* type2) const noexcept_if_release {
        return areTypesEqual(type1, type2) || areTypesPromotableOrDemotable(type1, type2);
    }
    
};
}  // namespace semantic

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SEMANTIC_ANALYZER_H