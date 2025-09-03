#ifndef MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SEMANTIC_ANALYZER_HPP
#define MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SEMANTIC_ANALYZER_HPP

#include <frontend/ast.hpp>
#include <frontend/parser.hpp>
#include <frontend/visitor/visitor_base.hpp>
#include <global_macros.hpp>

#include "frontend/ast/ast_expressions.hpp"
#include "symbol_table.hpp"

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

    constexpr inline bool isFunctionContext() const noexcept { return functionBody > 0; }
    constexpr inline bool isIfContext() const noexcept { return ifStatement > 0; }
    constexpr inline bool isWhileLoopContext() const noexcept { return whileLoop > 0; }
    constexpr inline bool isRepeatLoopContext() const noexcept { return repeatLoop > 0; }
    constexpr inline bool isForLoopContext() const noexcept { return forLoop > 0; }
    constexpr inline bool isSwitchContext() const noexcept { return switchStatement > 0; }
    constexpr inline bool isLoopContext() const noexcept {
        return isWhileLoopContext() || isRepeatLoopContext() || isForLoopContext();
    }
};

class SemanticAnalyzer final : public visitor::Visitor<void> {
   private:
    using visitor::Visitor<void>::visit;
    SymbolTable symbolTable;
    std::string currentModule;

    // Note: These are mutable so that things like logError can be called in const functions
    // while still setting these flags
    mutable bool hasError_;
    mutable bool hasWarning_;
    Context context;

   public:
    explicit SemanticAnalyzer() noexcept : hasError_(false), hasWarning_(false) { symbolTable.enterScope(); }
    ~SemanticAnalyzer() noexcept = default;

    inline void analyze(parser::ParsedFile& parsedFile) {
        // checkImports(parsedFile.imports);
        currentModule = parsedFile.moduleName;
        for (const auto& statement : parsedFile.program) { visit(statement.get()); }
    }

    bool hasError() const noexcept { return hasError_; }
    bool hasWarning() const noexcept { return hasWarning_; }

   private:
    // ===== Basic AST Traversal =====
    void checkImports(std::vector<parser::Import>& imports);

    // ===== Misc Helpers =====
    inline void enterScope() { symbolTable.enterScope(); }
    inline void exitScope() { symbolTable.exitScope(); }

    template <typename... Args>
    inline void logWarning(const std::format_string<Args...>& fmt, const ast::ASTNode* node, Args&&... args) noexcept {
        logging::logWarning(std::format(fmt, std::forward<Args>(args)...), node->getLine(), node->getColumn());
        hasWarning_ = true;
    }

    template <typename... Args>
    inline void logError(const std::format_string<Args...>& fmt, const ast::ASTNode* node,
                         Args&&... args) const noexcept {
        logging::logError(std::format(fmt, std::forward<Args>(args)...), node->getLine(), node->getColumn());
        hasError_ = true;
    }

    inline void checkBlock(ast::Block& block) {
        for (auto& statement : block) { visit(statement.get()); }
    }

    // ===== Specific Expression Checks =====
    void visit(ast::AggregateInstantiationExpression*) override;
    void visit(ast::AggregateLiteralExpression*) override;
    void visit(ast::ArrayLiteralExpression*) override;
    void visit(ast::AssignmentExpression*) override;
    void visit(ast::BinaryExpression*) override;
    void visit(ast::BoolLiteralExpression*) override;
    void visit(ast::CharLiteralExpression*) override;
    void visit(ast::FunctionCallExpression*) override;
    void visit(ast::GenericExpression*) override;
    void visit(ast::IdentifierExpression*) override;
    void visit(ast::IndexExpression*) override;
    void visit(ast::MemberAccessExpression*) override;
    void visit(ast::NumberLiteralExpression*) override;
    void visit(ast::PostfixExpression*) override;
    void visit(ast::PrefixExpression*) override;
    void visit(ast::ScopeResolutionExpression*) override;
    void visit(ast::StringLiteralExpression*) override;
    void visit(ast::TypeCastExpression*) override;

