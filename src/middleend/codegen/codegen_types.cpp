#include <llvm/IR/Value.h>

#include <middleend/codegen/codegen_base.hpp>

namespace Manganese {

namespace codegen {

auto IRGenerator::visit(ast::AggregateType* type) -> typevisit_t {
    DISCARD(type);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::ArrayType* type) -> typevisit_t {
    DISCARD(type);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::FunctionType* type) -> typevisit_t {
    DISCARD(type);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::GenericType* type) -> typevisit_t {
    DISCARD(type);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::PointerType* type) -> typevisit_t {
    DISCARD(type);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::SymbolType* type) -> typevisit_t {
    DISCARD(type);
    NOT_IMPLEMENTED("Codegen is not available yet");
}

}  // namespace codegen

}  // namespace Manganese