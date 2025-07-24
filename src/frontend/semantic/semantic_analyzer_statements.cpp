/**
 * @file semantic_analyzer_statements.cpp
 * @brief Responsible for performing semantic analysis on all the AST statement nodes
 */

#include <frontend/ast.h>
#include <frontend/semantic/semantic_analyzer.h>
#include <global_macros.h>

#include <format>
#include <stdexcept>

namespace Manganese {
namespace semantic {

void SemanticAnalyzer::checkStatement(ast::Statement* statement) noexcept_if_release {
    switch (statement->kind()) {
        case ast::StatementKind::AliasStatement:
            checkAliasStatement(static_cast<ast::AliasStatement*>(statement));
            break;
        case ast::StatementKind::BreakStatement:
            checkBreakStatement(static_cast<ast::BreakStatement*>(statement));
            break;
        case ast::StatementKind::BundleDeclarationStatement:
            checkBundleDeclarationStatement(static_cast<ast::BundleDeclarationStatement*>(statement));
            break;
        case ast::StatementKind::ContinueStatement:
            checkContinueStatement(static_cast<ast::ContinueStatement*>(statement));
            break;
        case ast::StatementKind::EnumDeclarationStatement:
            checkEnumDeclarationStatement(static_cast<ast::EnumDeclarationStatement*>(statement));
            break;
        case ast::StatementKind::ExpressionStatement:
            checkExpressionStatement(static_cast<ast::ExpressionStatement*>(statement));
            break;
        case ast::StatementKind::FunctionDeclarationStatement:
            checkFunctionDeclarationStatement(static_cast<ast::FunctionDeclarationStatement*>(statement));
            break;
        case ast::StatementKind::IfStatement:
            checkIfStatement(static_cast<ast::IfStatement*>(statement));
            break;
        case ast::StatementKind::ImportStatement:
            checkImportStatement(static_cast<ast::ImportStatement*>(statement));
            break;
        case ast::StatementKind::ModuleDeclarationStatement:
            checkModuleDeclarationStatement(static_cast<ast::ModuleDeclarationStatement*>(statement));
            break;
        case ast::StatementKind::RepeatLoopStatement:
            checkRepeatLoopStatement(static_cast<ast::RepeatLoopStatement*>(statement));
            break;
        case ast::StatementKind::ReturnStatement:
            checkReturnStatement(static_cast<ast::ReturnStatement*>(statement));
            break;
        case ast::StatementKind::SwitchStatement:
            checkSwitchStatement(static_cast<ast::SwitchStatement*>(statement));
            break;
        case ast::StatementKind::VariableDeclarationStatement:
            checkVariableDeclarationStatement(static_cast<ast::VariableDeclarationStatement*>(statement));
            break;
        case ast::StatementKind::WhileLoopStatement:
            checkWhileLoopStatement(static_cast<ast::WhileLoopStatement*>(statement));
            break;
        default:
            using std::format;
            ASSERT_UNREACHABLE(
                format("No semantic analysis method for statement type {}",
                       static_cast<int>(statement->kind())));
            break;
    }
}

// ===== Specific Statement Checks =====
void SemanticAnalyzer::checkAliasStatement(ast::AliasStatement* statement) {
    bool isInvalidAlias = false;
    if (!typeExists(statement->baseType)) {
        logError("Base type '{}' for alias '{}' does not exist", statement, statement->baseType->toString(), statement->alias);
        isInvalidAlias = true;
    }
    if (symbolTable.lookupInCurrentScope(statement->alias)) {
        logError("Alias '{}' already exists in the current scope", statement, statement->alias);
        isInvalidAlias = true;
    }
    if (isInvalidAlias) {
        return;
    };
    symbolTable.declare(
        Symbol{
            .name = statement->alias,
            .kind = SymbolKind::TypeAlias,
            .type = statement->baseType,
            .line = statement->getLine(),
            .column = statement->getColumn(),
            .declarationNode = statement,
            .isConstant = false,  // Type aliases are not constants
            .scopeDepth = symbolTable.currentScopeDepth(),
            .visibility = statement->visibility});
}
void SemanticAnalyzer::checkBreakStatement(ast::BreakStatement* statement) {
    if (!context.isLoopContext() && !context.isSwitchContext()) {
        logError("break statements can only be used inside loops or switch statements", statement);
    }
}
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
void SemanticAnalyzer::checkContinueStatement(ast::ContinueStatement* statement) {
    if (!context.isLoopContext()) {
        logError("continue statements can only be used inside loops", statement);
    }
}
void SemanticAnalyzer::checkEnumDeclarationStatement(ast::EnumDeclarationStatement* statement) {
    DISCARD(statement);
    PRINT_LOCATION;
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkExpressionStatement(ast::ExpressionStatement* statement) {
    checkExpression(statement->expression.get());
}
void SemanticAnalyzer::checkFunctionDeclarationStatement(ast::FunctionDeclarationStatement* statement) {
    DISCARD(statement);
    PRINT_LOCATION;
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkIfStatement(ast::IfStatement* statement) {
    DISCARD(statement);
    PRINT_LOCATION;
    throw std::runtime_error("Not implemented");
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
void SemanticAnalyzer::checkRepeatLoopStatement(ast::RepeatLoopStatement* statement) {
    DISCARD(statement);
    PRINT_LOCATION;
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkReturnStatement(ast::ReturnStatement* statement) {
    if (!context.isFunctionContext()) {
        logError("return statements can only be used inside functions", statement);
    }
    checkExpression(statement->value.get());
}
void SemanticAnalyzer::checkSwitchStatement(ast::SwitchStatement* statement) {
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
void SemanticAnalyzer::checkWhileLoopStatement(ast::WhileLoopStatement* statement) {
    DISCARD(statement);
    PRINT_LOCATION;
    throw std::runtime_error("Not implemented");
}

}  // namespace semantic
}  // namespace Manganese