    // ===== Specific Statement Checks =====
    void visit(ast::AggregateDeclarationStatement*) override;
    void visit(ast::AliasStatement*) override;
    void visit(ast::BreakStatement*) override;
    void visit(ast::ContinueStatement*) override;
    void visit(ast::EnumDeclarationStatement*) override;
    void visit(ast::EmptyStatement*) override { return; }  // Empty statements don't do anything
    void visit(ast::ExpressionStatement*) override;
    void visit(ast::FunctionDeclarationStatement*) override;
    void visit(ast::IfStatement*) override;
    void visit(ast::RepeatLoopStatement*) override;
    void visit(ast::ReturnStatement*) override;
    void visit(ast::SwitchStatement*) override;
    void visit(ast::VariableDeclarationStatement*) override;
    void visit(ast::WhileLoopStatement*) override;

    // ===== Type checks =====
    // Note: These need to be implemented to match the visitor interface, but aren't used
    void visit(ast::AggregateType*) override { return; }
    void visit(ast::ArrayType*) override { return; }
    void visit(ast::FunctionType*) override { return; }
    void visit(ast::GenericType*) override { return; }
    void visit(ast::PointerType*) override { return; }
    void visit(ast::SymbolType*) override { return; }

   private:
    // ===== Helpers for Specific Checks =====
    bool checkAggregateFieldAssignmentExpression(ast::AssignmentExpression*);
    bool checkDereferenceAssignmentExpression(ast::AssignmentExpression*);
    bool checkIdentifierAssignmentExpression(ast::AssignmentExpression*);
    bool checkIndexAssignmentExpression(ast::AssignmentExpression*);
    bool handleInPlaceAssignment(Manganese::ast::AssignmentExpression*);

    ast::TypeSPtr_t resolveBinaryExpressionType(ast::BinaryExpression* binaryExpression) const noexcept_if_release;
    ast::TypeSPtr_t resolveArrayBinaryExpressionType(ast::BinaryExpression* binaryExpression) const noexcept_if_release;
    ast::TypeSPtr_t resolveArithmeticBinaryExpressionType(ast::BinaryExpression* binaryExpression,
                                                          lexer::TokenType op) const noexcept_if_release;

    // ===== Type helpers =====
    bool typeExists(const ast::TypeSPtr_t& type);
    const ast::Type* resolveAlias(const ast::Type* type) const noexcept_if_release;
    /**
     * @brief Checks if one type can be promoted or demoted to another type. (e.g. int32 <-> int64)
     * @note Issues a warning on demotion
     * @note This should not allow implicit conversions (e.g. char-> int),only the same "basic" type with different
     * widths
     */
    bool areTypesPromotableOrDemotable(const ast::Type* from, const ast::Type* to) const noexcept_if_release;

    inline bool areTypesCompatible(const ast::Type* type1, const ast::Type* type2) const noexcept_if_release {
        if (!type1 || !type2) { return false; }
        const ast::Type* type1Resolved = resolveAlias(type1);
        const ast::Type* type2Resolved = resolveAlias(type2);
        return *type1Resolved == *type2Resolved || areTypesPromotableOrDemotable(type1Resolved, type2Resolved);
    }
    inline bool isBool(const ast::Type* t) const noexcept_if_release {
        return areTypesCompatible(t, std::make_shared<ast::SymbolType>("bool").get());
    }

    ast::TypeSPtr_t widestNumericType(const ast::Type* type1, const ast::Type* type2) const noexcept_if_release;
};

// ===== Helper Functions that don't depend on the SemanticAnalyzer instance =====

constexpr lexer::TokenType getBinaryOperatorFromAssignmentOperator(lexer::TokenType assignmentOp) noexcept_if_release;

inline bool isSignedInt(const ast::Type* t) { return ast::isPrimitiveType(t) && ast::toStringOr(t).starts_with("int"); }

inline bool isUInt(const ast::Type* t) { return ast::isPrimitiveType(t) && ast::toStringOr(t).starts_with("uint"); }

inline bool isAnyInt(const ast::Type* t) { return isSignedInt(t) || isUInt(t); }

inline bool isFloat(const ast::Type* t) { return ast::isPrimitiveType(t) && ast::toStringOr(t).starts_with("float"); }

inline bool isChar(const ast::Type* t) { return ast::isPrimitiveType(t) && ast::toStringOr(t) == "char"; }

inline bool isString(const ast::Type* t) { return ast::isPrimitiveType(t) && ast::toStringOr(t) == "string"; }

}  // namespace semantic

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SEMANTIC_ANALYZER_HPP