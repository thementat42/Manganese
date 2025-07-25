#include <frontend/semantic/semantic_analyzer.h>

namespace Manganese {

namespace semantic {

void SemanticAnalyzer::checkBundleDeclarationStatement(ast::BundleDeclarationStatement* statement) {
    if (symbolTable.lookup(statement->name)) {
        logError("Bundle '{}' was previously declared", statement, statement->name);
        return;
    }

    // Checking for duplicate fields already happened in the parser
    for (const auto& field : statement->fields) {
        if (ast::isPrimitiveType(field.type)) {
            continue;
        }
        if (!statement->genericTypes.empty() &&
            std::find(statement->genericTypes.begin(), statement->genericTypes.end(), field.type->toString()) != statement->genericTypes.end()) {
            continue;  // This is a generic type, so it's valid
        }
        if (!typeExists(field.type)) {
            logError("Field '{}' in bundle '{}' has type '{}' which was not declared (either as a bundle or a type alias)",
                     statement, field.name, statement->name, field.type->toString());
            return;
        }
    }

    std::vector<ast::TypeSPtr_t> fieldTypes;
    fieldTypes.resize(statement->fields.size());
    for (size_t i = 0; i < statement->fields.size(); ++i) {
        fieldTypes[i] = statement->fields[i].type;
    }
    symbolTable.declare(
        Symbol{
            .name = statement->name,
            .kind = SymbolKind::Bundle,
            .type = std::make_shared<ast::BundleType>(fieldTypes),
            .line = statement->getLine(),
            .column = statement->getColumn(),
            .declarationNode = statement,
            .isConstant = false,  // Bundles are not constants
            .scopeDepth = symbolTable.currentScopeDepth(),
            .visibility = statement->visibility,

        });
}

void SemanticAnalyzer::checkEnumDeclarationStatement(ast::EnumDeclarationStatement* statement) {
    DISCARD(statement);
    PRINT_LOCATION;
    throw std::runtime_error("Not implemented");
}

void SemanticAnalyzer::checkFunctionDeclarationStatement(ast::FunctionDeclarationStatement* statement) {
    if (context.isFunctionContext()) {
        logError("Function declarations cannot be nested inside other functions", statement);
        return;
    }

    if (symbolTable.lookupInCurrentScope(statement->name)) {
        logError("Function '{}' was already declared in this scope. Function overloading is not currently supported", statement, statement->name);
        return;
    }
    for (const auto& param : statement->parameters) {
        if (!param.type) {
            logError("Parameter '{}' in function '{}' must have a type", statement, param.name, statement->name);
            return;
        }
        if (!typeExists(param.type)) {
            logError("Parameter '{}' in function '{}' has type '{}' which was not declared (either as a bundle or a type alias)",
                     statement, param.name, statement->name, param.type->toString());
            return;
        }
    }
    // If return type is specified, check that it exists

    ast::TypeSPtr_t returnType = statement->returnType;
    this->currentFunctionReturnType = returnType;
    context.functionBody++;
    enterScope();

    for (const auto& param : statement->parameters) {
        symbolTable.declare(
            Symbol{
                .name = param.name,
                .kind = param.isConst ? SymbolKind::Constant : SymbolKind::Variable,
                .type = param.type,
                .line = statement->getLine(),
                .column = statement->getColumn(),
                .declarationNode = statement,
                .isConstant = param.isConst,
                .scopeDepth = symbolTable.currentScopeDepth(),
                .visibility = ast::Visibility::Private,  // Parameters are always private
            });
    }

    checkBlock(statement->body);
    context.functionBody--;
    exitScope();
    this->currentFunctionReturnType = nullptr;

    std::vector<ast::FunctionParameterType> parameterTypes;
    parameterTypes.reserve(statement->parameters.size());
    for (const auto& param : statement->parameters) {
        parameterTypes.emplace_back(param.isConst, param.type);
    }

    symbolTable.declare(
        Symbol{
            .name = statement->name,
            .kind = SymbolKind::Function,
            .type = std::make_shared<ast::FunctionType>(parameterTypes, returnType),
            .line = statement->getLine(),
            .column = statement->getColumn(),
            .declarationNode = statement,
            .isConstant = false,  // Functions are not constants
            .scopeDepth = symbolTable.currentScopeDepth(),
            .visibility = statement->visibility,
        });
}

void SemanticAnalyzer::checkImportStatement(ast::ImportStatement* statement) {
    DISCARD(statement);
    PRINT_LOCATION;
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkModuleDeclarationStatement(ast::ModuleDeclarationStatement* statement) {
    DISCARD(statement);
    PRINT_LOCATION;
    throw std::runtime_error("Not implemented");
}

void SemanticAnalyzer::checkVariableDeclarationStatement(ast::VariableDeclarationStatement* statement) {
    bool isInvalidDeclaration = false;
    if (statement->isConstant() && !statement->value) {
        logError("Constant variable '{}' must be initialized", statement, statement->name);
        isInvalidDeclaration = true;
    }
    if (symbolTable.lookupInCurrentScope(statement->name)) {
        logError("Variable {} was already defined in this scope. If you meant to change its value, use {} = {} (without {})",
                 statement, statement->name, statement->name,
                 statement->isConstant() ? "const" : "let",
                 statement->value->toString());
        isInvalidDeclaration = true;
    }
    if (!statement->type && !statement->value) {
        logError("Variable '{}' must have a type or an initializer", statement, statement->name);
        isInvalidDeclaration = true;
    }
    if (isInvalidDeclaration) {
        return;
    }

    if (statement->type && statement->value) {
        checkExpression(statement->value.get());
        if (!areTypesCompatible(statement->value->getType(), statement->type.get())) {
            logError("Type mismatch for variable '{}': expected {}, got {}", statement,
                     statement->name, statement->type->toString(), statement->value->getType()->toString());
        }
    } else if (statement->value) {
        checkExpression(statement->value.get());
        statement->type = statement->value->getTypePtr();
    }
    // If the variable only has a type and no initializer, there's no need to check the type

    symbolTable.declare(
        Symbol{
            .name = statement->name,
            .kind = statement->isConstant() ? SymbolKind::Constant : SymbolKind::Variable,
            .type = statement->type,
            .line = statement->getLine(),
            .column = statement->getColumn(),
            .declarationNode = statement,
            .isConstant = statement->isConstant(),
            .scopeDepth = symbolTable.currentScopeDepth(),
            .visibility = statement->visibility,
        });
}

}  // namespace semantic

}  // namespace Manganese