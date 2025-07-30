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
struct Context {
    int64_t functionBody = 0;
    int64_t ifStatement = 0;
    int64_t whileLoop = 0;
    int64_t repeatLoop = 0;
    int64_t forLoop = 0;
    int64_t switchStatement = 0;
    ast::TypeSPtr_t currentFunctionReturnType = nullptr;

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

    // Note: These are mutable so that things like logError can be called in const functions
    // while still setting these flags
    mutable bool hasError_;
    mutable bool hasWarning_;
    Context context;

   public:
    explicit SemanticAnalyzer() noexcept : hasError_(false), hasWarning_(false) {
        symbolTable.enterScope();
    }

    inline void analyze(parser::ParsedFile& parsedFile) {
        // checkImports(parsedFile.imports);
        currentModule = parsedFile.moduleName;
        for (const auto& statement : parsedFile.program) {
            checkStatement(statement.get());
        }
    }

    bool hasError() const noexcept { return hasError_; }
    bool hasWarning() const noexcept { return hasWarning_; }

   private:
    // ===== Basic AST Traversal =====
    void checkImports(std::vector<parser::Import>& imports);
    void checkStatement(ast::Statement* statement) noexcept_if_release;
    void checkExpression(ast::Expression* expression) noexcept_if_release;

    // ===== Misc Helpers =====
    inline void enterScope() { symbolTable.enterScope(); }
    inline void exitScope() { symbolTable.exitScope(); }

    template <typename... Args>
    inline void logWarning(const std::format_string<Args...>& fmt, ast::ASTNode* node, Args&&... args) noexcept {
        logging::logWarning(std::format(fmt, std::forward<Args>(args)...), node->getLine(), node->getColumn());
        hasWarning_ = true;
    }

    template <typename... Args>
    inline void logError(const std::format_string<Args...>& fmt, const ast::ASTNode* node, Args&&... args) const noexcept {
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
    bool handleInPlaceAssignment(Manganese::ast::AssignmentExpression* expression);
    bool typeExists(const ast::TypeSPtr_t& type);
    const ast::Type* resolveAlias(const ast::Type* type) const noexcept_if_release;
    /**
     * @brief Checks if one type can be promoted or demoted to another type. (e.g. int32 <-> int64)
     * @note Issues a warning on demotion
     * @note This should not allow implicit conversions (e.g. char-> int),only the same "basic" type with different widths
     */
    bool areTypesPromotableOrDemotable(const ast::Type* from, const ast::Type* to) const noexcept_if_release;

    inline bool areTypesCompatible(const ast::Type* type1, const ast::Type* type2) const noexcept_if_release {
        const ast::Type* type1Resolved = resolveAlias(type1);
        const ast::Type* type2Resolved = resolveAlias(type2);
        return *type1Resolved == *type2Resolved || areTypesPromotableOrDemotable(type1Resolved, type2Resolved);
    }
    inline bool isBool(const ast::Type* t) const noexcept_if_release {
        return areTypesCompatible(t, std::make_shared<ast::SymbolType>("bool").get());
    }

    ast::TypeSPtr_t resolveBinaryExpressionType(ast::BinaryExpression* binaryExpression) const noexcept_if_release;
    ast::TypeSPtr_t widestNumericType(const ast::Type* type1, const ast::Type* type2) const noexcept_if_release;
    ast::TypeSPtr_t resolveArrayBinaryExpressionType(ast::BinaryExpression* binaryExpression) const noexcept_if_release;
    ast::TypeSPtr_t resolveArithmeticBinaryExpressionType(ast::BinaryExpression* binaryExpression, lexer::TokenType op) const noexcept_if_release;
};

// ===== Helper Functions that don't depend on the SemanticAnalyzer instance =====

constexpr lexer::TokenType getBinaryOperatorFromAssignmentOperator(lexer::TokenType assignmentOp) noexcept_if_release;
inline const auto isSignedInt = [](const ast::Type* t) -> bool {
    return ast::isPrimitiveType(t) && t->toString().starts_with("int");
};
inline const auto isUInt = [](const ast::Type* t) -> bool {
    return ast::isPrimitiveType(t) && t->toString().starts_with("uint");
};

inline const auto isAnyInt = [](const ast::Type* t) -> bool {
    return isSignedInt(t) || isUInt(t);
};

inline const auto isFloat = [](const ast::Type* t) -> bool {
    return ast::isPrimitiveType(t) && t->toString().starts_with("float");
};
inline const auto isChar = [](const ast::Type* t) -> bool {
    return ast::isPrimitiveType(t) && t->toString() == "char";
};
// inline const auto isBool = [](const ast::Type* t) -> bool {
//     return ast::isPrimitiveType(t) && t->toString() == "bool";
// };
inline const auto isString = [](const ast::Type* t) -> bool {
    return ast::isPrimitiveType(t) && t->toString() == "string";
};
}  // namespace semantic

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SEMANTIC_ANALYZER_H