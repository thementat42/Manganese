#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include <core.hpp>
#include <frontend/ast.hpp>
#include <frontend/semantic.hpp>
#include <middleend/codegen/ir_generator.hpp>


namespace Manganese::codegen {

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const semantic::SemanticType* type) -> typevisit_t {
    return nullptr;
}

// Visitors for AST types (just call the semantic type visitors)

[[nodiscard]] auto IRGenerator::visit(const ast::AggregateType* type) -> typevisit_t {
    return visit(type->semanticType);
}

[[nodiscard]] auto IRGenerator::visit(const ast::ArrayType* type) -> typevisit_t { return visit(type->semanticType); }

[[nodiscard]] auto IRGenerator::visit(const ast::FunctionType* type) -> typevisit_t {
    return visit(type->semanticType);
}

[[nodiscard]] auto IRGenerator::visit(const ast::GenericInstantiationType* type) -> typevisit_t {
    return visit(type->semanticType);
}

[[nodiscard]] auto IRGenerator::visit(const ast::PointerType* type) -> typevisit_t { return visit(type->semanticType); }

[[nodiscard]] auto IRGenerator::visit(const ast::ScopedType* type) -> typevisit_t { return visit(type->semanticType); }

[[nodiscard]] auto IRGenerator::visit(const ast::IdentifierType* type) -> typevisit_t {
    return visit(type->semanticType);
}

[[nodiscard]] auto IRGenerator::visit(const ast::TypeofType* type) -> typevisit_t { return visit(type->semanticType); }

[[nodiscard]] auto IRGenerator::visit(const ast::PoisonedType* /*unused*/) -> typevisit_t {
    ASSERT_UNREACHABLE("Poison Type node was not caught during semantic analysis");
}

}  // namespace Manganese::codegen