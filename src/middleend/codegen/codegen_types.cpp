#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Value.h>

#include <middleend/codegen/codegen_base.hpp>
#include <vector>


namespace Manganese {

namespace codegen {

typedef int address_space_t;

[[maybe_unused]] constexpr address_space_t LLVM_GLOBAL_ADDRESS_SPACE = 0;
[[maybe_unused]] constexpr address_space_t LLVM_LOCAL_ADDRESS_SPACE = 1;
[[maybe_unused]] constexpr address_space_t LLVM_CONSTANT_ADDRESS_SPACE = 2;

auto IRGenerator::visit(ast::AggregateType* type) -> typevisit_t {
    DISCARD(type);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::ArrayType* type) -> typevisit_t {
    DISCARD(type);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::FunctionType* type) -> typevisit_t {
    // TODO: Allow variadic arguments
    typevisit_t returnType = type->returnType ? visit(type->returnType) : llvm::Type::getVoidTy(*theContext);
    std::vector<typevisit_t> argTypes;
    argTypes.reserve(type->parameterTypes.size());
    for (auto& paramType : type->parameterTypes) { argTypes.push_back(visit(paramType.type)); }
    return llvm::FunctionType::get(returnType, argTypes, false);
}
auto IRGenerator::visit(ast::GenericType* type) -> typevisit_t {
    DISCARD(type);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::PointerType* type) -> typevisit_t {
    // TODO: Specialize address space
    return llvm::PointerType::get(visit(type->baseType), /*AddressSpace=*/LLVM_GLOBAL_ADDRESS_SPACE);
}
auto IRGenerator::visit(ast::SymbolType* type) -> typevisit_t {
    DISCARD(type);
    NOT_IMPLEMENTED("Codegen is not available yet");
}

}  // namespace codegen

}  // namespace Manganese