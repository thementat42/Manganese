#ifndef MANGANESE_INCLUDE_FRONTEND_VISITOR_VISITOR_BASE_HPP
#define MANGANESE_INCLUDE_FRONTEND_VISITOR_VISITOR_BASE_HPP

#include <frontend/ast.hpp>
#include <global_macros.hpp>

namespace Manganese {

namespace visitor {

template <class ExpressionResult, class StatementResult = ExpressionResult, class TypeResult = ExpressionResult>
class Visitor {
   public:
    virtual ~Visitor() noexcept = default;
    using exprvisit_t = ExpressionResult;
    using stmtvisit_t = StatementResult;
    using typevisit_t = TypeResult;

   protected:
    // ===== Expression Visiting =====
    virtual exprvisit_t visit(ast::AggregateInstantiationExpression*) = 0;
    virtual exprvisit_t visit(ast::AggregateLiteralExpression*) = 0;
    virtual exprvisit_t visit(ast::ArrayLiteralExpression*) = 0;
    virtual exprvisit_t visit(ast::AssignmentExpression*) = 0;
    virtual exprvisit_t visit(ast::BinaryExpression*) = 0;
    virtual exprvisit_t visit(ast::BoolLiteralExpression*) = 0;
    virtual exprvisit_t visit(ast::CharLiteralExpression*) = 0;
    virtual exprvisit_t visit(ast::FunctionCallExpression*) = 0;
    virtual exprvisit_t visit(ast::GenericExpression*) = 0;
    virtual exprvisit_t visit(ast::IdentifierExpression*) = 0;
    virtual exprvisit_t visit(ast::IndexExpression*) = 0;
    virtual exprvisit_t visit(ast::MemberAccessExpression*) = 0;
    virtual exprvisit_t visit(ast::NumberLiteralExpression*) = 0;
    virtual exprvisit_t visit(ast::PostfixExpression*) = 0;
    virtual exprvisit_t visit(ast::PrefixExpression*) = 0;
    virtual exprvisit_t visit(ast::ScopeResolutionExpression*) = 0;
    virtual exprvisit_t visit(ast::StringLiteralExpression*) = 0;
    virtual exprvisit_t visit(ast::TypeCastExpression*) = 0;

    // ===== Statement Visiting =====
    virtual stmtvisit_t visit(ast::AggregateDeclarationStatement*) = 0;
    virtual stmtvisit_t visit(ast::AliasStatement*) = 0;
    virtual stmtvisit_t visit(ast::BreakStatement*) = 0;
    virtual stmtvisit_t visit(ast::ContinueStatement*) = 0;
    virtual stmtvisit_t visit(ast::EmptyStatement*) = 0;
    virtual stmtvisit_t visit(ast::EnumDeclarationStatement*) = 0;
    virtual stmtvisit_t visit(ast::ExpressionStatement*) = 0;
    virtual stmtvisit_t visit(ast::FunctionDeclarationStatement*) = 0;
    virtual stmtvisit_t visit(ast::IfStatement*) = 0;
    virtual stmtvisit_t visit(ast::RepeatLoopStatement*) = 0;
    virtual stmtvisit_t visit(ast::ReturnStatement*) = 0;
    virtual stmtvisit_t visit(ast::SwitchStatement*) = 0;
    virtual stmtvisit_t visit(ast::VariableDeclarationStatement*) = 0;
    virtual stmtvisit_t visit(ast::WhileLoopStatement*) = 0;

    // ===== Type Visiting =====
    virtual typevisit_t visit(ast::AggregateType*) = 0;
    virtual typevisit_t visit(ast::ArrayType*) = 0;
    virtual typevisit_t visit(ast::FunctionType*) = 0;
    virtual typevisit_t visit(ast::GenericType*) = 0;
    virtual typevisit_t visit(ast::PointerType*) = 0;
    virtual typevisit_t visit(ast::SymbolType*) = 0;

    // ===== Dispatch for the different kinds of nodes =====

    exprvisit_t visit(ast::Expression*);
    // A wrapper around visit(Expression*) to handle a unique pointer
    FORCE_INLINE exprvisit_t visit(ast::ExpressionUPtr_t& expr) { return visit(expr.get()); }

    stmtvisit_t visit(ast::Statement*);
    // A wrapper around visit(Statement*) to handle a unique pointer
    FORCE_INLINE stmtvisit_t visit(ast::StatementUPtr_t& stmt) { return visit(stmt.get()); }

    typevisit_t visit(ast::Type*);
    // A wrapper around visit(Type*) to handle a unique pointer
    FORCE_INLINE typevisit_t visit(ast::TypeSPtr_t& type) { return visit(type.get()); }

};

}  // namespace visitor

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_VISITOR_VISITOR_BASE_HPP