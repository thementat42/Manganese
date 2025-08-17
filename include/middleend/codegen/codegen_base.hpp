#ifndef MANGANESE_INCLUDE_MIDDLEEND_CODEGEN_CODEGEN_BASE_HPP
#define MANGANESE_INCLUDE_MIDDLEEND_CODEGEN_CODEGEN_BASE_HPP

#include <llvm/IR/Value.h>

#include <frontend/ast.hpp>
#include <frontend/visitor/visitor_base.hpp>

namespace Manganese {
namespace codegen {

class IRGenerator final : public visitor::Visitor<llvm::Value*> {
   public:
    using visitor::Visitor<llvm::Value*>::visit;
    explicit IRGenerator() noexcept = default;
    ~IRGenerator() noexcept = default;

   private:
    // ===== Specific Expression Code Generation =====
    visit_t visit(ast::AggregateInstantiationExpression* expression) override;
    visit_t visit(ast::ArrayLiteralExpression* expression) override;
    visit_t visit(ast::AssignmentExpression* expression) override;
    visit_t visit(ast::BinaryExpression* expression) override;
    visit_t visit(ast::BoolLiteralExpression* expression) override;
    visit_t visit(ast::CharLiteralExpression* expression) override;
    visit_t visit(ast::FunctionCallExpression* expression) override;
    visit_t visit(ast::GenericExpression* expression) override;
    visit_t visit(ast::IdentifierExpression* expression) override;
    visit_t visit(ast::IndexExpression* expression) override;
    visit_t visit(ast::MemberAccessExpression* expression) override;
    visit_t visit(ast::NumberLiteralExpression* expression) override;
    visit_t visit(ast::PostfixExpression* expression) override;
    visit_t visit(ast::PrefixExpression* expression) override;
    visit_t visit(ast::ScopeResolutionExpression* expression) override;
    visit_t visit(ast::StringLiteralExpression* expression) override;
    visit_t visit(ast::TypeCastExpression* expression) override;

    // ===== Specific Statement Code Generation =====
    visit_t visit(ast::AggregateDeclarationStatement* statement) override;
    visit_t visit(ast::AliasStatement* statement) override;
    visit_t visit(ast::BreakStatement* statement) override;
    visit_t visit(ast::ContinueStatement* statement) override;
    visit_t visit(ast::EnumDeclarationStatement* statement) override;
    visit_t visit(ast::ExpressionStatement* statement) override;
    visit_t visit(ast::FunctionDeclarationStatement* statement) override;
    visit_t visit(ast::IfStatement* statement) override;
    visit_t visit(ast::RepeatLoopStatement* statement) override;
    visit_t visit(ast::ReturnStatement* statement) override;
    visit_t visit(ast::SwitchStatement* statement) override;
    visit_t visit(ast::VariableDeclarationStatement* statement) override;
    visit_t visit(ast::WhileLoopStatement* statement) override;

    // ===== Specific Type Code Generation =====
    visit_t visit(ast::AggregateType* type) override;
    visit_t visit(ast::ArrayType* type) override;
    visit_t visit(ast::FunctionType* type) override;
    visit_t visit(ast::GenericType* type) override;
    visit_t visit(ast::PointerType* type) override;
    visit_t visit(ast::SymbolType* type) override;
};

}  // namespace codegen
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_MIDDLEEND_CODEGEN_CODEGEN_BASE_H