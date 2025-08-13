#include <llvm/IR/Value.h>

#include <middleend/codegen/codegen_base.hpp>

namespace Manganese {

namespace codegen {

llvm::Value* generateAggregateType(const ast::AggregateType* type) {
    DISCARD(type);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
llvm::Value* generateArrayType(const ast::ArrayType* type) {
    DISCARD(type);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
llvm::Value* generateFunctionType(const ast::FunctionType* type) {
    DISCARD(type);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
llvm::Value* generateGenericType(const ast::GenericType* type) {
    DISCARD(type);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
llvm::Value* generatePointerType(const ast::PointerType* type) {
    DISCARD(type);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
llvm::Value* generateSymbolType(const ast::SymbolType* type) {
    DISCARD(type);
    NOT_IMPLEMENTED("Codegen is not available yet");
}

}  // namespace codegen

}  // namespace Manganese