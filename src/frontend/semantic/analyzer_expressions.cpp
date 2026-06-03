#include <core.hpp>
#include <frontend/ast.hpp>
#include <frontend/semantic.hpp>
#include <io/logging.hpp>
#include <mnstl/number.hxx>
#include <utils/result.hpp>


namespace Manganese {
namespace semantic {

auto analyzer::visit(ast::AggregateInstantiationExpression* expression) -> exprvisit_t {
    /*
    checking something like

    Point@[int] {
        .x = 3, .y = 4
    }
    Need:
    - Type to exist
    - Type to actually be an aggregate
    - Number of generic parameters to match
    - Number of initializers to match
    - Fields to match (have to be in order)
    - For each field, the assignment expression must be valid and have a type that can be converted to the type the
    member was declared with

    */

    const Symbol* aggregateSymbol = symbolTable.lookup(expression->name);
    auto line = expression->getLine();
    auto col = expression->getColumn();
    if (!aggregateSymbol) {
        logging::logError(line, col, "Aggregate type '{}' was not declared in any scope", expression->name);
        return Result::Failure;
    }
    if (aggregateSymbol->kind != SymbolKind::Aggregate) {
        logging::logError(line, col, "'{}' is not an aggregate type so cannot be instantiated as one.",
                          expression->name);
        return Result::Failure;
    }
    ast::AggregateDeclarationStatement* declarationNode
        = static_cast<ast::AggregateDeclarationStatement*>(aggregateSymbol->node);

    if (declarationNode->genericTypes.size() != expression->genericTypes.size()) {
        logging::logError(line, col,
                          "Aggregate type '{}' has '{}' generic type parameters, but {} parameters were provided",
                          expression->name, declarationNode->genericTypes.size(), expression->genericTypes.size());
        return Result::Failure;
    }
    if (expression->fields.size() != declarationNode->fields.size()) {
        logging::logError(line, col, "Aggregate type '{}' has '{}' membbers, but {} initializers were provided",
                          expression->name, declarationNode->fields.size(), expression->fields.size());
        return Result::Failure;
    }

    // Check each field in the aggregate instantiation to make sure it matches the definition
    Result result = Result::Success;
    for (std::size_t i = 0; i < expression->fields.size(); ++i) {
        const auto& field = expression->fields[i];
        const auto& expectedField = declarationNode->fields[i];
        if (field.name != expectedField.name) {
            logging::logError(
                line, col, "Field {} in aggregate instantiation does not match field name {} in aggregate type {} {}",
                field.name, expectedField.name, expression->name,
                "(Note: aggregate fields must be instantiated in order)");
            result = Result::Failure;
        }

        visit(field.value);  // check the instantiation expression

        if (!areTypesCompatible(field.value->getType(), expectedField.type.get())) {
            logging::logError(
                line, col,
                "Field {} in aggregate instantiation has type {}, but was declared in {} with type {} (cannot convert between {} and {})",
                ast::toStringOr(field.value), ast::toStringOr(field.value->getType()), declarationNode->name,
                ast::toStringOr(expectedField.type), ast::toStringOr(field.value->getType()),
                ast::toStringOr(expectedField.type));
            result = Result::Failure;
        }
    }

    // Leave type as null for an invalid type
    if (result != Result::Failure) { expression->setType(std::make_shared<ast::SymbolType>(expression->name)); }
    return result;
}

// auto analyzer::visit(ast::AggregateLiteralExpression* expression) -> exprvisit_t;
// auto analyzer::visit(ast::ArrayLiteralExpression* expression) -> exprvisit_t;
// auto analyzer::visit(ast::AssignmentExpression* expression) -> exprvisit_t;
// auto analyzer::visit(ast::BinaryExpression* expression) -> exprvisit_t;

auto analyzer::visit(ast::BoolLiteralExpression* expression) -> exprvisit_t {
    expression->setType(primitiveTypes.boolean);
    return Result::Success;  // literals are always semantically correct
}
auto analyzer::visit(ast::CharLiteralExpression* expression) -> exprvisit_t {
    expression->setType(primitiveTypes.character);
    return Result::Success;  // literals are always semantically correct
}

// auto analyzer::visit(ast::FunctionCallExpression* expression) -> exprvisit_t;
// auto analyzer::visit(ast::GenericExpression* expression) -> exprvisit_t;
// auto analyzer::visit(ast::IdentifierExpression* expression) -> exprvisit_t;
// auto analyzer::visit(ast::IndexExpression* expression) -> exprvisit_t;
// auto analyzer::visit(ast::MemberAccessExpression* expression) -> exprvisit_t;

auto analyzer::visit(ast::NumberLiteralExpression* expression) -> exprvisit_t {
    using held_type = mnstl::number_t::held_type;
    switch (expression->value.underlying_type()) {
        case held_type::int8: expression->setType(primitiveTypes.int8); break;
        case held_type::int16: expression->setType(primitiveTypes.int16); break;
        case held_type::int32: expression->setType(primitiveTypes.int32); break;
        case held_type::int64: expression->setType(primitiveTypes.int64); break;
        case held_type::int128: expression->setType(primitiveTypes.int128); break;
        case held_type::uint8: expression->setType(primitiveTypes.uint8); break;
        case held_type::uint16: expression->setType(primitiveTypes.uint16); break;
        case held_type::uint32: expression->setType(primitiveTypes.uint32); break;
        case held_type::uint64: expression->setType(primitiveTypes.uint64); break;
        case held_type::uint128: expression->setType(primitiveTypes.uint128); break;
        case held_type::float32: expression->setType(primitiveTypes.float32); break;
        case held_type::float64: expression->setType(primitiveTypes.float64); break;
        case held_type::none: [[fallthrough]];
        default: ASSERT_UNREACHABLE("In analyzer: Number literal expression had no parser-deduced type");
    }
    return Result::Success;  // literals are always semantically correct
}

// auto analyzer::visit(ast::PostfixExpression* expression) -> exprvisit_t;
// auto analyzer::visit(ast::PrefixExpression* expression) -> exprvisit_t;
// auto analyzer::visit(ast::ScopeResolutionExpression* expression) -> exprvisit_t;

auto analyzer::visit(ast::StringLiteralExpression* expression) -> exprvisit_t {
    expression->setType(primitiveTypes.string);
    return Result::Success;  // literals are always semantically correct
}

// auto analyzer::visit(ast::TypeCastExpression* expression) -> exprvisit_t;

}  // namespace semantic
}  // namespace Manganese