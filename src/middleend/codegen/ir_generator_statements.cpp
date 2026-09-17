#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include <frontend/ast.hpp>
#include <middleend/codegen/ir_generator.hpp>

namespace Manganese::codegen {

auto IRGenerator::visit([[maybe_unused]] const ast::AggregateDeclarationStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit([[maybe_unused]] const ast::AliasStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit([[maybe_unused]] const ast::BreakStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit([[maybe_unused]] const ast::ContinueStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit([[maybe_unused]] const ast::EmptyStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit([[maybe_unused]] const ast::EnumDeclarationStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit([[maybe_unused]] const ast::ExpressionStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit([[maybe_unused]] const ast::ForLoopStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit([[maybe_unused]] const ast::FunctionDeclarationStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit([[maybe_unused]] const ast::IfStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit([[maybe_unused]] const ast::ImportStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit([[maybe_unused]] const ast::ModuleDeclarationStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit([[maybe_unused]] const ast::NamespaceStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit([[maybe_unused]] const ast::NestedBlockStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit([[maybe_unused]] const ast::ReturnStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit([[maybe_unused]] const ast::SwitchStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit([[maybe_unused]] const ast::VariableDeclarationStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit([[maybe_unused]] const ast::WhileLoopStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit([[maybe_unused]] const ast::PoisonedStatement* statement) -> stmtvisit_t {}

}  // namespace Manganese::codegen