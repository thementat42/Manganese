#ifndef MANGANESE_INCLUDE_MIDDLEEND_CODEGEN_IR_GENERATOR_HPP
#define MANGANESE_INCLUDE_MIDDLEEND_CODEGEN_IR_GENERATOR_HPP 1

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include <frontend/ast.hpp>
#include <frontend/parser.hpp>
#include <memory>
#include <mnstl/tiny_stack.hxx>
#include <string_view>

namespace Manganese::codegen {

using _irgen_base_t = ast::Visitor<llvm::Value*, void, llvm::Type*, false>;

class IRGenerator final : public _irgen_base_t {
   private:
    llvm::LLVMContext& context;
    std::unique_ptr<llvm::Module> module;
    llvm::IRBuilder<> builder;
    mnstl::tiny_stack<std::unordered_map<std::string_view, llvm::Value*>> scopes;
    std::vector<parser::ParsedFile>& files;

   public:
    IRGenerator(llvm::LLVMContext& llvmContext, std::string_view moduleName,
                std::vector<parser::ParsedFile>& parsedFiles) :
        context(llvmContext),
        module(std::make_unique<llvm::Module>(moduleName, context)),
        builder(context),
        files(parsedFiles) {}

    std::unique_ptr<llvm::Module> takeModule() noexcept { return std::move(module); }

   protected:
    // overrides for visitor functions
    using _irgen_base_t::visit;

#define STMT(name) stmtvisit_t visit(const ast::name*) override;
#define EXPR(name) exprvisit_t visit(const ast::name*) override;
#define TYPE(name) typevisit_t visit(const ast::name*) override;
#include <frontend/ast/ast.def>
#undef STMT
#undef EXPR
#undef TYPE
};

}  // namespace Manganese::codegen

#endif  // MANGANESE_INCLUDE_MIDDLEEND_CODEGEN_IR_GENERATOR_HPP