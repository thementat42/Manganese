#ifndef MANGANESE_INCLUDE_MIDDLEEND_CODEGEN_IR_GENERATOR_HPP
#define MANGANESE_INCLUDE_MIDDLEEND_CODEGEN_IR_GENERATOR_HPP 1

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include <frontend/ast.hpp>
#include <frontend/lexer/token.hpp>
#include <frontend/parser.hpp>
#include <frontend/semantic.hpp>
#include <frontend/semantic/type_context.hpp>
#include <memory>
#include <mnstl/tiny_stack.hxx>
#include <ostream>
#include <string_view>
#include <utils/target_info.hpp>

namespace Manganese::codegen {

using _irgen_base_t = ast::Visitor<llvm::Value*, void, llvm::Type*, false>;

struct LoopTarget {
    llvm::BasicBlock* continueBlock;  // where a 'continue' statement jumps to
    llvm::BasicBlock* exitBlock;  // where a 'break' statement jumps to
};

class IRGenerator final : public _irgen_base_t {
   private:
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    std::unordered_map<std::string, llvm::Value*> namedValues;
    std::unordered_map<const semantic::SemanticType*, llvm::StructType*> savedTypes;
    std::vector<parser::ParsedFile>& files;
    utils::TargetInfo targetInfo;
    mnstl::tiny_stack<LoopTarget> loopStack;
    semantic::SemanticAnalyzer& analyzer;

   public:
    IRGenerator(std::string_view moduleName, std::vector<parser::ParsedFile>& parsedFiles,
                semantic::SemanticAnalyzer& analyzerReference, utils::TargetInfo info) :
        context(std::make_unique<llvm::LLVMContext>()),
        module(std::make_unique<llvm::Module>(moduleName, *context)),
        builder(std::make_unique<llvm::IRBuilder<>>(*context)),
        files(parsedFiles),
        targetInfo(info),
        analyzer(analyzerReference) {}

    std::unique_ptr<llvm::Module> takeModule() noexcept { return std::move(module); }
    llvm::Module* getModule() const noexcept { return module.get(); }

    void generate() noexcept;
    void dump(std::ostream&) const;

   protected:
    // overrides for visitor functions
    using _irgen_base_t::visit;

    [[nodiscard]] exprvisit_t visit(const ast::Expression* expr) { return _irgen_base_t::visit(expr); }
    stmtvisit_t visit(const ast::Statement* stmt) { _irgen_base_t::visit(stmt); }
    typevisit_t visit(const ast::Type* type) { return _irgen_base_t::visit(type); }
    [[nodiscard]] typevisit_t visit(const semantic::SemanticType*);

#define STMT(name)     stmtvisit_t visit(const ast::name*) override;
#define EXPR(name)     [[nodiscard]] exprvisit_t visit(const ast::name*) override;
#define TYPE(name)     [[nodiscard]] typevisit_t visit(const ast::name*) override;
#define SEMANTIC(name) [[nodiscard]] typevisit_t visit(const semantic::name*);
#include <frontend/ast/ast.def>
#undef STMT
#undef EXPR
#undef TYPE
#undef SEMANTIC

    stmtvisit_t visit(semantic::generic_tag_t, const ast::FunctionDeclarationStatement*, std::string_view mangledName,
                      const semantic::InstantiationKey& key);

    [[nodiscard]] typevisit_t visitTypeAsValue(const semantic::SemanticType* type);

    stmtvisit_t visit(const ast::Block& block);
    [[nodiscard]] typevisit_t visit(const semantic::Aggregate*);
    [[nodiscard]] typevisit_t visit(const semantic::Array*);
    [[nodiscard]] typevisit_t visit(const semantic::Enum*);
    [[nodiscard]] typevisit_t visit(const semantic::Function*);
    [[nodiscard]] typevisit_t visit(const semantic::Pointer*);
    [[nodiscard]] static typevisit_t visit(const semantic::Poison*) NOEXCEPT_IF_RELEASE;
    [[nodiscard]] typevisit_t visit(const semantic::Uninitialized*);
    [[nodiscard]] typevisit_t visit(const semantic::Void*);
    [[nodiscard]] typevisit_t getPrimitiveType(ast::PrimitiveType type);

    [[nodiscard]] llvm::IntegerType* getLLVMIntegerType(const ast::NumberLiteralExpression* expression,
                                                        std::string_view lexeme) const;
    [[nodiscard]] llvm::Value* convertNumberToType(llvm::Value* val, const semantic::SemanticType* fromType,
                                                   const semantic::SemanticType* toType);
    static llvm::CmpInst::Predicate getFloatPredicate(lexer::TokenType op) NOEXCEPT_IF_RELEASE;
    static llvm::CmpInst::Predicate getIntPredicate(lexer::TokenType op, bool isSigned) NOEXCEPT_IF_RELEASE;
    [[nodiscard]] exprvisit_t getLValue(const ast::Expression* expr);
};

}  // namespace Manganese::codegen

#endif  // MANGANESE_INCLUDE_MIDDLEEND_CODEGEN_IR_GENERATOR_HPP