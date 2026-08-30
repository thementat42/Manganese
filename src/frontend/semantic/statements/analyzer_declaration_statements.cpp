#include <core.hpp>
#include <cstdint>
#include <format>
#include <frontend/ast.hpp>
#include <frontend/semantic.hpp>
#include <io/logging.hpp>
#include <utils/result.hpp>

namespace Manganese::semantic {

auto Analyzer::visit(ast::AggregateDeclarationStatement* statement) -> stmtvisit_t {
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

auto Analyzer::visit(ast::AliasStatement* statement) -> stmtvisit_t {
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

auto Analyzer::visit(ast::EnumDeclarationStatement* statement) -> stmtvisit_t {
    stmtvisit_t result = stmtvisit_t::Success;
    Symbol* symbol = symbolTable.lookup(statement->name);
    if (!symbol) {
        ASSERT_UNREACHABLE(std::format("Enum {} was not registered during type initalization", statement->name));
    }

    if (symbol->status == ResolutionStatus::Success) { return stmtvisit_t::Success; }
    if (symbol->status == ResolutionStatus::Failure) { return stmtvisit_t::Failure; }

    symbol->status = ResolutionStatus::InProgress;

    // Default to an int32 if no type is given (or if there's an error)
    const SemanticType* underlyingType = typeContext.getPrimitive(ast::PrimitiveType_t::i32);
    if (statement->baseType) {
        if (visit(statement->baseType) == stmtvisit_t::Failure) {
            symbol->status = ResolutionStatus::Failure;
            result = stmtvisit_t::Failure;
        } else if (!statement->baseType->semanticType->isInteger()) {
            logError(statement->baseType, "Base type of enum '{}' must be an integer type, not '{}'", statement->name,
                     statement->baseType->semanticType->toString());
            result = stmtvisit_t::Failure;
        } else {
            underlyingType = statement->baseType->semanticType;
        }
    }
    const Enum* enumType = static_cast<const Enum*>(typeContext.getEnum(statement->name));
    enumType->underlyingType = underlyingType;
    symbol->type = enumType;

    std::int64_t currentVariantValue = 0;

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
            currentVariantValue = explicitVal.number_unchecked().value_as<std::int64_t>();
        }

        variants.emplace_back(variant.name, currentVariantValue);
        currentVariantValue++;
    }
    enumType->variants = std::move(variants);
    return result;
}

auto Analyzer::visit(ast::FunctionDeclarationStatement* statement) -> stmtvisit_t {
    if (context.inFunction) {
        logError(statement,
                 "Nested functions are not supported: function '{}' cannot be declared inside another function",
                 statement->name);
        return stmtvisit_t::Failure;
    }

    if (!statement->genericTypes.empty()) { return stmtvisit_t::Success; }

    Symbol* symbol = symbolTable.lookup(statement->name);
    if (!symbol) {
        ASSERT_UNREACHABLE(std::format("Function '{}' was not registered during symbol collection", statement->name));
    }
    if (!symbol->type) {
        ASSERT_UNREACHABLE(
            std::format("Function '{}' was registered during symbol collection but had no type", statement->name));
    }

    if (symbol->status == ResolutionStatus::Success) { return stmtvisit_t::Success; }
    if (symbol->status == ResolutionStatus::Failure) { return stmtvisit_t::Failure; }

    // Re-entrancy check: allow recursive calls inside the body
    if (symbol->status == ResolutionStatus::InProgress) { return stmtvisit_t::Success; }

    symbol->status = ResolutionStatus::InProgress;

    const ContextGuard contextGuard{context.inFunction, true};
    const Function* functionType = static_cast<const Function*>(symbol->type);

    stmtvisit_t signatureResult = stmtvisit_t::Success;

    symbolTable.enterScope();
    for (std::size_t i = 0; i < statement->parameters.size(); ++i) {
        const auto& param = statement->parameters[i];
        const SemanticType* resolvedParamType = functionType->parameterTypes[i].type;

        // Register parameter in local function scope
        stmtvisit_t paramDeclaration = symbolTable.declare(
            param.name,
            Symbol{.type = resolvedParamType,
                   .node = statement,
                   .kind = (param.isMutable) ? SymbolKind::Parameter : SymbolKind::ConstantParameter,
                   .isMutable = param.isMutable});

        if (paramDeclaration == stmtvisit_t::Failure) [[unlikely]] {
            logError(statement, "Failed to declare parameter '{}' in scope for function '{}'", param.name,
                     statement->name);
            signatureResult = stmtvisit_t::Failure;
        }

        // Validate default arguments in local scope
        if (param.defaultValue) {
            visit(param.defaultValue);

            const SemanticType* defaultType = param.defaultValue->semanticType;

            if (!defaultType) {
                logError(statement, "Unable to determine type of default value for parameter '{}' in function '{}'",
                         param.name, statement->name);
                signatureResult = stmtvisit_t::Failure;
            } else if (resolvedParamType && !areTypesCompatible(resolvedParamType, defaultType)) {
                logError(statement,
                         "Default value for parameter '{}' in function '{}' has type '{}', "
                         "but '{}' was expected",
                         param.name, statement->name, defaultType->toString(), resolvedParamType->toString());
                signatureResult = stmtvisit_t::Failure;
            }
        }
    }

    // Type-check the body
    context.currentFunctionReturnType = functionType->returnType;
    const stmtvisit_t bodyResult = visit(statement->body, false);
    context.currentFunctionReturnType = nullptr;

    const bool isSuccess = (signatureResult == stmtvisit_t::Success && bodyResult == stmtvisit_t::Success);
    symbol->status = isSuccess ? ResolutionStatus::Success : ResolutionStatus::Failure;

    return isSuccess ? stmtvisit_t::Success : stmtvisit_t::Failure;
}

auto Analyzer::visit(ast::VariableDeclarationStatement* statement) -> stmtvisit_t {
    const SemanticType* variableType = nullptr;

    if (statement->type) {
        // User has an explicit type
        if (visit(statement->type) == stmtvisit_t::Failure) { return stmtvisit_t::Failure; }
        variableType = statement->type->semanticType;
    }

    if (statement->value) {
        if (visit(statement->value) == exprvisit_t::Failure) { return stmtvisit_t::Failure; }
        const SemanticType* initializerType = statement->value->semanticType;

        // Catch missing or void initializer types immediately
        if (!initializerType || initializerType->isVoid()) {
            logError(statement->value, "Cannot initialize variable '{}' with a void expression", statement->name);
            return stmtvisit_t::Failure;
        }

        if (!variableType) {
            // Deduce type from initializer
            variableType = initializerType;
        } else {
            // Check explicit type against initializer type
            const typeCompatibilityResult compat = areTypesCompatible(initializerType, variableType);
            if (!compat) {
                logError(statement, "Cannot assign value of type {} to variable '{}' of type {}",
                         initializerType->toString(), statement->name, variableType->toString());
                return stmtvisit_t::Failure;
            } else if (compat.result == Compatible_t::Warning) {
                logWarning(statement, "{}", compat.message);
            }
        }
    } else if (!statement->isMutable) {
        logError(statement, "Immutable variable '{}' must have an initializer", statement->name);
        return stmtvisit_t::Failure;
    }

    if (!variableType) {
        logError(statement, "Variable '{}' must either have an explicit type or an initial value", statement->name);
        return stmtvisit_t::Failure;
    }

    const Result declarationResult
        = symbolTable.declare(statement->name,
                              Symbol{.type = variableType,
                                     .node = statement,
                                     .kind = statement->isMutable ? SymbolKind::Variable : SymbolKind::Constant,
                                     .isMutable = statement->isMutable,
                                     .status = ResolutionStatus::Success});

    if (declarationResult == Result::Failure) {
        logError(statement, "Redeclaration error: variable '{}' is already declared in this scope", statement->name);
        return stmtvisit_t::Failure;
    }

    return stmtvisit_t::Success;
}

}  // namespace Manganese::semantic