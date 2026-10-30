#ifndef MANGANESE_INCLUDE_FRONTEND_VISITOR_VISITOR_BASE_HPP
#define MANGANESE_INCLUDE_FRONTEND_VISITOR_VISITOR_BASE_HPP

#include <core.hpp>
#include <format>
#include <frontend/ast/ast_base.hpp>
#include <frontend/ast/ast_expressions.hpp>
#include <frontend/ast/ast_statements.hpp>
#include <frontend/ast/ast_types.hpp>
#include <type_traits>

namespace Manganese::ast {

template <class ExpressionResult, class StatementResult, class TypeResult, bool AltersNodes>
class Visitor {
   public:
    virtual ~Visitor() noexcept = default;
    using exprvisit_t = ExpressionResult;
    using stmtvisit_t = StatementResult;
    using typevisit_t = TypeResult;

    template <class U>
    using NodePointer = std::conditional_t<AltersNodes, U*, const U*>;

   protected:
#define STMT_NODE(name) virtual stmtvisit_t visit(NodePointer<ast::name>) = 0;
#define EXPR_NODE(name) virtual exprvisit_t visit(NodePointer<ast::name>) = 0;
#define TYPE_NODE(name) virtual typevisit_t visit(NodePointer<ast::name>) = 0;
#include <frontend/ast/ast.def>
#undef STMT_NODE
#undef EXPR_NODE
#undef TYPE_NODE

    // Dispatch for the different kinds of nodes

    stmtvisit_t visit(NodePointer<ast::Statement> stmt) {
        switch (stmt->kind) {
#define STMT_NODE(name) \
    case ast::StatementKind::name: return visit(static_cast<NodePointer<ast::name>>(stmt));

#define EXPR_NODE(name)
#define TYPE_NODE(name)

#include <frontend/ast/ast.def>
        }

        ASSERT_UNREACHABLE_FMT("No visit() overload for statement kind {}", static_cast<int>(stmt->kind));
#undef STMT_NODE
#undef EXPR_NODE
#undef TYPE_NODE
    }

    exprvisit_t visit(NodePointer<ast::Expression> expr) {
        switch (expr->kind) {
#define STMT_NODE(name)
#define EXPR_NODE(name) \
    case ast::ExpressionKind::name: return visit(static_cast<NodePointer<ast::name>>(expr));

#define TYPE_NODE(name)
#include <frontend/ast/ast.def>
        }

        ASSERT_UNREACHABLE_FMT("No visit() overload for expression kind {}", static_cast<int>(expr->kind));
#undef STMT_NODE
#undef EXPR_NODE
#undef TYPE_NODE
    }

    typevisit_t visit(NodePointer<ast::Type> type) {
        switch (type->kind) {
#define STMT_NODE(name)
#define EXPR_NODE(name)

#define TYPE_NODE(name) \
    case ast::TypeKind::name: return visit(static_cast<NodePointer<ast::name>>(type));

#include <frontend/ast/ast.def>
        }

        ASSERT_UNREACHABLE_FMT("No visit() overload for type kind {}", static_cast<int>(type->kind));

#undef STMT_NODE
#undef EXPR_NODE
#undef TYPE_NODE
    }
};

}  // namespace Manganese::ast

#endif  // MANGANESE_INCLUDE_FRONTEND_VISITOR_VISITOR_BASE_HPP