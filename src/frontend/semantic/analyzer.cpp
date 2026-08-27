#include <core.hpp>
#include <format>
#include <frontend/ast.hpp>
#include <frontend/lexer/token.hpp>
#include <frontend/semantic/analyzer.hpp>
#include <frontend/semantic/type_context.hpp>
#include <mnstl/enum_matches.hxx>
#include <mnstl/fold_result.hxx>
#include <utility>
#include <utils/result.hpp>
#include <utils/type_names.hpp>

namespace Manganese::semantic {

Result Analyzer::analyze() {
    // Don't want errors cascading because of conflicting redeclarations
    if (buildScopeTree() == Result::Failure) { return Result::Failure; }
    symbolTable.switchToCheckingMode();
    if (collectGlobals() == Result::Failure) { return Result::Failure; }
    return checkStatements();
}

Result Analyzer::collectGlobals() {
    Result result = Result::Success;
    for (ast::Statement* statement : parsedFile.program) {
        if (_collectGlobalsInStatement(statement, ast::StatementKind::AggregateDeclarationStatement)
            == Result::Failure) {
            result = Result::Failure;
        }
    }

    // Do this on a separate pass in case a function uses an aggregate in its signature
    for (ast::Statement* statement : parsedFile.program) {
        if (_collectGlobalsInStatement(statement, ast::StatementKind::FunctionDeclarationStatement)
            == Result::Failure) {
            result = Result::Failure;
        }
    }

    return result;
}

Result Analyzer::_collectGlobalsInStatement(ast::Statement* statement, ast::StatementKind targetKind) {
    if (statement->kind == targetKind) {
        if (targetKind == ast::StatementKind::AggregateDeclarationStatement) {
            return collectGlobalAggregate(static_cast<ast::AggregateDeclarationStatement*>(statement));
        }
        if (targetKind == ast::StatementKind::FunctionDeclarationStatement) {
            return collectGlobalFunction(static_cast<ast::FunctionDeclarationStatement*>(statement));
        }
    }
    if (statement->kind == ast::StatementKind::NamespaceStatement) {
        auto* namespaceStatement = static_cast<ast::NamespaceStatement*>(statement);
        symbolTable.enterNamespace(namespaceStatement->name, namespaceStatement);
        Result result = Result::Success;
        for (ast::Statement* innerStatement : namespaceStatement->block) {
            if (_collectGlobalsInStatement(innerStatement, targetKind) == Result::Failure) { result = Result::Failure; }
        }

        symbolTable.exitNamespace();
        return result;
    }
    // something else we don't care about
    return Result::Success;
}

bool Analyzer::isMutableExpression(const ast::Expression* expr) {
    if (!expr) { return false; }
    using enum ast::ExpressionKind;
    switch (expr->kind) {
        case IdentifierExpression: {
            const auto* id = static_cast<const ast::IdentifierExpression*>(expr);
            const Symbol* symbol = symbolTable.lookup(id->name);
            return symbol ? symbol->isMutable : false;
        }
        case PrefixExpression: {
            const auto* prefix = static_cast<const ast::PrefixExpression*>(expr);
            if (prefix->op == lexer::TokenType::Dereference) {
                const SemanticType* opType = prefix->right->semanticType;
                if (!opType || !opType->isPointer()) { return false; }
                return static_cast<const Pointer*>(opType)->isMutable;
            }
            return false;
        }
        case MemberAccessExpression: {
            const auto* mem = static_cast<const ast::MemberAccessExpression*>(expr);
            return isMutableExpression(mem->object);
        }
        case IndexExpression: {
            const auto* index = static_cast<const ast::IndexExpression*>(expr);
            return isMutableExpression(index->variable);
        }
        case ScopeResolutionExpression: {
            return false;  // TODO
        }
        default: return false;
    }
}

Result Analyzer::collectGlobalAggregate(ast::AggregateDeclarationStatement* aggregate) {
    // Skip uninstantiated generics
    if (!aggregate->genericTypes.empty()) { return Result::Success; }

    Symbol* symbol = symbolTable.lookup(aggregate->name);
    if (!symbol) {
        ASSERT_UNREACHABLE(std::format("Aggregate '{}' was not recorded during type collection", aggregate->name));
    }

    // Skip if already processed
    if (symbol->type) { return Result::Success; }

    Result result = Result::Success;
    std::vector<AggregateField> fields;
    fields.reserve(aggregate->fields.size());

    for (const auto& field : aggregate->fields) {
        const typevisit_t fieldResult = visit(field.type);
        if (fieldResult == typevisit_t::Failure) {
            logError(aggregate, "Unknown type '{}' for field '{}' in aggregate '{}'", field.type->toString(),
                     field.name, aggregate->name);
            result = Result::Failure;
        }

        const SemanticType* fieldType
            = (fieldResult == typevisit_t::Failure ? typeContext.getVoid() : field.type->semanticType);
        fields.push_back(AggregateField{.name = field.name, .type = fieldType});
    }

    symbol->type = typeContext.getNamedAggregate(aggregate->name, std::move(fields));
    symbol->status = ResolutionStatus::NotStarted;

    return result;
}

Result Analyzer::collectGlobalFunction(ast::FunctionDeclarationStatement* function) {
    // Skip uninstantiated generics
    if (!function->genericTypes.empty()) { return Result::Success; }

    Symbol* symbol = symbolTable.lookup(function->name);
    if (!symbol) {
        ASSERT_UNREACHABLE(std::format("Function '{}' was not recorded during type collection", function->name));
    }

    // Detect direct signature cycles (e.g. infinite parameter expansion)
    if (symbol->status == ResolutionStatus::InProgress) {
        logError(function, "Cyclic dependency detected in signature of function '{}'", function->name);
        symbol->status = ResolutionStatus::Failure;
        return Result::Failure;
    }

    symbol->status = ResolutionStatus::InProgress;

    std::vector<Parameter> paramTypes;
    paramTypes.reserve(function->parameters.size());
    Result funcResult = Result::Success;

    for (const auto& param : function->parameters) {
        const typevisit_t paramResult = visit(param.type);
        if (paramResult == typevisit_t::Failure) {
            logError(function, "Unknown parameter type '{}' for parameter '{}' in function '{}'",
                     param.type->toString(), param.name, function->name);
            funcResult = Result::Failure;
        }

        const SemanticType* paramType
            = (paramResult == typevisit_t::Failure ? typeContext.getVoid() : param.type->semanticType);
        paramTypes.push_back(
            Parameter{.isMutable = param.isMutable, .isVariadic = param.isVariadic, .type = paramType});
    }

    const SemanticType* resolvedReturnType = typeContext.getVoid();
    if (function->returnType) {
        const typevisit_t returnResult = visit(function->returnType);
        if (returnResult == typevisit_t::Failure) {
            logError(function, "Unknown return type '{}' in function '{}'", function->returnType->toString(),
                     function->name);
            funcResult = Result::Failure;
        } else {
            resolvedReturnType = function->returnType->semanticType;
        }
    }

    symbol->type = typeContext.getFunction(std::move(paramTypes), resolvedReturnType);
    symbol->status = ResolutionStatus::NotStarted;

    return funcResult;
}

Result Analyzer::checkStatements() {  // semantic analysis pass (this can also check the generic specializations)
    Result programIsSemanticallyValid = Result::Success;
    for (ast::Statement* stmt : parsedFile.program) {
        if (this->visit(stmt) == Result::Failure) { programIsSemanticallyValid = Result::Failure; }
    }
    return programIsSemanticallyValid;
}

}  // namespace Manganese::semantic
