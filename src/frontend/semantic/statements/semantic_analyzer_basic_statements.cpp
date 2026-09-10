#include <core.hpp>
#include <format>
#include <frontend/ast.hpp>
#include <frontend/semantic.hpp>
#include <io/logging.hpp>
#include <utils/result.hpp>

namespace Manganese::semantic {

auto SemanticAnalyzer::visit(ast::EmptyStatement* /*unused*/) -> stmtvisit_t {
    return stmtvisit_t::Success;  // nothing to check
}

auto SemanticAnalyzer::visit(ast::ExpressionStatement* statement) -> stmtvisit_t {
    if (visit(statement->expression) == exprvisit_t::Failure) {
        statement->expression->semanticType = typeContext.getPoison();
        return stmtvisit_t::Failure;
    }
#if MN_DEBUG
    if (statement->expression->semanticType == nullptr) {
        logging::logInternal(logging::LogLevel::Warning, "Expression '{}' did not have its semantic type set",
                             statement->expression->toString());
        statement->expression->semanticType = typeContext.getPoison();
        return stmtvisit_t::Failure;
    }
#endif
    return stmtvisit_t::Success;
}

auto SemanticAnalyzer::visit(ast::NestedBlockStatement* statement) -> stmtvisit_t { return visit(statement->block); }

auto SemanticAnalyzer::visit(ast::ModuleDeclarationStatement* /*unused*/) -> stmtvisit_t {
    return stmtvisit_t::Success;
}
auto SemanticAnalyzer::visit(ast::ImportStatement* /*unused*/) -> stmtvisit_t { return stmtvisit_t::Success; }

auto SemanticAnalyzer::visit(ast::NamespaceStatement* statement) -> stmtvisit_t {
    symbolTable.enterNamespace(statement->name, statement);
    stmtvisit_t result = stmtvisit_t::Success;
    for (ast::Statement* stmt : statement->block) {
        if (visit(stmt) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }
    }
    symbolTable.exitNamespace();
    return result;
}

auto SemanticAnalyzer::visit(ast::ReturnStatement* statement) -> stmtvisit_t {
    if (!context.inFunction) {
        logError(statement, "'return' can only be used in a function");
        return stmtvisit_t::Failure;
    }

    // void return
    if (statement->value == nullptr) {
        if (context.currentFunctionReturnType != typeContext.getVoid()) {
            logError(statement, "Non-void function must return a value");
            return stmtvisit_t::Failure;
        }
        return stmtvisit_t::Success;
    }

    if (visit(statement->value) == stmtvisit_t::Failure) { return stmtvisit_t::Failure; }
    if (statement->value->semanticType->isPoison()) {
        logError(statement->value, "Could not deduce type of return expression");
    }

    if (!areTypesCompatible(statement->value->semanticType, context.currentFunctionReturnType)) {
        logError(
            statement, "Function returns '{}' but expression in return statement has type '{}'",
            (context.currentFunctionReturnType != nullptr ? context.currentFunctionReturnType->toString() : "void"),
            statement->value->semanticType->toString());
        return stmtvisit_t::Failure;
    }
    return stmtvisit_t::Success;
}

auto SemanticAnalyzer::visit(ast::PoisonedStatement* /*unused*/) -> stmtvisit_t { return stmtvisit_t::Failure; }

}  // namespace Manganese::semantic
