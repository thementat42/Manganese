#include <format>
#include <frontend/ast.hpp>
#include <frontend/ast/visitor_base.hpp>
#include <utils/result.hpp>

namespace Manganese::ast {

template <class Expr, class Stmt, class Type>
auto Visitor<Expr, Stmt, Type>::visit(ast::Expression* expr) -> exprvisit_t {
    switch (expr->kind) {
#define STMT(name)
#define EXPR(name) \
    case ast::ExpressionKind::name: return visit(static_cast<ast::name*>(expr));

#define TYPE(name)
#include <frontend/ast/ast.def>

        default:
            ASSERT_UNREACHABLE(std::format("No visit() overload for expression kind {}", static_cast<int>(expr->kind)));
    }

#undef STMT
#undef EXPR
#undef TYPE
}
template <class Expr, class Stmt, class Type>
auto Visitor<Expr, Stmt, Type>::visit(ast::Statement* stmt) -> stmtvisit_t {
    switch (stmt->kind) {
#define STMT(name) \
    case ast::StatementKind::name: return visit(static_cast<ast::name*>(stmt));

#define EXPR(name)
#define TYPE(name)

#include <frontend/ast/ast.def>

        default:
            ASSERT_UNREACHABLE(std::format("No visit() overload for statement kind {}", static_cast<int>(stmt->kind)));
    }
#undef STMT
#undef EXPR
#undef TYPE
}
template <class Expr, class Stmt, class Type>
auto Visitor<Expr, Stmt, Type>::visit(ast::Type* type) -> typevisit_t {
    switch (type->kind) {
#define STMT(name)
#define EXPR(name)

#define TYPE(name) \
    case ast::TypeKind::name: return visit(static_cast<ast::name*>(type));

#include <frontend/ast/ast.def>

        default: ASSERT_UNREACHABLE(std ::format("No visit() overload for type kind {}", static_cast<int>(type->kind)));
    }

#undef STMT
#undef EXPR
#undef TYPE
}

template class Visitor<Result, Result, Result>;

}  // namespace Manganese::ast