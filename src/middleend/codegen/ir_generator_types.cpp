#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
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
        case Primitive: return getPrimitiveType(type->primitiveType);
    };
    ASSERT_UNREACHABLE("Unknown semantic type kind in IRGenerator::visit(SemanticType*)");
}

[[nodiscard]] auto IRGenerator::visit(const semantic::Aggregate* type) -> typevisit_t {
    std::vector<llvm::Type*> elementTypes;
    elementTypes.reserve(type->fields.size());
    for (const semantic::AggregateField& field : type->fields) { elementTypes.push_back(visit(field.type)); }
    return llvm::StructType::get(*context, elementTypes);
}

[[nodiscard]] auto IRGenerator::visit(const semantic::Array* type) -> typevisit_t {
    llvm::Type* elementLLVMType = visit(type->elementType);
    return llvm::ArrayType::get(elementLLVMType, *type->length);
}

[[nodiscard]] auto IRGenerator::visit(const semantic::Enum* type) -> typevisit_t { return visit(type->underlyingType); }

[[nodiscard]] auto IRGenerator::visit(const semantic::Function* type) -> typevisit_t {
    llvm::Type* returnTypeLLVM = visit(type->returnType);
    std::vector<llvm::Type*> parameterLLVMTypes;
    parameterLLVMTypes.reserve(type->parameterTypes.size());
    for (const auto& parameter : type->parameterTypes) { parameterLLVMTypes.push_back(visit(parameter.type)); }
    return llvm::FunctionType::get(returnTypeLLVM, parameterLLVMTypes, /*isVarArg=*/false);
}

[[nodiscard]] auto IRGenerator::visit(const semantic::Pointer* /*unused*/) -> typevisit_t {
    return builder->getPtrTy();
}

[[nodiscard]] auto IRGenerator::visit(const semantic::Poison* /*unused*/) -> typevisit_t {
    ASSERT_UNREACHABLE("Poison semantic type was not flagged as semantically invalid");
}

[[nodiscard]] auto IRGenerator::visit(const semantic::Uninitialized* /*unused*/) -> typevisit_t { return nullptr; }

[[nodiscard]] auto IRGenerator::visit(const semantic::Void* /*unused*/) -> typevisit_t { return builder->getVoidTy(); }

[[nodiscard]] auto IRGenerator::getPrimitiveType(ast::PrimitiveType type) -> typevisit_t {
    // Note: LLVM doesn't have a signed/unsigned distinction so signed and unsigned ints of the same bit width
    // map to the same LLVM type
    using enum ast::PrimitiveType;
    switch (type) {
        case ast::PrimitiveType::int8:
        case ast::PrimitiveType::uint8: return builder->getInt8Ty();
        case ast::PrimitiveType::int16:
        case ast::PrimitiveType::uint16: return builder->getInt16Ty();
        case ast::PrimitiveType::int32:
        case ast::PrimitiveType::uint32:
        case ast::PrimitiveType::character:  // char32 is a 32-bit int
            return builder->getInt32Ty();
        case ast::PrimitiveType::int64:
        case ast::PrimitiveType::uint64: return builder->getInt64Ty();
        case ast::PrimitiveType::int128:
        case ast::PrimitiveType::uint128: return builder->getInt128Ty();
        case ast::PrimitiveType::float32: return builder->getFloatTy();
        case ast::PrimitiveType::float64: return builder->getDoubleTy();
        case ast::PrimitiveType::boolean:
            // LLVM doesn't have a separate boolean type, just a "1-bit integer"
            return builder->getInt1Ty();
        case ast::PrimitiveType::string: {
            llvm::Type* charPointerType = llvm::PointerType::get(*context, 0);
            llvm::Type* lengthType = builder->getInt64Ty();
            // strings are a pointer + a length so return an anonymous struct
            // needs to be handled by language runtime
            return llvm::StructType::get(*context, {charPointerType, lengthType});
        }
        case ast::PrimitiveType::not_primitive:
            ASSERT_UNREACHABLE("IRGenerator::getPrimitiveType called on non-primitive semantic type");
    }
    ASSERT_UNREACHABLE("Unknown primitive type in IRGenerator::getPrimitiveType");
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