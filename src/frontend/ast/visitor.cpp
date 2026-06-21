#include <format>
#include <frontend/ast/visitor_base.hpp>

namespace Manganese {
namespace ast {

template <class Expr, class Stmt, class Type>
auto Visitor<Expr, Stmt, Type>::visit(ast::Expression* expr) -> exprvisit_t {
    switch (expr->kind) {
#define STMT(name, str)
#define EXPR(name, str) \
    case ast::ExpressionKind::name: return visit(static_cast<ast::name*>(expr));

#define TYPE(name, str)
#include <frontend/ast/ast.def>

        default:
            ASSERT_UNREACHABLE(
                std::format("No visit() overload for expression kind {}", static_cast<int>(expr->kind)));
    }

#undef STMT
#undef EXPR
#undef TYPE
}
template <class Expr, class Stmt, class Type>
auto Visitor<Expr, Stmt, Type>::visit(ast::Statement* stmt) -> stmtvisit_t {
    switch (stmt->kind) {
#define STMT(name, str) \
    case ast::StatementKind::name: return visit(static_cast<ast::name*>(stmt));

#define EXPR(name, str)
#define TYPE(name, str)

#include <frontend/ast/ast.def>

        default:
            ASSERT_UNREACHABLE(
                std::format("No visit() overload for statement kind {}", static_cast<int>(stmt->kind)));
    }
#undef STMT
#undef EXPR
#undef TYPE
}
template <class Expr, class Stmt, class Type>
auto Visitor<Expr, Stmt, Type>::visit(ast::Type* type) -> typevisit_t {
    switch (type->kind) {
#define STMT(name, str)
#define EXPR(name, str)

#define TYPE(name, str) \
    case ast::TypeKind::name: return visit(static_cast<ast::name*>(type));

#include <frontend/ast/ast.def>

        default:
            ASSERT_UNREACHABLE(std ::format("No visit() overload for type kind {}", static_cast<int>(type->kind)));
    }

#undef STMT
#undef EXPR
#undef TYPE
}

template class Visitor<Result, Result, Result>;

}  // namespace ast
}  // namespace Manganese