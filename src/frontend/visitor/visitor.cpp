#include <format>
#include <frontend/ast/visitor_base.hpp>

namespace Manganese {
namespace ast {

template <class Expr, class Stmt, class Type>
auto Visitor<Expr, Stmt, Type>::visit(ast::Expression* expr) -> exprvisit_t {
    using enum ast::ExpressionKind;
    switch (expr->kind()) {
        case AggregateInstantiationExpression: return visit(static_cast<ast::AggregateInstantiationExpression*>(expr));
        case AggregateLiteralExpression: return visit(static_cast<ast::AggregateLiteralExpression*>(expr));
        case ArrayLiteralExpression: return visit(static_cast<ast::ArrayLiteralExpression*>(expr));
        case AssignmentExpression: return visit(static_cast<ast::AssignmentExpression*>(expr));
        case BinaryExpression: return visit(static_cast<ast::BinaryExpression*>(expr));
        case BoolLiteralExpression: return visit(static_cast<ast::BoolLiteralExpression*>(expr));
        case CharLiteralExpression: return visit(static_cast<ast::CharLiteralExpression*>(expr));
        case FunctionCallExpression: return visit(static_cast<ast::FunctionCallExpression*>(expr));
        case GenericExpression: return visit(static_cast<ast::GenericExpression*>(expr));
        case IdentifierExpression: return visit(static_cast<ast::IdentifierExpression*>(expr));
        case IndexExpression: return visit(static_cast<ast::IndexExpression*>(expr));
        case MemberAccessExpression: return visit(static_cast<ast::MemberAccessExpression*>(expr));
        case NumberLiteralExpression: return visit(static_cast<ast::NumberLiteralExpression*>(expr));
        case PostfixExpression: return visit(static_cast<ast::PostfixExpression*>(expr));
        case PrefixExpression: return visit(static_cast<ast::PrefixExpression*>(expr));
        case ScopeResolutionExpression: return visit(static_cast<ast::ScopeResolutionExpression*>(expr));
        case StringLiteralExpression: return visit(static_cast<ast::StringLiteralExpression*>(expr));
        case TypeCastExpression: return visit(static_cast<ast::TypeCastExpression*>(expr));
        default:
            ASSERT_UNREACHABLE(
                std ::format("No visit() overload for expression kind {}", static_cast<int>(expr->kind())));
    }
}
template <class Expr, class Stmt, class Type>
auto Visitor<Expr, Stmt, Type>::visit(ast::Statement* stmt) -> stmtvisit_t {
    using enum ast::StatementKind;
    switch (stmt->kind()) {
        case AggregateDeclarationStatement: return visit(static_cast<ast::AggregateDeclarationStatement*>(stmt));
        case AliasStatement: return visit(static_cast<ast::AliasStatement*>(stmt));
        case BreakStatement: return visit(static_cast<ast::BreakStatement*>(stmt));
        case ContinueStatement: return visit(static_cast<ast::ContinueStatement*>(stmt));
        case EmptyStatement: return visit(static_cast<ast::EmptyStatement*>(stmt));
        case EnumDeclarationStatement: return visit(static_cast<ast::EnumDeclarationStatement*>(stmt));
        case ExpressionStatement: return visit(static_cast<ast::ExpressionStatement*>(stmt));
        case FunctionDeclarationStatement: return visit(static_cast<ast::FunctionDeclarationStatement*>(stmt));
        case IfStatement: return visit(static_cast<ast::IfStatement*>(stmt));
        case RepeatLoopStatement: return visit(static_cast<ast::RepeatLoopStatement*>(stmt));
        case ReturnStatement: return visit(static_cast<ast::ReturnStatement*>(stmt));
        case SwitchStatement: return visit(static_cast<ast::SwitchStatement*>(stmt));
        case VariableDeclarationStatement: return visit(static_cast<ast::VariableDeclarationStatement*>(stmt));
        case WhileLoopStatement: return visit(static_cast<ast::WhileLoopStatement*>(stmt));
        default:
            ASSERT_UNREACHABLE(
                std ::format("No visit() overload for statement kind {}", static_cast<int>(stmt->kind())));
    }
}
template <class Expr, class Stmt, class Type>
auto Visitor<Expr, Stmt, Type>::visit(ast::Type* type) -> typevisit_t {
    using enum ast::TypeKind;
    switch (type->kind()) {
        case AggregateType: return visit(static_cast<ast::AggregateType*>(type));
        case ArrayType: return visit(static_cast<ast::ArrayType*>(type));
        case FunctionType: return visit(static_cast<ast::FunctionType*>(type));
        case GenericType: return visit(static_cast<ast::GenericType*>(type));
        case PointerType: return visit(static_cast<ast::PointerType*>(type));
        case SymbolType: return visit(static_cast<ast::SymbolType*>(type));
        default:
            ASSERT_UNREACHABLE(std ::format("No visit() overload for type kind {}", static_cast<int>(type->kind())));
    }
}

}  // namespace ast
}  // namespace Manganese