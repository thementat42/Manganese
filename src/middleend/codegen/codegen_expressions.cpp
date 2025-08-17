#include <llvm/IR/Value.h>

#include <middleend/codegen/codegen_base.hpp>


namespace Manganese {

namespace codegen {

auto IRGenerator::visit(ast::AggregateInstantiationExpression* expression) -> visit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::ArrayLiteralExpression* expression) -> visit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::AssignmentExpression* expression) -> visit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::BinaryExpression* expression) -> visit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::BoolLiteralExpression* expression) -> visit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::CharLiteralExpression* expression) -> visit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::FunctionCallExpression* expression) -> visit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::GenericExpression* expression) -> visit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::IdentifierExpression* expression) -> visit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::IndexExpression* expression) -> visit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::MemberAccessExpression* expression) -> visit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::NumberLiteralExpression* expression) -> visit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::PostfixExpression* expression) -> visit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::PrefixExpression* expression) -> visit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::ScopeResolutionExpression* expression) -> visit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::StringLiteralExpression* expression) -> visit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::TypeCastExpression* expression) -> visit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}

}  // namespace codegen

}  // namespace Manganese