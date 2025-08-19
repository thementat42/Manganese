#ifndef MANGANESE_INCLUDE_MIDDLEEND_CODEGEN_CODEGEN_BASE_HPP
#define MANGANESE_INCLUDE_MIDDLEEND_CODEGEN_CODEGEN_BASE_HPP

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include <climits>
#include <frontend/ast.hpp>
#include <frontend/visitor/visitor_base.hpp>
#include <global_macros.hpp>
#include <map>
#include <memory>
#include <string>

namespace Manganese {
namespace codegen {

class IRGenerator final : public visitor::Visitor<llvm::Value*> {
   private:
    using visitor::Visitor<llvm::Value*>::visit;
    std::unique_ptr<llvm::LLVMContext> theContext;
    std::unique_ptr<llvm::Module> theModule;
    std::unique_ptr<llvm::IRBuilder<>> theBuilder;
    std::map<std::string, llvm::Value*> namedValues;

   public:
    explicit IRGenerator() noexcept = default;
    ~IRGenerator() noexcept = default;

   private:
    // ===== Specific Expression Code Generation =====
    visit_t visit(ast::AggregateInstantiationExpression*) override;
    visit_t visit(ast::ArrayLiteralExpression*) override;
    visit_t visit(ast::AssignmentExpression*) override;
    visit_t visit(ast::BinaryExpression*) override;
    visit_t visit(ast::BoolLiteralExpression*) override;
    visit_t visit(ast::CharLiteralExpression*) override;
    visit_t visit(ast::FunctionCallExpression*) override;
    visit_t visit(ast::GenericExpression*) override;
    visit_t visit(ast::IdentifierExpression*) override;
    visit_t visit(ast::IndexExpression*) override;
    visit_t visit(ast::MemberAccessExpression*) override;
    visit_t visit(ast::NumberLiteralExpression*) override;
    visit_t visit(ast::PostfixExpression*) override;
    visit_t visit(ast::PrefixExpression*) override;
    visit_t visit(ast::ScopeResolutionExpression*) override;
    visit_t visit(ast::StringLiteralExpression*) override;
    visit_t visit(ast::TypeCastExpression*) override;

    // ===== Specific Statement Code Generation =====
    visit_t visit(ast::AggregateDeclarationStatement*) override;
    visit_t visit(ast::AliasStatement*) override;
    visit_t visit(ast::BreakStatement*) override;
    visit_t visit(ast::ContinueStatement*) override;
    visit_t visit(ast::EnumDeclarationStatement*) override;
    visit_t visit(ast::EmptyStatement*) override;
    visit_t visit(ast::ExpressionStatement*) override;
    visit_t visit(ast::FunctionDeclarationStatement*) override;
    visit_t visit(ast::IfStatement*) override;
    visit_t visit(ast::RepeatLoopStatement*) override;
    visit_t visit(ast::ReturnStatement*) override;
    visit_t visit(ast::SwitchStatement*) override;
    visit_t visit(ast::VariableDeclarationStatement*) override;
    visit_t visit(ast::WhileLoopStatement*) override;

    // ===== Specific Type Code Generation =====
    visit_t visit(ast::AggregateType*) override;
    visit_t visit(ast::ArrayType*) override;
    visit_t visit(ast::FunctionType*) override;
    visit_t visit(ast::GenericType*) override;
    visit_t visit(ast::PointerType*) override;
    visit_t visit(ast::SymbolType*) override;
};

// ===== Helper Functions that are not part of the IRGenerator class =====
template <class T>
constexpr FORCE_INLINE unsigned getBitWidth() {
    return static_cast<unsigned>(sizeof(T) * CHAR_BIT);
}

}  // namespace codegen
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_MIDDLEEND_CODEGEN_CODEGEN_BASE_H