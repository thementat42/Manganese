#include <core.hpp>
#include <frontend/ast.hpp>
#include <frontend/semantic.hpp>

namespace Manganese {
namespace semantic {

auto analyzer::visit(ast::AggregateType* type) -> typevisit_t {
    return (type->semanticType = resolveType(type)) ? Result::Success : Result::Failure;
}
auto analyzer::visit(ast::ArrayType* type) -> typevisit_t {
    return (type->semanticType = resolveType(type)) ? Result::Success : Result::Failure;
}
auto analyzer::visit(ast::FunctionType* type) -> typevisit_t {
    return (type->semanticType = resolveType(type)) ? Result::Success : Result::Failure;
}
auto analyzer::visit(ast::GenericType* type) -> typevisit_t {
    return (type->semanticType = resolveType(type)) ? Result::Success : Result::Failure;
}
auto analyzer::visit(ast::PointerType* type) -> typevisit_t {
    return (type->semanticType = resolveType(type)) ? Result::Success : Result::Failure;
}
auto analyzer::visit(ast::SymbolType* type) -> typevisit_t {
    return (type->semanticType = resolveType(type)) ? Result::Success : Result::Failure;
}

}  // namespace semantic
}  // namespace Manganese
