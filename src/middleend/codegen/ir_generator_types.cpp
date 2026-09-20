#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include <core.hpp>
#include <frontend/ast.hpp>
#include <frontend/semantic.hpp>
#include <middleend/codegen/ir_generator.hpp>

namespace Manganese::codegen {

[[nodiscard]] auto IRGenerator::visit(const semantic::SemanticType* type) -> typevisit_t {
    using enum semantic::SemanticTypeKind;
    switch (type->kind) {
        case Aggregate: return visit(static_cast<const semantic::Aggregate*>(type));
        case Array: return visit(static_cast<const semantic::Array*>(type));
        case Enum: return visit(static_cast<const semantic::Enum*>(type));
        case Function: return visit(static_cast<const semantic::Function*>(type));
        case Pointer: return visit(static_cast<const semantic::Pointer*>(type));
        case Poison: return visit(static_cast<const semantic::Poison*>(type));
        case Uninitialized: return visit(static_cast<const semantic::Uninitialized*>(type));
        case Void: return visit(static_cast<const semantic::Void*>(type));
        case Generic: {
            // TODO
            return nullptr;
        }
        case Primitive: {
            // TODO
            return decltype(nullptr){};
        }
    };
    ASSERT_UNREACHABLE("Unknown semantic type kind in IRGenerator::visit(SemanticType*)");
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const semantic::Aggregate* type) -> typevisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const semantic::Array* type) -> typevisit_t { return nullptr; }

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const semantic::Enum* type) -> typevisit_t { return nullptr; }

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const semantic::Function* type) -> typevisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const semantic::Pointer* type) -> typevisit_t { return nullptr; }

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const semantic::Poison* type) -> typevisit_t { return nullptr; }

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const semantic::Uninitialized* type) -> typevisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const semantic::Void* type) -> typevisit_t { return nullptr; }

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