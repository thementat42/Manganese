#ifndef MANGANESE_INCLUDE_FRONTEND_SEMANTIC_ANALYZER_HPP
#define MANGANESE_INCLUDE_FRONTEND_SEMANTIC_ANALYZER_HPP

#include <frontend/ast.hpp>
#include <frontend/lexer.hpp>
#include <frontend/parser.hpp>
#include <frontend/semantic/symbol_table.hpp>
#include <global_macros.hpp>


namespace Manganese {

namespace semantic {

class analyzer final : public ast::Visitor<void, void, void> {
    // note: with `final`, the compiler can more intelligently detect when analyzer is abstract (i.e., a particular
    // visit() override hasn't been implemented) right here, instead of failing at an analyzer instantiation
   private:
    SymbolTable table;
    parser::ParsedFile& parsed;

   public:
    analyzer(parser::ParsedFile& file) : table(), parsed(file) {}
    void analyze() {
        collect_types();
        collect_symbols();
        check_statements();
    }
    ~analyzer() override = default;

   private:
    void collect_types();  // first pass -- collect all user-defined types
    void collect_symbols();  // second pass -- collect variables, functions, etc.
    inline void check_statements() {  // semantic analysis pass
        for (auto& stmt : parsed.program) { this->visit(stmt); }
    }

   protected:
    // overrides for visitor functions
    using ast::Visitor<void, void, void>::visit;

    // ===== Expression Visiting =====
    exprvisit_t visit(ast::AggregateInstantiationExpression*) override;
    exprvisit_t visit(ast::AggregateLiteralExpression*) override;
    exprvisit_t visit(ast::ArrayLiteralExpression*) override;
    exprvisit_t visit(ast::AssignmentExpression*) override;
    exprvisit_t visit(ast::BinaryExpression*) override;
    exprvisit_t visit(ast::BoolLiteralExpression*) override;
    exprvisit_t visit(ast::CharLiteralExpression*) override;
    exprvisit_t visit(ast::FunctionCallExpression*) override;
    exprvisit_t visit(ast::GenericExpression*) override;
    exprvisit_t visit(ast::IdentifierExpression*) override;
    exprvisit_t visit(ast::IndexExpression*) override;
    exprvisit_t visit(ast::MemberAccessExpression*) override;
    exprvisit_t visit(ast::NumberLiteralExpression*) override;
    exprvisit_t visit(ast::PostfixExpression*) override;
    exprvisit_t visit(ast::PrefixExpression*) override;
    exprvisit_t visit(ast::ScopeResolutionExpression*) override;
    exprvisit_t visit(ast::StringLiteralExpression*) override;
    exprvisit_t visit(ast::TypeCastExpression*) override;

    // ===== Statement Visiting =====
    stmtvisit_t visit(ast::AggregateDeclarationStatement*) override;
    stmtvisit_t visit(ast::AliasStatement*) override;
    stmtvisit_t visit(ast::BreakStatement*) override;
    stmtvisit_t visit(ast::ContinueStatement*) override;
    stmtvisit_t visit(ast::EmptyStatement*) override;
    stmtvisit_t visit(ast::EnumDeclarationStatement*) override;
    stmtvisit_t visit(ast::ExpressionStatement*) override;
    stmtvisit_t visit(ast::FunctionDeclarationStatement*) override;
    stmtvisit_t visit(ast::IfStatement*) override;
    stmtvisit_t visit(ast::RepeatLoopStatement*) override;
    stmtvisit_t visit(ast::ReturnStatement*) override;
    stmtvisit_t visit(ast::SwitchStatement*) override;
    stmtvisit_t visit(ast::VariableDeclarationStatement*) override;
    stmtvisit_t visit(ast::WhileLoopStatement*) override;

    // ===== Type Visiting =====
    typevisit_t visit(ast::AggregateType*) override;
    typevisit_t visit(ast::ArrayType*) override;
    typevisit_t visit(ast::FunctionType*) override;
    typevisit_t visit(ast::GenericType*) override;
    typevisit_t visit(ast::PointerType*) override;
    typevisit_t visit(ast::SymbolType*) override;
};

}  // namespace semantic

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_SEMANTIC_ANALYZER_HPP