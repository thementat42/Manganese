#include <frontend/ast.hpp>
#include <frontend/semantic/semantic_analyzer.hpp>

namespace Manganese {
using ast::toStringOr;
namespace semantic {

void SemanticAnalyzer::visit(ast::AggregateDeclarationStatement* statement) {
    if (symbolTable.lookup(statement->name)) {
        logError("Aggregate '{}' was previously declared", statement, statement->name);
        return;
    }

    // Checking for duplicate fields already happened in the parser
    for (const auto& field : statement->fields) {
        if (ast::isPrimitiveType(field.type)) { continue; }
        if (!statement->genericTypes.empty()
            && std::find(statement->genericTypes.begin(), statement->genericTypes.end(), toStringOr(field.type))
                != statement->genericTypes.end()) {
            continue;  // This is a generic type, so it's valid
        }
        if (!typeExists(field.type)) {
            logError(
                "Field '{}' in Aggregate '{}' has type '{}' which was not declared (either as an aggregate or a type alias)",
                statement, field.name, statement->name, toStringOr(field.type));
            return;
        }
    }

    std::vector<ast::TypeSPtr_t> fieldTypes;
    fieldTypes.resize(statement->fields.size());
    for (size_t i = 0; i < statement->fields.size(); ++i) { fieldTypes[i] = statement->fields[i].type; }
    symbolTable.declare(Symbol{
        .name = statement->name,
        .kind = SymbolKind::Aggregate,
        .type = std::make_shared<ast::AggregateType>(fieldTypes),
        .line = statement->getLine(),
        .column = statement->getColumn(),
        .declarationNode = statement,
        // e.g. if Foo and Bar are aggregates, having Foo = Bar doesn't make sense since it'd invalidate existing Foo instances
        .isMutable = false,
        .scopeDepth = symbolTable.currentScopeDepth(),
        .visibility = statement->visibility,

    });
}

void SemanticAnalyzer::visit(ast::EnumDeclarationStatement* statement) {
    DISCARD(statement);
    NOT_IMPLEMENTED("Enums are complicated");
}

void SemanticAnalyzer::visit(ast::FunctionDeclarationStatement* statement) {
    // TODO: Ensure every path returns a value (some kind of block resolution return type assignment?)
    if (context.isFunctionContext()) {
        logError("Function declarations cannot be nested inside other functions", statement);
        return;
    }

    if (symbolTable.lookupInCurrentScope(statement->name)) {
        logError("Function '{}' was already declared in this scope. Function overloading is not currently supported",
                 statement, statement->name);
        return;
    }
    for (const auto& param : statement->parameters) {
        if (!param.type) {
            logError("Parameter '{}' in function '{}' must have a type", statement, param.name, statement->name);
            return;
        }
        if (!typeExists(param.type)) {
            logError(
                "Parameter '{}' in function '{}' has type '{}' which was not declared (either as an aggregate or a type alias)",
                statement, param.name, statement->name, toStringOr(param.type));
            return;
        }
    }
    // If return type is specified, check that it exists

    context.currentFunctionReturnType = statement->returnType;
    context.functionBody++;
    enterScope();  // Parameters are only valid within this scope

    for (const auto& param : statement->parameters) {
        symbolTable.declare(Symbol{
            .name = param.name,
            .kind = param.isMutable ? SymbolKind::FunctionParameter : SymbolKind::ConstantFunctionParameter,
            .type = param.type,
            .line = statement->getLine(),
            .column = statement->getColumn(),
            .declarationNode = statement,
            .isMutable = param.isMutable,
            .scopeDepth = symbolTable.currentScopeDepth(),
            .visibility = ast::Visibility::Private,  // Parameters are always private
        });
    }

    checkBlock(statement->body);
    context.functionBody--;
    exitScope();
    context.currentFunctionReturnType = nullptr;

    std::vector<ast::FunctionParameterType> parameterTypes;
    parameterTypes.reserve(statement->parameters.size());
    for (const auto& param : statement->parameters) { parameterTypes.emplace_back(param.isMutable, param.type); }

    symbolTable.declare(Symbol{
        .name = statement->name,
        .kind = SymbolKind::Function,
        .type = std::make_shared<ast::FunctionType>(parameterTypes, statement->returnType),
        .line = statement->getLine(),
        .column = statement->getColumn(),
        .declarationNode = statement,
        .isMutable = false,  // Functions are not constants
        .scopeDepth = symbolTable.currentScopeDepth(),
        .visibility = statement->visibility,
    });
}

void SemanticAnalyzer::visit(ast::VariableDeclarationStatement* statement) {
    bool isInvalidDeclaration = false;
    if (!statement->isMutable && !statement->value) {
        logError("Constant variable '{}' must be initialized", statement, statement->name);
        isInvalidDeclaration = true;
    }

    // Check for redefinitions
    if (symbolTable.lookupInCurrentScope(statement->name)) {
        logError(
            "Variable {} was already defined in this scope. If you meant to change its value, use {} = {} (without {})",
            statement, statement->name, statement->name,
            toStringOr(statement->value), statement->isMutable ? "let mut" : "let");
        isInvalidDeclaration = true;
    }

    // Check for shadowing
    const Symbol* outerSymbol = nullptr;
    for (int64_t depth = symbolTable.currentScopeDepth() - 1; depth >= 0; --depth) {
        outerSymbol = symbolTable.lookupAtDepth(statement->name, depth);
        if (outerSymbol) break;
    }

    if (outerSymbol) {
        logWarning("Variable '{}' shadows another variable with the same name from an outer scope", statement,
                   statement->name);
    }

    if (!statement->type && !statement->value) {
        logError("Variable '{}' must have a type or an initializer", statement, statement->name);
        isInvalidDeclaration = true;
    }
    if (isInvalidDeclaration) { return; }

    if (statement->type && statement->value) {
        visit(statement->value.get());
        if (!areTypesCompatible(statement->value->getType(), statement->type.get())) {
            logError("Type mismatch for variable '{}': expected {}, got {}", statement, statement->name,
                     toStringOr(statement->type), toStringOr(statement->value->getType()));
        }
    } else if (statement->value) {
        visit(statement->value.get());
        statement->type = statement->value->getTypePtr();
    }
    // If the variable only has a type and no initializer, there's no need to check the type

    symbolTable.declare(Symbol{
        .name = statement->name,
        .kind = statement->isMutable ? SymbolKind::Variable : SymbolKind::Constant,
        .type = statement->type,
        .line = statement->getLine(),
        .column = statement->getColumn(),
        .declarationNode = statement,
        .isMutable = statement->isMutable,
        .scopeDepth = symbolTable.currentScopeDepth(),
        .visibility = statement->visibility,
    });
}

}  // namespace semantic

}  // namespace Manganese