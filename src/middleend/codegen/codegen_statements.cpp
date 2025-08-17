#include <llvm/IR/Value.h>

#include <middleend/codegen/codegen_base.hpp>

namespace Manganese {

namespace codegen {

auto IRGenerator::visit(ast::AggregateDeclarationStatement* statement) -> visit_t {
    DISCARD(statement);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::AliasStatement* statement) -> visit_t {
    DISCARD(statement);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::BreakStatement* statement) -> visit_t {
    DISCARD(statement);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::ContinueStatement* statement) -> visit_t {
    DISCARD(statement);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::EmptyStatement* statement) -> visit_t {
    DISCARD(statement);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::EnumDeclarationStatement* statement) -> visit_t {
    DISCARD(statement);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::ExpressionStatement* statement) -> visit_t {
    DISCARD(statement);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::FunctionDeclarationStatement* statement) -> visit_t {
    DISCARD(statement);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::IfStatement* statement) -> visit_t {
    DISCARD(statement);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::RepeatLoopStatement* statement) -> visit_t {
    DISCARD(statement);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::ReturnStatement* statement) -> visit_t {
    DISCARD(statement);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::SwitchStatement* statement) -> visit_t {
    DISCARD(statement);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::VariableDeclarationStatement* statement) -> visit_t {
    DISCARD(statement);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::WhileLoopStatement* statement) -> visit_t {
    DISCARD(statement);
    NOT_IMPLEMENTED("Codegen is not available yet");
}

}  // namespace codegen

}  // namespace Manganese