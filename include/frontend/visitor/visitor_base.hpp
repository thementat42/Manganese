#ifndef MANGANESE_INCLUDE_FRONTEND_VISITOR_VISITOR_BASE_HPP
#define MANGANESE_INCLUDE_FRONTEND_VISITOR_VISITOR_BASE_HPP

#include <format>
#include <frontend/ast.hpp>
#include <global_macros.hpp>

#include "frontend/ast/ast_base.hpp"
#include "frontend/ast/ast_expressions.hpp"
#include "frontend/ast/ast_types.hpp"

namespace Manganese {

namespace visitor {

template <class Result>
class Visitor {
   public:
    virtual ~Visitor() noexcept = default;

    protected:
    // ===== Expression Visiting =====
    virtual Result visit(ast::AggregateInstantiationExpression*) = 0;
    virtual Result visit(ast::ArrayLiteralExpression*) = 0;
    virtual Result visit(ast::AssignmentExpression*) = 0;
    virtual Result visit(ast::BinaryExpression*) = 0;
    virtual Result visit(ast::BoolLiteralExpression*) = 0;
    virtual Result visit(ast::CharLiteralExpression*) = 0;
    virtual Result visit(ast::FunctionCallExpression*) = 0;
    virtual Result visit(ast::GenericExpression*) = 0;
    virtual Result visit(ast::IdentifierExpression*) = 0;
    virtual Result visit(ast::IndexExpression*) = 0;
    virtual Result visit(ast::MemberAccessExpression*) = 0;
    virtual Result visit(ast::NumberLiteralExpression*) = 0;
    virtual Result visit(ast::PostfixExpression*) = 0;
    virtual Result visit(ast::PrefixExpression*) = 0;
    virtual Result visit(ast::ScopeResolutionExpression*) = 0;
    virtual Result visit(ast::StringLiteralExpression*) = 0;
    virtual Result visit(ast::TypeCastExpression*) = 0;

    // ===== Statement Visiting =====
    virtual Result visit(ast::AggregateDeclarationStatement*) = 0;
    virtual Result visit(ast::AliasStatement*) = 0;
    virtual Result visit(ast::BreakStatement*) = 0;
    virtual Result visit(ast::ContinueStatement*) = 0;
    // virtual Result visit(ast::EmptyStatement*) = 0;
    virtual Result visit(ast::EnumDeclarationStatement*) = 0;
    virtual Result visit(ast::ExpressionStatement*) = 0;
    virtual Result visit(ast::FunctionDeclarationStatement*) = 0;
    virtual Result visit(ast::IfStatement*) = 0;
    virtual Result visit(ast::RepeatLoopStatement*) = 0;
    virtual Result visit(ast::ReturnStatement*) = 0;
    virtual Result visit(ast::SwitchStatement*) = 0;
    virtual Result visit(ast::VariableDeclarationStatement*) = 0;
    virtual Result visit(ast::WhileLoopStatement*) = 0;

    // ===== Type Visiting =====
    virtual Result visit(ast::AggregateType*) = 0;
    virtual Result visit(ast::ArrayType*) = 0;
    virtual Result visit(ast::FunctionType*) = 0;
    virtual Result visit(ast::GenericType*) = 0;
    virtual Result visit(ast::PointerType*) = 0;
    virtual Result visit(ast::SymbolType*) = 0;

    // ===== Dispatch for the different kinds of nodes =====
    Result visit(ast::Expression* expr) {
        using enum ast::ExpressionKind;
        switch (expr->kind()) {
            case AggregateInstantiationExpression:
                return visit(static_cast<ast::AggregateInstantiationExpression*>(expr));
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
                    std::format("No visit() overload for expression kind {}", static_cast<int>(expr->kind())));
        }
    }

    Result visit(ast::Statement* stmt) {
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
                    std::format("No visit() overload for statement kind {}", static_cast<int>(stmt->kind())));
        }
    }

    Result visit(ast::Type* type) {
        using enum ast::TypeKind;
        switch (type->kind()) {
            case AggregateType: return visit(static_cast<ast::AggregateType*>(type));
            case ArrayType: return visit(static_cast<ast::ArrayType*>(type));
            case FunctionType: return visit(static_cast<ast::FunctionType*>(type));
            case GenericType: return visit(static_cast<ast::GenericType*>(type));
            case PointerType: return visit(static_cast<ast::PointerType*>(type));
            case SymbolType: return visit(static_cast<ast::SymbolType*>(type));
            default:
                ASSERT_UNREACHABLE(std::format("No visit() overload for type kind {}", static_cast<int>(type->kind())));
        }
    }
};

}  // namespace visitor

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_VISITOR_VISITOR_BASE_HPP