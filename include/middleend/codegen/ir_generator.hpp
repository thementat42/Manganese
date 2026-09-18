#include "utils/target_info.hpp"
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
#include <utils/target_info.hpp>

namespace Manganese::codegen {

using _irgen_base_t = ast::Visitor<llvm::Value*, void, llvm::Type*, false>;

class IRGenerator final : public _irgen_base_t {
   private:
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    std::unordered_map<std::string, llvm::Value*> namedValues;
    std::vector<parser::ParsedFile>& files;
    utils::TargetInfo targetInfo;

   public:
    IRGenerator(std::string_view moduleName, std::vector<parser::ParsedFile>& parsedFiles, utils::TargetInfo info) :
        context(std::make_unique<llvm::LLVMContext>()),
        module(std::make_unique<llvm::Module>(moduleName, *context)),
        builder(std::make_unique<llvm::IRBuilder<>>(*context)),
        files(parsedFiles),
        targetInfo(info) {}

    std::unique_ptr<llvm::Module> takeModule() noexcept { return std::move(module); }

    void generate() noexcept {
        for (const parser::ParsedFile& file : files) {
            for (const ast::Statement* statement : file.program) { visit(statement); }
        }
    }

    [[nodiscard]] llvm::IntegerType* getLLVMIntegerType(const ast::NumberLiteralExpression* expression,
                                                        std::string_view lexeme) const;

   protected:
    // overrides for visitor functions
    using _irgen_base_t::visit;

#define STMT(name) stmtvisit_t visit(const ast::name*) override;
#define EXPR(name) [[nodiscard]] exprvisit_t visit(const ast::name*) override;
#define TYPE(name) [[nodiscard]] typevisit_t visit(const ast::name*) override;
#include <frontend/ast/ast.def>
#undef STMT
#undef EXPR
#undef TYPE
    [[nodiscard]] typevisit_t visit(const semantic::SemanticType*);
};

}  // namespace Manganese::codegen

#endif  // MANGANESE_INCLUDE_MIDDLEEND_CODEGEN_IR_GENERATOR_HPP