#include <core.hpp>
#include <format>
#include <frontend/ast.hpp>
#include <frontend/semantic.hpp>
#include <io/logging.hpp>
#include <utils/result.hpp>

namespace Manganese::semantic {

auto Analyzer::visit(ast::EmptyStatement*) -> stmtvisit_t {
    return stmtvisit_t::Success;  // nothing to check
}

auto Analyzer::visit(ast::ExpressionStatement* statement) -> stmtvisit_t {
    if (visit(statement->expression) == exprvisit_t::Failure) { return stmtvisit_t::Failure; }

    // Crucial distinction:
    if (!statement->expression->semanticType) { return stmtvisit_t::Failure; }
    return stmtvisit_t::Success;
}

auto Analyzer::visit(ast::NestedBlockStatement* statement) -> stmtvisit_t { return visit(statement->block); }

auto Analyzer::visit([[maybe_unused]] ast::NamespaceStatement* statement) -> stmtvisit_t {
    return stmtvisit_t::Success;
}

auto Analyzer::visit(ast::ReturnStatement* statement) -> stmtvisit_t {
    if (!context.inFunction) {
        logError(statement, "'return' can only be used in a function");
        return stmtvisit_t::Failure;
    }

    // void return
    if (!statement->value) {
        if (context.currentFunctionReturnType != typeContext.getVoid()) {
            logError(statement, "Non-void function must return a value");
            return stmtvisit_t::Failure;
        }
        return stmtvisit_t::Success;
    }

    if (visit(statement->value) == stmtvisit_t::Failure) { return stmtvisit_t::Failure; }
    if (!statement->value->semanticType) { logError(statement->value, "Could not deduce type of return expression"); }

    if (!areTypesCompatible(statement->value->semanticType, context.currentFunctionReturnType)) {
        logError(statement, "Function returns '{}' but expression in return statement has type '{}'",
                 (context.currentFunctionReturnType ? context.currentFunctionReturnType->toString() : "void"),
                 statement->value->semanticType->toString());
        return stmtvisit_t::Failure;
    }
    return stmtvisit_t::Success;
}

}  // namespace Manganese::semantic
