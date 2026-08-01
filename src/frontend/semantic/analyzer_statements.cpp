#include <core.hpp>
#include <cstdint>
#include <format>
#include <frontend/ast.hpp>
#include <frontend/semantic.hpp>
#include <io/logging.hpp>
#include <utils/result.hpp>

namespace Manganese::semantic {

auto analyzer::visit(ast::AggregateDeclarationStatement* statement) -> stmtvisit_t {
    // We don't know the generic types at declaration so we can't check them
    // Instead, check only when they're instantiated
    if (!statement->genericTypes.empty()) { return stmtvisit_t::Success; }

    Symbol* symbol = symbolTable.lookup(statement->name);
    if (!symbol) {
        ASSERT_UNREACHABLE(std::format("Aggregate '{}' was not logged in the symbol table", statement->name));
    }

    const auto* aggregateType = static_cast<const Aggregate*>(symbol->type);

    if (aggregateType->status == ResolutionStatus::InProgress) {
        logError(statement, "Aggregate '{}' eventually contains itself through a dependency chain", statement->name);
        return stmtvisit_t::Failure;
    }

    if (aggregateType->status == ResolutionStatus::Success) { return stmtvisit_t::Success; }

    aggregateType->status = ResolutionStatus::InProgress;

    std::vector<AggregateField> fieldTypes;
    fieldTypes.reserve(statement->fields.size());

    for (const ast::AggregateField& field : statement->fields) {
        visit(field.type);
        const SemanticType* resolvedFieldType = field.type->semanticType;
        if (!resolvedFieldType) {
            logging::logError(field.line, field.column, "Unknown type for field '{}' in aggregate '{}'", field.name,
                              statement->name);
            return stmtvisit_t::Failure;
        }

        if (resolvedFieldType->isAggregate()) {
            const auto* nestedAggregateType = static_cast<const Aggregate*>(resolvedFieldType);
            Symbol* nestedSymbol = symbolTable.lookup(nestedAggregateType->name);
            if (nestedSymbol && nestedSymbol->node) {
                // Cast to non-const ast::Statement* so visit() can accept it
                auto* nestedStmt = static_cast<ast::Statement*>(nestedSymbol->node);

                if (visit(nestedStmt) == stmtvisit_t::Failure) {
                    aggregateType->status = ResolutionStatus::Failure;
                    return stmtvisit_t::Failure;
                }
            }
        }
        fieldTypes.push_back(AggregateField{.name = field.name, .type = resolvedFieldType});
    }

    aggregateType->fields = std::move(fieldTypes);
    aggregateType->status = ResolutionStatus::Success;
    return stmtvisit_t::Success;
}

auto analyzer::visit(ast::AliasStatement* statement) -> stmtvisit_t {
    Symbol* symbol = symbolTable.lookup(statement->alias);
    if (!symbol) {
        ASSERT_UNREACHABLE(
            std::format("Alias symbol '{}' was not registered during type collection", statement->alias));
    }
    // Already resolved
    if (symbol->status == ResolutionStatus::Success) { return stmtvisit_t::Success; }
    if (symbol->status == ResolutionStatus::Failure) { return stmtvisit_t::Failure; }

    // Cycle Detection
    if (symbol->status == ResolutionStatus::InProgress) {
        logError(statement, "Cyclic type alias detected in the definition of alias '{}'", statement->alias);
        symbol->status = ResolutionStatus::Failure;
        return stmtvisit_t::Failure;
    }

    symbol->status = ResolutionStatus::InProgress;
    if (visit(statement->baseType) == stmtvisit_t::Failure) {
        symbol->status = ResolutionStatus::Failure;
        return stmtvisit_t::Failure;
    }
    symbol->type = statement->baseType->semanticType;
    symbol->status = ResolutionStatus::Success;
    return stmtvisit_t::Success;
}

auto analyzer::visit(ast::BreakStatement* statement) -> stmtvisit_t {
    if (!context.whileLoopDepth && !context.forLoopDepth && !context.switchStatementDepth) {
        logError(statement, "'break' can only be used in loops or switch statements");
        return stmtvisit_t::Failure;
    }
    return stmtvisit_t::Success;
}

auto analyzer::visit(ast::ContinueStatement* statement) -> stmtvisit_t {
    if (!context.whileLoopDepth && !context.forLoopDepth) {
        logError(statement, "'continue' can only be used in loops");
        return stmtvisit_t::Failure;
    }
    return stmtvisit_t::Success;
}

auto analyzer::visit(ast::EmptyStatement*) -> stmtvisit_t {
    return stmtvisit_t::Success;  // nothing to check
}

auto analyzer::visit(ast::EnumDeclarationStatement* statement) -> stmtvisit_t {
    stmtvisit_t result = stmtvisit_t::Success;
    Symbol* symbol = symbolTable.lookup(statement->name);
    if (!symbol) {
        ASSERT_UNREACHABLE(std::format("Enum {} was not registered during type initalization", statement->name));
    }

    if (symbol->status == ResolutionStatus::Success) { return stmtvisit_t::Success; }
    if (symbol->status == ResolutionStatus::Failure) { return stmtvisit_t::Failure; }

    symbol->status = ResolutionStatus::InProgress;

    const SemanticType* underlyingType = nullptr;
    if (statement->baseType) {
        if (visit(statement->baseType) == stmtvisit_t::Failure) {
            symbol->status = ResolutionStatus::Failure;
            result = stmtvisit_t::Failure;
        }
        underlyingType = statement->baseType->semanticType;
    } else {
        // If not specified, use int32
        underlyingType = typeContext.getPrimitive(ast::PrimitiveType_t::i32);
    }
    const Enum* enumType = static_cast<const Enum*>(typeContext.getEnum(statement->name));
    enumType->underlyingType = underlyingType;
    symbol->type = enumType;

    int64_t currentVariantValue = 0;

    std::vector<Variant> variants;

    for (ast::EnumValue& variant : statement->values) {
        if (variant.value) {
            if (visit(variant.value) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }
            auto explicitVal = variant.value->fold();
            if (!explicitVal.has_value()) {
                logging::logError(variant.line, variant.column,
                                  "Variant {} (in enum {}) must have a compile-time value", variant.name,
                                  statement->name);
                symbol->status = ResolutionStatus::Failure;
                result = stmtvisit_t::Failure;
            }
            if (!explicitVal.is_number() || !explicitVal.number_unchecked().is_integer()) {
                logging::logError(variant.line, variant.column, "Variant {} (in enum {}) must have an integer value",
                                  variant.name, statement->name);
                symbol->status = ResolutionStatus::Failure;
                result = stmtvisit_t::Failure;
            }
            currentVariantValue = explicitVal.number_unchecked().value_as<int64_t>();
        }

        variants.emplace_back(variant.name, currentVariantValue);
        currentVariantValue++;
    }
    enumType->variants = std::move(variants);
    return result;
}

auto analyzer::visit(ast::ExpressionStatement* statement) -> stmtvisit_t { return visit(statement->expression); }

auto analyzer::visit(ast::ForLoopStatement* statement) -> stmtvisit_t {
    auto result = stmtvisit_t::Success;
    const ContextGuard guard{context.forLoopDepth,
                             static_cast<decltype(context.forLoopDepth)>(context.forLoopDepth + 1)};
    bool blockNeedsToEnterScope = true;

    if (statement->initializationStep) {
        // We want the variable to be declared inside the scope of the for loop
        // since we enter a scope here, we need to the body visitor know it's already in the appropriate scope
        blockNeedsToEnterScope = false;
        symbolTable.enterScope();
        if (visit(statement->initializationStep) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }
    }

    if (statement->stopCondition) {
        // there is a stop condition, check it
        if (visit(statement->stopCondition) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }
        if (!statement->stopCondition->semanticType) {
            logError(statement->stopCondition, "Could not deduce type of for loop stop condition {}",
                     statement->stopCondition->toString());
            result = stmtvisit_t::Failure;
        } else {
            const typeCompatibilityResult conditionCanBeBool = areTypesCompatible(
                statement->stopCondition->semanticType, typeContext.getPrimitive(ast::PrimitiveType_t::boolean));

            if (!conditionCanBeBool) {
                logError(statement,
                         "Stop condition in for loop must be a boolean type or implicitly convertible to it, not {}",
                         statement->stopCondition->semanticType->toString());
                result = stmtvisit_t::Failure;
            } else if (conditionCanBeBool.result == Compatible_t::Warning) {
                logWarning(statement, "{}", conditionCanBeBool.message);
            }
        }
    }
    if (statement->postExpression) {
        // there is a post expression, check it
        if (visit(statement->postExpression) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }
    }
    if (visit(statement->body, blockNeedsToEnterScope) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }
    return result;
}

auto analyzer::visit(ast::FunctionDeclarationStatement* statement) -> stmtvisit_t {
    if (context.inFunction) {
        logError(statement,
                 "Nested functions are not supported: function '{}' cannot be declared inside another function",
                 statement->name);
        return stmtvisit_t::Failure;
    }

    // We don't know the generic types at declaration so we can't check them
    // Instead, check only when they're instantiated
    if (!statement->genericTypes.empty()) { return stmtvisit_t::Success; }

    Symbol* symbol = symbolTable.lookup(statement->name);
    if (!symbol) {
        ASSERT_UNREACHABLE(
            std::format("Function '{}' was not registered during symbol initialization", statement->name));
    }

    if (symbol->status == ResolutionStatus::Success) { return stmtvisit_t::Success; }
    if (symbol->status == ResolutionStatus::Failure) { return stmtvisit_t::Failure; }

    // Direct circular dependencies in function signatures (e.g. infinite type expansions)
    if (symbol->status == ResolutionStatus::InProgress) {
        // Recursive calls inside bodies are fine because body resolution happens while InProgress.
        // Re-entering here only happens if signature resolution loops.
        logError(statement, "Cyclic dependency detected in signature of function '{}'", statement->name);
        symbol->status = ResolutionStatus::Failure;
        return stmtvisit_t::Failure;
    }

    symbol->status = ResolutionStatus::InProgress;

    const ContextGuard contextGuard{context.inFunction, true};
    const SemanticType* resolvedReturnType = nullptr;
    stmtvisit_t signatureResult = stmtvisit_t::Success;

    if (statement->returnType) {
        visit(statement->returnType);
        resolvedReturnType = statement->returnType->semanticType;
        if (!resolvedReturnType) {
            logError(statement, "Unknown return type for function '{}'", statement->name);
            signatureResult = stmtvisit_t::Failure;
        }
    }

    symbolTable.enterScope();
    for (const ast::FunctionParameter& param : statement->parameters) {
        visit(param.type);
        const SemanticType* resolvedParamType = param.type->semanticType;
        if (!resolvedParamType) {
            logError(statement, "Unknown type for parameter '{}' in function '{}'", param.name, statement->name);

            symbol->status = ResolutionStatus::Failure;
            signatureResult = stmtvisit_t::Failure;
        }
        stmtvisit_t paramDeclaration = symbolTable.declare(
            param.name,
            Symbol{.type = resolvedParamType,
                   .node = statement,
                   .kind = (param.isMutable) ? SymbolKind::Parameter : SymbolKind::ConstantParameter,
                   .isMutable = param.isMutable});

        if (paramDeclaration == stmtvisit_t::Failure) [[unlikely]] {
            logError(statement, "Failed to declare parameter '{}' in scope for function '{}'", param.name,
                     statement->name);

            symbol->status = ResolutionStatus::Failure;
            signatureResult = stmtvisit_t::Failure;
        }
    }

    context.currentFunctionReturnType = resolvedReturnType;
    // already entered a scope for the parameters so we don't want to enter a scope while visiting the body
    constexpr bool bodyNeedsToEnterScope = false;
    const stmtvisit_t bodyResult = visit(statement->body, bodyNeedsToEnterScope);
    context.currentFunctionReturnType = nullptr;
    return (signatureResult == stmtvisit_t::Success && bodyResult == stmtvisit_t::Success) ? stmtvisit_t::Success : stmtvisit_t::Failure;
}

auto analyzer::visit(ast::IfStatement* statement) -> stmtvisit_t {
    auto result = stmtvisit_t::Success;
    const ContextGuard guard{context.ifStatementDepth,
                             static_cast<decltype(context.ifStatementDepth)>(context.ifStatementDepth + 1)};

    if (visit(statement->condition) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }

    if (!statement->condition->semanticType) {
        logError(statement, "Could not deduce type of condition {}", statement->condition->toString());
        result = stmtvisit_t::Failure;
    } else {
        const typeCompatibilityResult conditionCanBeBool = areTypesCompatible(
            statement->condition->semanticType, typeContext.getPrimitive(ast::PrimitiveType_t::boolean));

        if (!conditionCanBeBool) {
            logError(statement,
                     "Condition in if statement must be a boolean type or implicitly convertible to it, not {}",
                     statement->condition->semanticType->toString());
            result = stmtvisit_t::Failure;
        } else if (conditionCanBeBool.result == Compatible_t::Warning) {
            logWarning(statement, "{}", conditionCanBeBool.message);
        }
    }

    if (visit(statement->body) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }

    for (ast::ElifClause& elif : statement->elifs) {
        visit(elif.condition);

        if (!elif.condition->semanticType) {
            logError(statement, "Could not deduce type of condition {}", elif.condition->toString());
            result = stmtvisit_t::Failure;
        } else {
            const typeCompatibilityResult conditionCanBeBool = areTypesCompatible(
                elif.condition->semanticType, typeContext.getPrimitive(ast::PrimitiveType_t::boolean));

            if (!conditionCanBeBool) {
                logError(statement,
                         "Condition in elif statement must be a boolean type or implicitly convertible to it, not {}",
                         elif.condition->semanticType->toString());
                result = stmtvisit_t::Failure;
            } else if (conditionCanBeBool.result == Compatible_t::Warning) {
                logWarning(statement, "{}", conditionCanBeBool.message);
            }
        }

        if (visit(elif.body) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }
    }

    if (statement->elseBody.size() != 0) {
        // there is an else body
        if (visit(statement->elseBody) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }
    }
    return result;
}

auto analyzer::visit(ast::NestedBlockStatement* statement) -> stmtvisit_t { return visit(statement->block); }

auto analyzer::visit(ast::ReturnStatement* statement) -> stmtvisit_t {
    if (!context.inFunction) {
        logError(statement, "'return' can only be used in a function");
        return stmtvisit_t::Failure;
    }

    // void return
    if (!statement->value) {
        if (context.currentFunctionReturnType != nullptr) {
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

auto analyzer::visit([[maybe_unused]] ast::SwitchStatement* statement) -> stmtvisit_t { return stmtvisit_t::Success; }
auto analyzer::visit([[maybe_unused]] ast::VariableDeclarationStatement* statement) -> stmtvisit_t {
    return stmtvisit_t::Success;
}

auto analyzer::visit(ast::WhileLoopStatement* statement) -> stmtvisit_t {
    const ContextGuard guard{context.whileLoopDepth,
                             static_cast<decltype(context.whileLoopDepth)>(context.whileLoopDepth + 1)};

    auto result = stmtvisit_t::Success;

    if (visit(statement->condition) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }
    if (!statement->condition->semanticType) {
        logError(statement, "Could not deduce type of expression {}", statement->condition->toString());
        return stmtvisit_t::Failure;
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

    if (visit(statement->body) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }

    return result;
}

}  // namespace Manganese::semantic
