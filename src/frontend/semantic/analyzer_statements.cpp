#include <core.hpp>
#include <format>
#include <frontend/ast.hpp>
#include <frontend/semantic.hpp>
#include <io/logging.hpp>
#include <utils/result.hpp>

namespace Manganese::semantic {

auto analyzer::visit(ast::AggregateDeclarationStatement* statement) -> stmtvisit_t {
    // We don't know the generic types at declaration so we can't check them
    // Instead, check only when they're instantiated
    if (!statement->genericTypes.empty()) { return Result::Success; }

    Symbol* symbol = symbolTable.lookup(statement->name);
    if (!symbol) {
        ASSERT_UNREACHABLE(std::format("Aggregate '{}' was not logged in the symbol table", statement->name));
    }

    const auto* aggregateType = static_cast<const Aggregate*>(symbol->type);

    if (aggregateType->status == ResolutionStatus::InProgress) {
        logError(statement, "Aggregate '{}' eventually contains itself through a dependency chain", statement->name);
        return Result::Failure;
    }

    if (aggregateType->status == ResolutionStatus::Success) { return Result::Success; }

    aggregateType->status = ResolutionStatus::InProgress;

    std::vector<AggregateField> fieldTypes;
    fieldTypes.reserve(statement->fields.size());

    for (const ast::AggregateField& field : statement->fields) {
        visit(field.type);
        const SemanticType* resolvedFieldType = field.type->semanticType;
        if (!resolvedFieldType) {
            logging::logError(field.line, field.column, "Unknown type for field '{}' in aggregate '{}'", field.name,
                              statement->name);
            return Result::Failure;
        }

        if (resolvedFieldType->isAggregate()) {
            const auto* nestedAggregateType = static_cast<const Aggregate*>(resolvedFieldType);
            Symbol* nestedSymbol = symbolTable.lookup(nestedAggregateType->name);
            if (nestedSymbol && nestedSymbol->node) {
                // Cast to non-const ast::Statement* so visit() can accept it
                auto* nestedStmt = static_cast<ast::Statement*>(nestedSymbol->node);

                if (visit(nestedStmt) == Result::Failure) {
                    aggregateType->status = ResolutionStatus::Failure;
                    return Result::Failure;
                }
            }
        }
        fieldTypes.push_back(AggregateField{.name = field.name, .type = resolvedFieldType});
    }

    aggregateType->fields = std::move(fieldTypes);
    aggregateType->status = ResolutionStatus::Success;
    return Result::Success;
}

auto analyzer::visit([[maybe_unused]] ast::AliasStatement* statement) -> stmtvisit_t { return Result::Success; }

auto analyzer::visit(ast::BreakStatement* statement) -> stmtvisit_t {
    if (!context.whileLoopDepth && !context.forLoopDepth && !context.switchStatementDepth) {
        logError(statement, "'break' can only be used in loops or switch statements");
        return Result::Failure;
    }
    return Result::Success;
}

auto analyzer::visit(ast::ContinueStatement* statement) -> stmtvisit_t {
    if (!context.whileLoopDepth && !context.forLoopDepth) {
        logError(statement, "'continue' can only be used in loops");
        return Result::Failure;
    }
    return Result::Success;
}

auto analyzer::visit(ast::EmptyStatement*) -> stmtvisit_t {
    return Result::Success;  // nothing to check
}

auto analyzer::visit([[maybe_unused]] ast::EnumDeclarationStatement* statement) -> stmtvisit_t {
    return Result::Success;
}

auto analyzer::visit(ast::ExpressionStatement* statement) -> stmtvisit_t { return visit(statement->expression); }

auto analyzer::visit(ast::ForLoopStatement* statement) -> stmtvisit_t {
    auto result = Result::Success;
    const ContextGuard guard{context.forLoopDepth,
                             static_cast<decltype(context.forLoopDepth)>(context.forLoopDepth + 1)};
    bool blockNeedsToEnterScope = true;

    if (statement->initializationStep) {
        // We want the variable to be declared inside the scope of the for loop
        // since we enter a scope here, we need to the body visitor know it's already in the appropriate scope
        blockNeedsToEnterScope = false;
        symbolTable.enterScope();
        if (visit(statement->initializationStep) == Result::Failure) { result = Result::Failure; }
    }
    if (statement->stopCondition) {
        // there is a stop condition, check it
        if (visit(statement->stopCondition) == Result::Failure) { result = Result::Failure; }
        if (!statement->stopCondition->semanticType) {
            logError(statement->stopCondition, "Could not deduce type of for loop stop condition {}",
                     statement->stopCondition->toString());
            result = Result::Failure;
        } else {
            const typeCompatibilityResult conditionCanBeBool = areTypesCompatible(
                statement->stopCondition->semanticType, typeContext.getPrimitive(ast::PrimitiveType_t::boolean));

            if (!conditionCanBeBool) {
                logError(statement,
                         "Stop condition in for loop must be a boolean type or implicitly convertible to it, not {}",
                         statement->stopCondition->semanticType->toString());
                result = Result::Failure;
            } else if (conditionCanBeBool.result == Compatible_t::Warning) {
                logWarning(statement, "{}", conditionCanBeBool.message);
            }
        }
    }
    if (statement->postExpression) {
        // there is a post expression, check it
        if (visit(statement->postExpression) == Result::Failure) { result = Result::Failure; }
    }
    if (visit(statement->body, blockNeedsToEnterScope) == Result::Failure) { result = Result::Failure; }
    return result;
}

auto analyzer::visit(ast::FunctionDeclarationStatement* statement) -> stmtvisit_t {
    if (context.inFunction) {
        logError(statement,
                 "Nested functions are not supported: function '{}' cannot be declared inside another function",
                 statement->name);
        return Result::Failure;
    }
    const ContextGuard guard{context.inFunction, true};
    // We don't know the generic types at declaration so we can't check them
    // Instead, check only when they're instantiated
    if (!statement->genericTypes.empty()) { return Result::Success; }
    const SemanticType* resolvedReturnType = nullptr;
    if (statement->returnType) {
        visit(statement->returnType);
        resolvedReturnType = statement->returnType->semanticType;
        if (!resolvedReturnType) {
            logError(statement, "Unknown return type for function '{}'", statement->name);
            return Result::Failure;
        }
    }
    symbolTable.enterScope();
    for (const ast::FunctionParameter& param : statement->parameters) {
        visit(param.type);
        const SemanticType* resolvedParamType = param.type->semanticType;
        if (!resolvedParamType) {
            logError(statement, "Unknown type for parameter '{}' in function '{}'", param.name, statement->name);
            symbolTable.exitScope();
            return Result::Failure;
        }
        Result paramDeclaration = symbolTable.declare(
            param.name,
            Symbol{.type = resolvedParamType,
                   .node = statement,
                   .kind = (param.isMutable) ? SymbolKind::Parameter : SymbolKind::ConstantParameter,
                   .isMutable = param.isMutable});
        if (paramDeclaration == Result::Failure) [[unlikely]] {
            logError(statement, "Failed to declare parameter '{}' in scope for function '{}'", param.name,
                     statement->name);
            symbolTable.exitScope();
            return Result::Failure;
        }
    }
    context.currentFunctionReturnType = resolvedReturnType;
    // already entered a scope for the parameters so we don't want to enter a scope while visiting the body
    Result bodyResult = visit(statement->body, false);
    context.currentFunctionReturnType = nullptr;
    return bodyResult;
}

auto analyzer::visit(ast::IfStatement* statement) -> stmtvisit_t {
    auto result = Result::Success;
    const ContextGuard guard{context.ifStatementDepth,
                             static_cast<decltype(context.ifStatementDepth)>(context.ifStatementDepth + 1)};

    if (visit(statement->condition) == Result::Failure) { result = Result::Failure; }

    if (!statement->condition->semanticType) {
        logError(statement, "Could not deduce type of condition {}", statement->condition->toString());
        result = Result::Failure;
    } else {
        const typeCompatibilityResult conditionCanBeBool = areTypesCompatible(
            statement->condition->semanticType, typeContext.getPrimitive(ast::PrimitiveType_t::boolean));

        if (!conditionCanBeBool) {
            logError(statement,
                     "Condition in if statement must be a boolean type or implicitly convertible to it, not {}",
                     statement->condition->semanticType->toString());
            result = Result::Failure;
        } else if (conditionCanBeBool.result == Compatible_t::Warning) {
            logWarning(statement, "{}", conditionCanBeBool.message);
        }
    }

    if (visit(statement->body) == Result::Failure) { result = Result::Failure; }

    for (ast::ElifClause& elif : statement->elifs) {
        visit(elif.condition);

        if (!elif.condition->semanticType) {
            logError(statement, "Could not deduce type of condition {}", elif.condition->toString());
            result = Result::Failure;
        } else {
            const typeCompatibilityResult conditionCanBeBool = areTypesCompatible(
                elif.condition->semanticType, typeContext.getPrimitive(ast::PrimitiveType_t::boolean));

            if (!conditionCanBeBool) {
                logError(statement,
                         "Condition in elif statement must be a boolean type or implicitly convertible to it, not {}",
                         elif.condition->semanticType->toString());
                result = Result::Failure;
            } else if (conditionCanBeBool.result == Compatible_t::Warning) {
                logWarning(statement, "{}", conditionCanBeBool.message);
            }
        }

        if (visit(elif.body) == Result::Failure) { result = Result::Failure; }
    }

    if (statement->elseBody.size() != 0) {
        // there is an else body
        if (visit(statement->elseBody) == Result::Failure) { result = Result::Failure; }
    }
    return result;
}

auto analyzer::visit(ast::NestedBlockStatement* statement) -> stmtvisit_t { return visit(statement->block); }

auto analyzer::visit(ast::ReturnStatement* statement) -> stmtvisit_t {
    if (!context.inFunction) {
        logError(statement, "'return' can only be used in a function");
        return Result::Failure;
    }

    // void return
    if (!statement->value) {
        if (context.currentFunctionReturnType != nullptr) {
            logError(statement, "Non-void function must return a value");
            return Result::Failure;
        }
        return Result::Success;
    }

    if (visit(statement->value) == Result::Failure) { return Result::Failure; }
    if (!statement->value->semanticType) { logError(statement->value, "Could not deduce type of return expression"); }

    if (!areTypesCompatible(statement->value->semanticType, context.currentFunctionReturnType)) {
        logError(statement, "Function returns '{}' but expression in return statement has type '{}'",
                 (context.currentFunctionReturnType ? context.currentFunctionReturnType->toString() : "void"),
                 statement->value->semanticType->toString());
        return Result::Failure;
    }
    return Result::Success;
}

auto analyzer::visit([[maybe_unused]] ast::SwitchStatement* statement) -> stmtvisit_t { return Result::Success; }
auto analyzer::visit([[maybe_unused]] ast::VariableDeclarationStatement* statement) -> stmtvisit_t {
    return Result::Success;
}

auto analyzer::visit(ast::WhileLoopStatement* statement) -> stmtvisit_t {
    const ContextGuard guard{context.whileLoopDepth,
                             static_cast<decltype(context.whileLoopDepth)>(context.whileLoopDepth + 1)};

    auto result = Result::Success;

    if (visit(statement->condition) == Result::Failure) { result = Result::Failure; }
    if (!statement->condition->semanticType) {
        logError(statement, "Could not deduce type of expression {}", statement->condition->toString());
        return Result::Failure;
    } else {
        const typeCompatibilityResult conditionCanBeBool = areTypesCompatible(
            statement->condition->semanticType, typeContext.getPrimitive(ast::PrimitiveType_t::boolean));

        if (!conditionCanBeBool) {
            logError(statement, "While loop condition must be a boolean value or implicitly convertible to it, not {}",
                     statement->condition->semanticType->toString());
        } else if (conditionCanBeBool.result == Compatible_t::Warning) {
            logWarning(statement, "{}", conditionCanBeBool.message);
        }
    }

    if (visit(statement->body) == Result::Failure) { result = Result::Failure; }

    return result;
}

}  // namespace Manganese::semantic
