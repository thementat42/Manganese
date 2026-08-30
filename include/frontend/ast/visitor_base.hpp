#ifndef MANGANESE_INCLUDE_FRONTEND_VISITOR_VISITOR_BASE_HPP
#define MANGANESE_INCLUDE_FRONTEND_VISITOR_VISITOR_BASE_HPP

#include <frontend/ast/ast_base.hpp>
#include <frontend/ast/ast_expressions.hpp>
#include <frontend/ast/ast_statements.hpp>
#include <frontend/ast/ast_types.hpp>

namespace Manganese::ast {

template <class ExpressionResult, class StatementResult, class TypeResult>
class Visitor {
   public:
    virtual ~Visitor() noexcept = default;
    using exprvisit_t = ExpressionResult;
    using stmtvisit_t = StatementResult;
    using typevisit_t = TypeResult;

   protected:
#define STMT(name) virtual stmtvisit_t visit(ast::name*) = 0;
#define EXPR(name) virtual exprvisit_t visit(ast::name*) = 0;
#define TYPE(name) virtual typevisit_t visit(ast::name*) = 0;
#include <frontend/ast/ast.def>
#undef STMT
#undef EXPR
#undef TYPE

    // Dispatch for the different kinds of nodes

    exprvisit_t visit(ast::Expression* expr) {
        switch (expr->kind) {
#define STMT(name)
#define EXPR(name) \
    case ast::ExpressionKind::name: return visit(static_cast<ast::name*>(expr));

#define TYPE(name)
#include <frontend/ast/ast.def>

            default:
                ASSERT_UNREACHABLE(
                    std::format("No visit() overload for expression kind {}", static_cast<int>(expr->kind)));
        }

#undef STMT
#undef EXPR
#undef TYPE
    }
    stmtvisit_t visit(ast::Statement* stmt) {
        switch (stmt->kind) {
#define STMT(name) \
    case ast::StatementKind::name: return visit(static_cast<ast::name*>(stmt));

#define EXPR(name)
#define TYPE(name)

#include <frontend/ast/ast.def>

            default:
                ASSERT_UNREACHABLE(
                    std::format("No visit() overload for statement kind {}", static_cast<int>(stmt->kind)));
        }
#undef STMT
#undef EXPR
#undef TYPE
    }
    typevisit_t visit(ast::Type* type) {
        switch (type->kind) {
#define STMT(name)
#define EXPR(name)

#define TYPE(name) \
    case ast::TypeKind::name: return visit(static_cast<ast::name*>(type));

#include <frontend/ast/ast.def>

            default:
                ASSERT_UNREACHABLE(std ::format("No visit() overload for type kind {}", static_cast<int>(type->kind)));
        }

#undef STMT
#undef EXPR
#undef TYPE
    }
};

}  // namespace Manganese::ast

#endif  // MANGANESE_INCLUDE_FRONTEND_VISITOR_VISITOR_BASE_HPP