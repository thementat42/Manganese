#include <llvm/IR/Value.h>

#include <middleend/codegen/codegen_base.hpp>

namespace Manganese {

namespace codegen {

auto IRGenerator::visit(ast::AggregateDeclarationStatement* statement) -> stmtvisit_t {
    DISCARD(statement);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::AliasStatement* statement) -> stmtvisit_t {
    DISCARD(statement);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::BreakStatement* statement) -> stmtvisit_t {
    DISCARD(statement);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::ContinueStatement* statement) -> stmtvisit_t {
    DISCARD(statement);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::EmptyStatement* statement) -> stmtvisit_t {
    DISCARD(statement);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::EnumDeclarationStatement* statement) -> stmtvisit_t {
    DISCARD(statement);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::ExpressionStatement* statement) -> stmtvisit_t {
    DISCARD(statement);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::FunctionDeclarationStatement* statement) -> stmtvisit_t {
    DISCARD(statement);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::IfStatement* statement) -> stmtvisit_t {
    DISCARD(statement);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::RepeatLoopStatement* statement) -> stmtvisit_t {
    DISCARD(statement);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::ReturnStatement* statement) -> stmtvisit_t {
    DISCARD(statement);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::SwitchStatement* statement) -> stmtvisit_t {
    DISCARD(statement);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::VariableDeclarationStatement* statement) -> stmtvisit_t {
    DISCARD(statement);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::WhileLoopStatement* statement) -> stmtvisit_t {
    DISCARD(statement);
    NOT_IMPLEMENTED("Codegen is not available yet");
}

}  // namespace codegen

}  // namespace Manganese