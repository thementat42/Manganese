#include <core.hpp>
#include <frontend/ast.hpp>
#include <frontend/semantic.hpp>
#include <io/logging.hpp>
#include <mnstl/number.hxx>
#include <utils/result.hpp>

namespace Manganese {
namespace semantic {

// auto analyzer::visit(ast::AggregateInstantiationExpression* expression) -> exprvisit_t;
// auto analyzer::visit(ast::AggregateLiteralExpression* expression) -> exprvisit_t;
// auto analyzer::visit(ast::ArrayLiteralExpression* expression) -> exprvisit_t;
// auto analyzer::visit(ast::AssignmentExpression* expression) -> exprvisit_t;
// auto analyzer::visit(ast::BinaryExpression* expression) -> exprvisit_t;
auto analyzer::visit(ast::BoolLiteralExpression* expression) -> exprvisit_t {
    expression->semanticType = typeContext.getPrimitive(ast::PrimitiveType_t::boolean);
    return Result::Success;
}
auto analyzer::visit(ast::CharLiteralExpression* expression) -> exprvisit_t {
    expression->semanticType = typeContext.getPrimitive(ast::PrimitiveType_t::character);
    return Result::Success;
}
// auto analyzer::visit(ast::FunctionCallExpression* expression) -> exprvisit_t;
// auto analyzer::visit(ast::GenericExpression* expression) -> exprvisit_t;
// auto analyzer::visit(ast::IdentifierExpression* expression) -> exprvisit_t;
// auto analyzer::visit(ast::IndexExpression* expression) -> exprvisit_t;
// auto analyzer::visit(ast::MemberAccessExpression* expression) -> exprvisit_t;
auto analyzer::visit(ast::NumberLiteralExpression* expression) -> exprvisit_t {
    using held_type = mnstl::number_t::held_type;
    using prim_t = ast::PrimitiveType_t;
    switch (expression->value.underlying_type()) {
        case held_type::int8: expression->semanticType = typeContext.getPrimitive(prim_t::i8); break;
        case held_type::int16: expression->semanticType = typeContext.getPrimitive(prim_t::i16); break;
        case held_type::int32: expression->semanticType = typeContext.getPrimitive(prim_t::i32); break;
        case held_type::int64: expression->semanticType = typeContext.getPrimitive(prim_t::i64); break;
        case held_type::int128: expression->semanticType = typeContext.getPrimitive(prim_t::i128); break;
        case held_type::uint8: expression->semanticType = typeContext.getPrimitive(prim_t::u8); break;
        case held_type::uint16: expression->semanticType = typeContext.getPrimitive(prim_t::u16); break;
        case held_type::uint32: expression->semanticType = typeContext.getPrimitive(prim_t::u32); break;
        case held_type::uint64: expression->semanticType = typeContext.getPrimitive(prim_t::u64); break;
        case held_type::uint128: expression->semanticType = typeContext.getPrimitive(prim_t::u128); break;
        case held_type::float32: expression->semanticType = typeContext.getPrimitive(prim_t::f32); break;
        case held_type::float64: expression->semanticType = typeContext.getPrimitive(prim_t::f64); break;
        case held_type::none: [[fallthrough]];
        default: ASSERT_UNREACHABLE("In analyzer: Number literal expression had no parser-deduced type");
    }
    return Result::Success;
}

// auto analyzer::visit(ast::PostfixExpression* expression) -> exprvisit_t;
// auto analyzer::visit(ast::PrefixExpression* expression) -> exprvisit_t;
// auto analyzer::visit(ast::ScopeResolutionExpression* expression) -> exprvisit_t;
auto analyzer::visit(ast::StringLiteralExpression* expression) -> exprvisit_t {
    expression->semanticType = typeContext.getPrimitive(ast::PrimitiveType_t::str);
    return Result::Success;
}
// auto analyzer::visit(ast::TypeCastExpression* expression) -> exprvisit_t;

}  // namespace semantic
}  // namespace Manganese