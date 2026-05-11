#ifndef MANGANESE_INCLUDE_FRONTEND_VISITOR_VISITOR_BASE_HPP
#define MANGANESE_INCLUDE_FRONTEND_VISITOR_VISITOR_BASE_HPP

#include <frontend/ast/ast_base.hpp>
#include <frontend/ast/ast_expressions.hpp>
#include <frontend/ast/ast_statements.hpp>
#include <frontend/ast/ast_types.hpp>
#include <global_macros.hpp>

namespace Manganese {

namespace ast {

template <class ExpressionResult, class StatementResult, class TypeResult>
class Visitor {
   public:
    virtual ~Visitor() noexcept = default;
    using exprvisit_t = ExpressionResult;
    using stmtvisit_t = StatementResult;
    using typevisit_t = TypeResult;

   protected:
#define STMT(name, str) virtual stmtvisit_t visit(ast::name*) = 0;
#define EXPR(name, str) virtual exprvisit_t visit(ast::name*) = 0;
#define TYPE(name, str) virtual typevisit_t visit(ast::name*) = 0;
#include <frontend/ast/ast.def>
#undef STMT
#undef EXPR
#undef TYPE

    // Dispatch for the different kinds of nodes

    exprvisit_t visit(ast::Expression*);
    stmtvisit_t visit(ast::Statement*);
    typevisit_t visit(ast::Type*);
    // A wrapper around visit(Type*) to handle a shared pointer
    FORCE_INLINE typevisit_t visit(ast::TypeSPtr_t& type) { return visit(type.get()); }
};

}  // namespace ast

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_VISITOR_VISITOR_BASE_HPP