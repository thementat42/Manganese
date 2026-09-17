#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include <frontend/ast.hpp>
#include <middleend/codegen/ir_generator.hpp>

namespace Manganese::codegen {

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::AggregateInstantiationExpression* expression)
    -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::AggregateLiteralExpression* expression)
    -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::AlignofExpression* expression) -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::ArrayLiteralExpression* expression) -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::AssignmentExpression* expression) -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::BinaryExpression* expression) -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::BoolLiteralExpression* expression) -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::CharLiteralExpression* expression) -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::FunctionCallExpression* expression) -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::GenericInstantiationExpression* expression)
    -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::IdentifierExpression* expression) -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::IndexExpression* expression) -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::MemberAccessExpression* expression) -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::NumberLiteralExpression* expression) -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::PostfixExpression* expression) -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::PrefixExpression* expression) -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::ScopeResolutionExpression* expression)
    -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::SizeofExpression* expression) -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::StringLiteralExpression* expression) -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::TypeCastExpression* expression) -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::UninitializedExpression* expression) -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::PoisonedExpression* expression) -> exprvisit_t {
    return nullptr;
}

}  // namespace Manganese::codegen