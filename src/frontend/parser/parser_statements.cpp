/**
 * @file parser_statements.cpp
 * @brief This file contains the implementation of statement parsing in the parser. It is split into its own file for readability and maintainability.
 */

#include <frontend/ast.h>
#include <frontend/lexer.h>
#include <frontend/parser.h>
#include <global_macros.h>

#include <format>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

namespace Manganese {
namespace parser {

StatementPtr_t Parser::parseStatement() {
    auto it = statementLookup.find(currentToken().getType());
    if (it != statementLookup.end()) {
        // If possible, parse a statement from the current token
        // Call the handler for the current token type
        return it->second(this);
    }

    // Parse out an expression then convert it to a statement
    ExpressionPtr_t expr = parseExpression(Precedence::Default);
    if (!isParsingBlockPrecursor) {
        expectToken(TokenType::Semicolon, "Expected semicolon after expression");
    }
    return std::make_unique<ast::ExpressionStatement>(std::move(expr));
}

// ===== Specific statement parsing methods =====

StatementPtr_t Parser::parseAliasStatement() {
    DISCARD(advance());
    TypePtr_t baseType;
    if (currentToken().isPrimitiveType() ||
        currentToken().getType() == TokenType::Func ||
        currentToken().getType() == TokenType::Ptr) {
        // Primitive types are easy to alias -- just parse a regular type
        baseType = parseType(Precedence::Default);
    } else {
        // If it's not a primtive type, we expect an identifier.
        // This might be a path (e.g. alias foo::bar as baz, so we need to handle that)
        std::string path = expectToken(TokenType::Identifier, "Expected an identifier after 'alias', or a primitive type.").getLexeme();
        while (currentToken().getType() == TokenType::ScopeResolution) {
            path += advance().getLexeme();
            path += expectToken(
                        TokenType::Identifier,
                        std::format("Expected an identifier after {}", lexer::tokenTypeToString(TokenType::ScopeResolution)))
                        .getLexeme();
        }

        if (currentToken().getType() == TokenType::At) {
            // Generic Type
            baseType = parseGenericType(std::make_unique<ast::SymbolType>(path), Precedence::Default);
        } else if (currentToken().getType() == TokenType::LeftSquare) {
            // Array type
            baseType = parseArrayType(std::make_unique<ast::SymbolType>(path), Precedence::Default);
        } else {
            // Regular (identifier) type
            baseType = std::make_unique<ast::SymbolType>(path);
        }
    }
    expectToken(TokenType::As, "Expected 'as' to introduce the type alias");
    std::string alias = expectToken(TokenType::Identifier, "Expected an alias name").getLexeme();
    expectToken(TokenType::Semicolon, "Expected a ';' after an alias statement");
    return std::make_unique<ast::AliasStatement>(std::move(baseType), std::move(alias));
}

StatementPtr_t Parser::parseBundleDeclarationStatement() {
    DISCARD(advance());
    std::vector<std::string> genericTypes;
    std::vector<ast::BundleField> fields;
    std::string name = expectToken(TokenType::Identifier, "Expected bundle name after 'bundle'").getLexeme();

    if (currentToken().getType() == TokenType::LeftSquare) {
        DISCARD(advance());
        while (!done() && currentToken().getType() != TokenType::RightSquare) {
            genericTypes.push_back(expectToken(TokenType::Identifier, "Expected a generic type name").getLexeme());
            if (currentToken().getType() != TokenType::RightSquare) {
                expectToken(TokenType::Comma, "Expected a ',' to separate generic types, or a ']' to close the generic type list");
            }
        }
        expectToken(TokenType::RightSquare, "Expected ']' to close generic type list");
    }
    expectToken(TokenType::LeftBrace, "Expected a '{'");

    while (!done()) {
        if (currentToken().getType() == TokenType::RightBrace) {
            break;  // Done declaration
        }
        if (currentToken().getType() != TokenType::Identifier) {
            logError(std::format("Unexpected token '{}' in bundle declaration. Expected field name.", currentToken().getLexeme()));
            DISCARD(advance());  // Skip the unexpected token to avoid infinite loop
        }
        std::string fieldName = advance().getLexeme();
        expectToken(TokenType::Colon, "Expected a ':' to declare a bundle field type.");
        TypePtr_t type = parseType(Precedence::Default);
        expectToken(TokenType::Semicolon, "Expected a ';'");

        auto duplicate = std::find_if(
            fields.begin(), fields.end(),
            [fieldName](const ast::BundleField& field) { return field.name == fieldName; });
        if (duplicate != fields.end()) {
            logError(std::format("Duplicate field '{}' in bundle '{}'", fieldName, name));
        } else {
            fields.emplace_back(fieldName, std::move(type));
        }
    }

    expectToken(TokenType::RightBrace);

    // Move since BundleField contains a unique_ptr which is not copyable
    return std::make_unique<ast::BundleDeclarationStatement>(name, std::move(genericTypes), std::move(fields));
}

StatementPtr_t Parser::parseDoWhileLoopStatement() {
    DISCARD(advance());
    ast::Block body = parseBlock("do-while body");
    expectToken(TokenType::While, "Expected 'while' after a 'do' block");
    expectToken(TokenType::LeftParen, "Expected '(' to introduce while condition");
    auto condition = parseExpression(Precedence::Default);
    expectToken(TokenType::RightParen, "Expected ')' to end a while condition");
    expectToken(TokenType::Semicolon, "Expected a ';' after a while clause");
    return std::make_unique<ast::WhileLoopStatement>(std::move(body), std::move(condition), true);
}

StatementPtr_t Parser::parseEnumDeclarationStatement() {
    DISCARD(advance());
    std::string name = expectToken(TokenType::Identifier, "Expected enum name after 'enum'").getLexeme();
    TypePtr_t baseType;
    std::vector<ast::EnumValue> values;
    if (currentToken().getType() == TokenType::Colon) {
        DISCARD(advance());
        if (!currentToken().isPrimitiveType()) {
            logError(std::format("Enums can only have primitive types as their underlying type, not {}", currentToken().getLexeme()));
        }
        baseType = std::make_unique<ast::SymbolType>(advance().getLexeme());
    } else {
        baseType = std::make_unique<ast::SymbolType>("int32");  // Default to int32 if no base type is specified
    }
    expectToken(TokenType::LeftBrace, "Expected '{' to start the enum body");
    while (!done() && currentToken().getType() != TokenType::RightBrace) {
        std::string valueName = expectToken(TokenType::Identifier, "Expected enum value name").getLexeme();
        ExpressionPtr_t valueExpression = nullptr;
        if (currentToken().getType() == TokenType::Assignment) {
            DISCARD(advance());
            valueExpression = parseExpression(Precedence::Default);
        }
        // Check for duplicate enum value names
        auto duplicate = std::find_if(
            values.begin(), values.end(),
            [valueName](const ast::EnumValue& value) { return value.name == valueName; });

        if (duplicate != values.end()) {
            logError(std::format("Enum value '{}' (in enum '{}') was previously declared", valueName, name));
        } else {
            values.emplace_back(valueName, std::move(valueExpression));
        }
        if (currentToken().getType() != TokenType::RightBrace) {
            expectToken(TokenType::Comma, "Expected ',' to separate enum values");
        }
    }
    expectToken(TokenType::RightBrace, "Expected '}' to end the enum body");
    if (values.empty()) {
        logError(std::format("Enum '{}' has no values", name));
    }
    return std::make_unique<ast::EnumDeclarationStatement>(std::move(name), std::move(baseType), std::move(values));
}

StatementPtr_t Parser::parseFunctionDeclarationStatement() {
    // TODO: Handle function visibility
    // TODO: Handle function attributes
    // TODO: Handle function default parameters
    // TODO: Handle function variadic parameters
    // TODO: Handle function overloading (mangled names)
    DISCARD(advance());
    std::string name = expectToken(TokenType::Identifier, "Expected function name").getLexeme();
    std::vector<ast::FunctionParameter> params;
    std::vector<std::string> genericTypes;
    TypePtr_t returnType = nullptr;
    ast::Block body;

    if (currentToken().getType() == TokenType::LeftSquare) {
        // Generics
        DISCARD(advance());
        while (!done()) {
            if (currentToken().getType() == TokenType::RightSquare) {
                break;  // End of generics
            }
            if (currentToken().getType() != TokenType::Identifier) {
                logError("Expected a generic type name");
                DISCARD(advance());  // Skip the unexpected token to avoid infinite loop
            }
            std::string genericName = expectToken(TokenType::Identifier, "Expected a generic type name").getLexeme();
            if (std::find(genericTypes.begin(), genericTypes.end(), genericName) != genericTypes.end()) {
                logError(std::format("Duplicate generic type '{}' in function '{}'", genericName, name));
            } else {
                genericTypes.push_back(std::move(genericName));
            }
            if (currentToken().getType() != TokenType::RightSquare) {
                expectToken(TokenType::Comma, "Expected a ',' to separate generic types, or a ']' to close the generic type list");
            }
        }
        expectToken(TokenType::RightSquare, "Expected ']' to close generic type list");
    }
    expectToken(TokenType::LeftParen);

    while (!done()) {
        if (currentToken().getType() == TokenType::RightParen) {
            break;
        }
        bool isConst = false;
        std::string param_name = expectToken(TokenType::Identifier, "Expected a variable name").getLexeme();
        expectToken(TokenType::Colon);
        if (currentToken().getType() == TokenType::Const) {
            DISCARD(advance());
            isConst = true;
        }
        TypePtr_t param_type = parseType(Precedence::Default);
        params.emplace_back(param_name, std::move(param_type), isConst);
        if (currentToken().getType() != TokenType::RightParen && currentToken().getType() != TokenType::EndOfFile) {
            expectToken(TokenType::Comma, "Expected a ',' to separate function parameters, or a ) to close the parameter list");
        }
    }
    expectToken(TokenType::RightParen);
    if (currentToken().getType() == TokenType::Arrow) {
        DISCARD(advance());
        returnType = parseType(Precedence::Default);
    }
    // Don't need to std::move body because of return value optimization
    return std::make_unique<ast::FunctionDeclarationStatement>(name, std::move(genericTypes), std::move(params), std::move(returnType), parseBlock("function body"));
}

StatementPtr_t Parser::parseIfStatement() {
    this->isParsingBlockPrecursor = true;
    DISCARD(advance());

    expectToken(TokenType::LeftParen, "Expected '(' to introduce if condition");
    auto condition = parseExpression(Precedence::Default);
    this->isParsingBlockPrecursor = false;
    expectToken(TokenType::RightParen, "Expected ')' to end if condition");
    ast::Block body = parseBlock("if body");

    std::vector<ast::ElifClause> elifs;
    while (currentToken().getType() == TokenType::Elif) {
        DISCARD(advance());

        this->isParsingBlockPrecursor = true;
        expectToken(TokenType::LeftParen, "Expected '(' to introduce elif condition");
        auto elifCondition = parseExpression(Precedence::Default);
        this->isParsingBlockPrecursor = false;
        expectToken(TokenType::RightParen, "Expected ')' to end elif condition");

        elifs.emplace_back(std::move(elifCondition), parseBlock("elif body"));
    }
    ast::Block elseBody;
    if (currentToken().getType() == TokenType::Else) {
        DISCARD(advance());
        elseBody = parseBlock("else body");
    }
    return std::make_unique<ast::IfStatement>(std::move(condition), std::move(body), std::move(elifs), std::move(elseBody));
}

StatementPtr_t Parser::parseImportStatement() {
    size_t startLine = currentToken().getLine();
    size_t startColumn = currentToken().getColumn();

    if (this->hasParsedFileHeader) {
        logging::logWarning("Imports should go at the top of the file", startLine, startColumn);
    }
    DISCARD(advance());
    std::vector<std::string> path;
    path.push_back(expectToken(TokenType::Identifier, "Expected a module name or path").getLexeme());
    while (currentToken().getType() == TokenType::ScopeResolution) {
        DISCARD(advance());  // Consume '::'
        path.push_back(expectToken(TokenType::Identifier, "Expected identifier after '::'").getLexeme());
    }
    std::string alias;
    if (currentToken().getType() == TokenType::As) {
        DISCARD(advance());
        alias = expectToken(TokenType::Identifier, "Expected an identifier as an import alias").getLexeme();
    }
    expectToken(TokenType::Semicolon, "Expected a ';' to end an import statement");

    bool duplicate = false;
    for (const auto& [existingPath, existingAlias] : imports) {
        if (path == existingPath) {
            std::string imported = std::accumulate(
                existingPath.begin() + 1,
                existingPath.end(),
                existingPath[0],  // existingPath should never be empty
                [](const std::string& a, const std::string& b) {
                    return a + "::" + b;
                });

            logging::logWarning(
                std::format("Duplicate import of {}", imported),
                startLine, startColumn);
            duplicate = true;
            break;

        } else if (alias == existingAlias && !alias.empty()) {
            logging::logWarning(
                std::format("Alias {} was already used", existingAlias),
                startLine, startColumn);
            duplicate = true;
            break;
        }
    }
    if (!duplicate) {
        imports.emplace_back(path, alias);
    }
    // Dummy node
    return std::make_unique<ast::ImportStatement>();
}

StatementPtr_t Parser::parseModuleDeclarationStatement() {
    auto temp = advance();
    size_t startLine = temp.getLine(), startColumn = temp.getColumn();
    if (this->hasParsedFileHeader) {
        logging::logWarning("Module declarations should go at the top of the file", startLine, startColumn);
    }
    std::string name = expectToken(TokenType::Identifier, "Expected a module name").getLexeme();
    expectToken(TokenType::Semicolon, "Expected a ';' after a module declaration");
    if (!this->moduleName.empty()) {
        logError("A module name has previously been declared in this file. Files can only have one module declaration.",
                 startLine, startColumn);
        return std::make_unique<ast::ModuleDeclarationStatement>();
    }
    this->moduleName = name;
    return std::make_unique<ast::ModuleDeclarationStatement>();
}

StatementPtr_t Parser::parseRepeatLoopStatement() {
    DISCARD(advance());
    expectToken(TokenType::LeftParen, "Expected '(' to introduce a number of iterations");
    auto numIterations = parseExpression(Precedence::Default);
    expectToken(TokenType::RightParen, "Expected ')' to end the number of iterations");
    // Don't need to std::move because of return value optimization
    return std::make_unique<ast::RepeatLoopStatement>(std::move(numIterations), parseBlock("repeat loop body"));
}

StatementPtr_t Parser::parseReturnStatement() {
    DISCARD(advance());
    ExpressionPtr_t expression = nullptr;
    if (currentToken().getType() != TokenType::Semicolon) {
        // If the next token is not a semicolon, parse an expression
        expression = parseExpression(Precedence::Default);
    }
    expectToken(TokenType::Semicolon, "Expected semicolon after return statement");
    return make_unique<ast::ReturnStatement>(std::move(expression));
}

StatementPtr_t Parser::parseSwitchStatement() {
    Token temp = advance();
    size_t startLine = temp.getLine(), startColumn = temp.getColumn();
    expectToken(TokenType::LeftParen, "Expected '(' to introduce switch variable");
    this->isParsingBlockPrecursor = true;
    ExpressionPtr_t variable = parseExpression(Precedence::Default);
    this->isParsingBlockPrecursor = false;
    expectToken(TokenType::RightParen, "Expected ')' to end switch variable");
    expectToken(TokenType::LeftBrace, "Expected '{' to start the switch body");

    std::vector<ast::CaseClause> cases;
    ast::Block defaultBody;

    while (currentToken().getType() == TokenType::Case) {
        DISCARD(advance());
        auto caseValue = parseExpression(Precedence::Default);
        ast::Block caseBody;
        expectToken(TokenType::Colon, "Expected ':' after case value");
        while (currentToken().getType() != TokenType::Case &&
               currentToken().getType() != TokenType::Default &&
               currentToken().getType() != TokenType::RightBrace) {
            caseBody.push_back(parseStatement());
        }
        cases.emplace_back(std::move(caseValue), std::move(caseBody));
    }
    if (currentToken().getType() == TokenType::Default) {
        DISCARD(advance());
        expectToken(TokenType::Colon, "Expected ':' after default case");
        while (currentToken().getType() != TokenType::RightBrace) {
            defaultBody.push_back(parseStatement());
        }
    }
    if (cases.empty() && defaultBody.empty()) {
        logging::logWarning("Switch statement has no cases or default body",
                            startLine, startColumn);
    }
    expectToken(TokenType::RightBrace, "Expected '}' to end the switch body");

    return std::make_unique<ast::SwitchStatement>(
        std::move(variable), std::move(cases), std::move(defaultBody));
}

StatementPtr_t Parser::parseVisibilityAffectedStatement() noexcept_if_release {
    ast::Visibility visibility;
    switch (advance().getType()) {
        case TokenType::Private:
            visibility = ast::Visibility::Private;
            break;
        case TokenType::Public:
            visibility = ast::Visibility::Public;
            break;
        case TokenType::ReadOnly:
            visibility = ast::Visibility::ReadOnly;
            break;
        default:
            ASSERT_UNREACHABLE("Unexpected token type in parseVisibilityAffectedStatement: " +
                               lexer::tokenTypeToString(currentToken().getType()));
    }
    size_t startLine = currentToken().getLine(), startColumn = currentToken().getColumn();
    switch (currentToken().getType()) {
        case TokenType::Bundle: {
            auto tempBundle = static_cast<ast::BundleDeclarationStatement*>(
                parseBundleDeclarationStatement().release());
            if (visibility == ast::Visibility::ReadOnly) {
                logging::logWarning("Bundles can only be public or private, not readonly",
                                    startLine, startColumn);
                visibility = ast::Visibility::Private;  // Default to private
            }
            tempBundle->visibility = visibility;
            return std::unique_ptr<ast::BundleDeclarationStatement>(tempBundle);
        }
        case TokenType::Enum: {
            auto tempEnum = static_cast<ast::EnumDeclarationStatement*>(
                parseEnumDeclarationStatement().release());
            if (visibility == ast::Visibility::ReadOnly) {
                logging::logWarning("Enums can only be public or private, not readonly",
                                    startLine, startColumn);
                visibility = ast::Visibility::Private;  // Default to private
            }
            tempEnum->visibility = visibility;
            return std::unique_ptr<ast::EnumDeclarationStatement>(tempEnum);
        }
        case TokenType::Func: {
            auto tempFunction = static_cast<ast::FunctionDeclarationStatement*>(
                parseFunctionDeclarationStatement().release());
            tempFunction->visibility = visibility;
            return std::unique_ptr<ast::FunctionDeclarationStatement>(tempFunction);
        }
        default:
            logError(std::format("{} cannot follow a visibility modifier",
                                 lexer::tokenTypeToString(currentToken().getType())),
                     startLine, startColumn);
            // Parse the statement as if it had no visibility modifier
            return parseStatement();
    }
}

StatementPtr_t Parser::parseVariableDeclarationStatement() {
    TypePtr_t explicitType;
    ExpressionPtr_t value;
    ast::Visibility visibility = defaultVisibility;

    bool isConst = advance().getType() == TokenType::Const;
    std::string name = expectToken(TokenType::Identifier,
                                   std::format("Expected variable name after '{}'", isConst ? "const" : "let"))
                           .getLexeme();
    if (currentToken().getType() == TokenType::Colon) {
        DISCARD(advance());  // Consume the colon
        if (currentToken().getType() == TokenType::Public) {
            visibility = ast::Visibility::Public;
            DISCARD(advance());  // Consume the public keyword
        } else if (currentToken().getType() == TokenType::ReadOnly) {
            visibility = ast::Visibility::ReadOnly;
            DISCARD(advance());  // Consume the read-only keyword
        } else if (currentToken().getType() == TokenType::Private) [[unlikely]] {
            // private is the default so it'd mainly be used for emphasis
            visibility = ast::Visibility::Private;
            DISCARD(advance());  // Consume the private keyword
        }
        explicitType = parseType(Precedence::Default);
    }
    if (currentToken().getType() != TokenType::Semicolon) {
        expectToken(TokenType::Assignment, "Expected '=' or ';' after variable name");
        value = parseExpression(Precedence::Default);
    } else if (explicitType == nullptr) {
        // If no value is provided, we need to have a type
        expectToken(TokenType::Colon, "Expected ':' to specify type for variable without initial value");
        explicitType = parseType(Precedence::Default);
    }

    expectToken(TokenType::Semicolon, "Expected semicolon after variable declaration");

    return std::make_unique<ast::VariableDeclarationStatement>(
        isConst,
        std::move(name),
        visibility,
        std::move(value),
        std::move(explicitType));
}

StatementPtr_t Parser::parseWhileLoopStatement() {
    DISCARD(advance());
    expectToken(TokenType::LeftParen, "Expected '(' to introduce while condition");
    auto condition = parseExpression(Precedence::Default);
    expectToken(TokenType::RightParen, "Expected ')' to end while condition");
    // Don't need to std::move because of return value optimization
    return std::make_unique<ast::WhileLoopStatement>(parseBlock("while loop body"), std::move(condition));
}

// ===== Helper Functions =====
ast::Block Parser::parseBlock(std::string blockName) {
    expectToken(TokenType::LeftBrace, "Expected a '{' to start " + blockName);
    ast::Block block;
    while (!done()) {
        if (currentToken().getType() == TokenType::RightBrace) {
            break;  // End of the block
        }
        block.push_back(parseStatement());
    }
    expectToken(TokenType::RightBrace, "Expected '}' to end " + blockName);
    if (block.empty()) {
        logging::logWarning(
            std::format("{} is empty", blockName),
            currentToken().getLine(), currentToken().getColumn());
    }
    return block;
}
}  // namespace parser
}  // namespace Manganese
