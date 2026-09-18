#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include <frontend/ast.hpp>
#include <middleend/codegen/ir_generator.hpp>

namespace Manganese::codegen {

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::AggregateType* type) -> typevisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::ArrayType* type) -> typevisit_t { return nullptr; }

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::FunctionType* type) -> typevisit_t { return nullptr; }

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::GenericInstantiationType* type) -> typevisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::PointerType* type) -> typevisit_t { return nullptr; }

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::ScopedType* type) -> typevisit_t { return nullptr; }

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::IdentifierType* type) -> typevisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::TypeofType* type) -> typevisit_t { return nullptr; }

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::PoisonedType* type) -> typevisit_t { return nullptr; }

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const semantic::SemanticType* type) -> typevisit_t {
    return nullptr;
}

}  // namespace Manganese::codegen