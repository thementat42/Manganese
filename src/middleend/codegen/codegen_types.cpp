#include <llvm/IR/Value.h>

#include <middleend/codegen/codegen_base.hpp>

namespace Manganese {

namespace codegen {

auto IRGenerator::visit(ast::AggregateType* type) -> visit_t {
    DISCARD(type);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::ArrayType* type) -> visit_t {
    DISCARD(type);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::FunctionType* type) -> visit_t {
    DISCARD(type);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::GenericType* type) -> visit_t {
    DISCARD(type);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::PointerType* type) -> visit_t {
    DISCARD(type);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::SymbolType* type) -> visit_t {
    DISCARD(type);
    NOT_IMPLEMENTED("Codegen is not available yet");
}

}  // namespace codegen

}  // namespace Manganese