#include <core.hpp>
#include <cstddef>
#include <cstdint>
#include <format>
#include <frontend/ast.hpp>
#include <frontend/semantic.hpp>
#include <io/logging.hpp>
#include <utility>
#include <utils/expression_folding.hpp>
#include <utils/result.hpp>
#include <vector>

namespace Manganese::semantic {

auto SemanticAnalyzer::visit(ast::AggregateDeclarationStatement* statement) -> stmtvisit_t {
    // We don't know the generic types at declaration so we can't check them
    // Instead, check only when they're instantiated
    if (!statement->genericTypes.empty()) { return stmtvisit_t::Success; }

    const Symbol* symbol = symbolTable.lookup(statement->name);
    if (symbol == nullptr) {
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
        DISCARD(visit(field.type));
        const SemanticType* resolvedFieldType = field.type->semanticType;
        if (resolvedFieldType->isPoison()) {
            logging::logError(field.line, field.column, "Unknown type for field '{}' in aggregate '{}'", field.name,
                              statement->name);
            return stmtvisit_t::Failure;
        }

        if (resolvedFieldType->isAggregate()) {
            const auto* nestedAggregateType = static_cast<const Aggregate*>(resolvedFieldType);
            const Symbol* nestedSymbol = symbolTable.lookup(nestedAggregateType->name);
            if (nestedSymbol != nullptr && nestedSymbol->node != nullptr) {
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

auto SemanticAnalyzer::visit(ast::AliasStatement* statement) -> stmtvisit_t {
    Symbol* symbol = symbolTable.lookup(statement->alias);
    if (symbol == nullptr) {
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

auto SemanticAnalyzer::visit(ast::EnumDeclarationStatement* statement) -> stmtvisit_t {
    stmtvisit_t result = stmtvisit_t::Success;
    Symbol* symbol = symbolTable.lookup(statement->name);
    if (symbol == nullptr) {
        ASSERT_UNREACHABLE(std::format("Enum {} was not registered during type initalization", statement->name));
    }

    if (symbol->status == ResolutionStatus::Success) { return stmtvisit_t::Success; }
    if (symbol->status == ResolutionStatus::Failure) { return stmtvisit_t::Failure; }

    symbol->status = ResolutionStatus::InProgress;

    // Default to an int32 if no type is given (or if there's an error)
    const SemanticType* underlyingType = typeContext.getPrimitive(ast::PrimitiveType::int32);
    if (statement->baseType != nullptr) {
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
        if (variant.value != nullptr) {
            if (visit(variant.value) == stmtvisit_t::Failure) { result = stmtvisit_t::Failure; }
            if (!variant.value->canFold()) {
                logging::logError(variant.line, variant.column,
                                  "Variant {} (in enum {}) must have a compile-time value", variant.name,
                                  statement->name);
                symbol->status = ResolutionStatus::Failure;
                result = stmtvisit_t::Failure;
            }
            if (!variant.value->semanticType->isInteger()) {
                logging::logError(variant.line, variant.column, "Variant {} (in enum {}) must have an integer value",
                                  variant.name, statement->name);
                symbol->status = ResolutionStatus::Failure;
                result = stmtvisit_t::Failure;
            }
            auto tmp = utils::computeExpression<std::int64_t>(
                variant.value, typeContext.getTargetInfo(),
                                    [this]<class... Args>(const auto* expr, std::format_string<Args...> fmt, Args&&... args) {
                        this->logError(expr, fmt, std::forward<Args>(args)...);
                    });
            if (!tmp.has_value()) {
                logError(variant.value, "Invalid value '{}' for variant '{}' in enum '{}'", variant.value->toString(),
                         variant.name, statement->name);

            } else {
                currentVariantValue = *tmp;
            }
        }

        variants.emplace_back(variant.name, currentVariantValue);
        currentVariantValue++;
    }
    enumType->variants = std::move(variants);
    return result;
}

auto SemanticAnalyzer::visit(ast::FunctionDeclarationStatement* statement) -> stmtvisit_t {
    if (context.inFunction) {
        logError(statement,
                 "Nested functions are not supported: function '{}' cannot be declared inside another function",
                 statement->name);
        return stmtvisit_t::Failure;
    }

    if (!statement->genericTypes.empty()) { return stmtvisit_t::Success; }

    Symbol* symbol = symbolTable.lookup(statement->name);
    if (symbol == nullptr || symbol->type == nullptr) {
        ASSERT_UNREACHABLE(
            std::format("Function '{}' was not properly registered during symbol collection", statement->name));
    }

    if (symbol->status == ResolutionStatus::Success) { return stmtvisit_t::Success; }
    if (symbol->status == ResolutionStatus::Failure) { return stmtvisit_t::Failure; }
    if (symbol->status == ResolutionStatus::InProgress) { return stmtvisit_t::Success; }

    symbol->status = ResolutionStatus::InProgress;

    const ContextGuard contextGuard{context.inFunction, true};
    const Function* functionType = static_cast<const Function*>(symbol->type);

    stmtvisit_t signatureResult = stmtvisit_t::Success;

    symbolTable.enterScope();
    for (std::size_t i = 0; i < statement->parameters.size(); ++i) {
        const auto& param = statement->parameters[i];
        const SemanticType* resolvedParamType = functionType->parameterTypes[i].type;

        const stmtvisit_t paramDeclaration = symbolTable.declare(
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

        if (param.defaultValue != nullptr) {
            if (visit(param.defaultValue) == exprvisit_t::Failure) {
                signatureResult = stmtvisit_t::Failure;
                continue;
            }

            const SemanticType* defaultValueType = param.defaultValue->semanticType;
            if (defaultValueType == nullptr || defaultValueType->isPoison()) {
                logError(statement, "Unable to determine type of default value for parameter '{}' in function '{}'",
                         param.name, statement->name);
                signatureResult = stmtvisit_t::Failure;
            } else if (!areTypesCompatible(defaultValueType, resolvedParamType)) {
                logError(statement,
                         "Default value for parameter '{}' in function '{}' has type '{}', "
                         "but '{}' was expected",
                         param.name, statement->name, defaultValueType->toString(), resolvedParamType->toString());
                signatureResult = stmtvisit_t::Failure;
            }
        }
    }

    context.currentFunctionReturnType = functionType->returnType;
    const stmtvisit_t bodyResult = visit(statement->body, false);
    context.currentFunctionReturnType = nullptr;

    const bool isSuccess = (signatureResult == stmtvisit_t::Success && bodyResult == stmtvisit_t::Success);
    symbol->status = isSuccess ? ResolutionStatus::Success : ResolutionStatus::Failure;

    return isSuccess ? stmtvisit_t::Success : stmtvisit_t::Failure;
}

auto SemanticAnalyzer::visit(ast::VariableDeclarationStatement* statement) -> stmtvisit_t {
    const SemanticType* variableType = nullptr;

    if (statement->type != nullptr) {
        if (visit(statement->type) == stmtvisit_t::Failure) { return stmtvisit_t::Failure; }
        variableType = statement->type->semanticType;
        if (variableType == typeContext.getVoid()) {
            logError(statement, "Variable '{}' cannot be declared with type void", statement->name);
            return stmtvisit_t::Failure;
        }
    }

    if (statement->value != nullptr) {
        context.currentVariableDeclarationType = variableType;
        if (visit(statement->value) == exprvisit_t::Failure) { return stmtvisit_t::Failure; }
        const SemanticType* initializerType = statement->value->semanticType;

        if (initializerType->isPoison()) {
            logError(statement->value, "Could not determine type of initializer '{}' for variable '{}'",
                     statement->value->toString(), statement->name);
            return stmtvisit_t::Failure;
        }
        if (initializerType->isVoid()) {
            logError(statement->value, "Cannot initialize variable '{}' with a void expression", statement->name);
            return stmtvisit_t::Failure;
        }

        if (variableType == nullptr) {
            variableType = initializerType;
        } else {
            // If the declared type used `[]`, unify/fill its length using the initializer's dimensions
            variableType = unifyArrayInference(variableType, initializerType);
            if (variableType == nullptr) {
                logError(statement, "Initializer type '{}' is incompatible with declared type",
                         initializerType->toString());
                return stmtvisit_t::Failure;
            }
        }
    } else if (!statement->isMutable) {
        logError(statement, "Immutable variable '{}' must have an initializer", statement->name);
        return stmtvisit_t::Failure;
    }

    if (variableType == nullptr
        || (variableType->isArray() && static_cast<const Array*>(variableType)->hasUnspecifiedLength())) {
        logError(statement, "Cannot infer array length; explicitly specify length or provide an initializer");
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