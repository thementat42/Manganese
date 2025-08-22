#ifndef MANGANESE_INCLUDE_MIDDLEEND_CODEGEN_CODEGEN_BASE_HPP
#define MANGANESE_INCLUDE_MIDDLEEND_CODEGEN_CODEGEN_BASE_HPP

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include <climits>
#include <frontend/ast.hpp>
#include <frontend/parser.hpp>
#include <frontend/visitor/visitor_base.hpp>
#include <global_macros.hpp>
#include <map>
#include <memory>
#include <string>

namespace Manganese {
namespace codegen {

class IRGenerator final : public visitor::Visitor<llvm::Value*, void, llvm::Type*> {
   private:
    using visitor::Visitor<llvm::Value*, void, llvm::Type*>::visit;
    std::unique_ptr<llvm::LLVMContext> theContext;
    std::unique_ptr<llvm::Module> theModule;
    std::unique_ptr<llvm::IRBuilder<>> theBuilder;
    std::map<std::string, llvm::Value*> namedValues;

   public:
    explicit IRGenerator() noexcept :
        theContext(std::make_unique<llvm::LLVMContext>()),
        theModule(std::make_unique<llvm::Module>("Manganese Module", *theContext)),
        theBuilder(std::make_unique<llvm::IRBuilder<>>(*theContext)) {}
    ~IRGenerator() noexcept = default;

    llvm::LLVMContext* getContext() noexcept { return theContext.get(); }
    const llvm::LLVMContext* getContext() const noexcept { return theContext.get(); }
    llvm::Module* getModule() noexcept { return theModule.get(); }
    const llvm::Module* getModule() const noexcept { return theModule.get(); }
    llvm::IRBuilder<>* getBuilder() noexcept { return theBuilder.get(); }
    const llvm::IRBuilder<>* getBuilder() const noexcept { return theBuilder.get(); }

    void generate(parser::ParsedFile& parsedFile) {
        for (const auto& statement : parsedFile.program) { visit(statement.get()); }
    }

   private:
    // ===== Specific Expression Code Generation =====
    exprvisit_t visit(ast::AggregateInstantiationExpression*) override;
    exprvisit_t visit(ast::ArrayLiteralExpression*) override;
    exprvisit_t visit(ast::AssignmentExpression*) override;
    exprvisit_t visit(ast::BinaryExpression*) override;
    exprvisit_t visit(ast::BoolLiteralExpression*) override;
    exprvisit_t visit(ast::CharLiteralExpression*) override;
    exprvisit_t visit(ast::FunctionCallExpression*) override;
    exprvisit_t visit(ast::GenericExpression*) override;
    exprvisit_t visit(ast::IdentifierExpression*) override;
    exprvisit_t visit(ast::IndexExpression*) override;
    exprvisit_t visit(ast::MemberAccessExpression*) override;
    exprvisit_t visit(ast::NumberLiteralExpression*) override;
    exprvisit_t visit(ast::PostfixExpression*) override;
    exprvisit_t visit(ast::PrefixExpression*) override;
    exprvisit_t visit(ast::ScopeResolutionExpression*) override;
    exprvisit_t visit(ast::StringLiteralExpression*) override;
    exprvisit_t visit(ast::TypeCastExpression*) override;

    // ===== Specific Statement Code Generation =====
    stmtvisit_t visit(ast::AggregateDeclarationStatement*) override;
    stmtvisit_t visit(ast::AliasStatement*) override;
    stmtvisit_t visit(ast::BreakStatement*) override;
    stmtvisit_t visit(ast::ContinueStatement*) override;
    stmtvisit_t visit(ast::EnumDeclarationStatement*) override;
    stmtvisit_t visit(ast::EmptyStatement*) override;
    stmtvisit_t visit(ast::ExpressionStatement*) override;
    stmtvisit_t visit(ast::FunctionDeclarationStatement*) override;
    stmtvisit_t visit(ast::IfStatement*) override;
    stmtvisit_t visit(ast::RepeatLoopStatement*) override;
    stmtvisit_t visit(ast::ReturnStatement*) override;
    stmtvisit_t visit(ast::SwitchStatement*) override;
    stmtvisit_t visit(ast::VariableDeclarationStatement*) override;
    stmtvisit_t visit(ast::WhileLoopStatement*) override;

    // ===== Specific Type Code Generation =====
    typevisit_t visit(ast::AggregateType*) override;
    typevisit_t visit(ast::ArrayType*) override;
    typevisit_t visit(ast::FunctionType*) override;
    typevisit_t visit(ast::GenericType*) override;
    typevisit_t visit(ast::PointerType*) override;
    typevisit_t visit(ast::SymbolType*) override;

    // ===== Helper Functions =====
    exprvisit_t generateShortCircuitBinaryExpression(ast::BinaryExpression* expression) noexcept;
};

// ===== Helper Functions that are not part of the IRGenerator class =====
template <class T>
constexpr FORCE_INLINE unsigned getBitWidth() {
    return static_cast<unsigned>(sizeof(T) * CHAR_BIT);
}

}  // namespace codegen
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_MIDDLEEND_CODEGEN_CODEGEN_BASE_H