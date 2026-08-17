#include <algorithm>
#include <core.hpp>
#include <cstddef>
#include <format>
#include <frontend/ast.hpp>
#include <frontend/lexer.hpp>
#include <frontend/parser.hpp>
#include <io/logging.hpp>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

namespace Manganese::parser {

ast::Statement* Parser::parseStatement() {
    const TokenType type = peekTokenType();

    if (type == TokenType::LeftBrace) {
        // don't need to move thanks to copy elision
        return arena.emplace<ast::NestedBlockStatement>(parseBlock("nested block"));
    }

    // Handle bare semicolons
    if (type == TokenType::Semicolon) {
        DISCARD(consumeToken());
        return ast::getEmptyStatement();
    }
    const std::size_t index = tokenToIndex(type);

    const statementHandler_t handler = statementLookup[index];
    if (handler) { return (this->*handler)(); }

    // Parse out an expression then convert it to a statement
    ast::Expression* expr = parseExpression(Precedence::Default);
    expectToken(TokenType::Semicolon, "Expected semicolon after expression");
    return arena.emplace<ast::ExpressionStatement>(expr);
}

// Specific statement parsing methods

ast::Statement* Parser::parseAggregateDeclarationStatement() {
    DISCARD(consumeToken());
    std::vector<std::string> genericTypes;
    std::vector<ast::AggregateField> fields;
    std::string name = expectToken(TokenType::Identifier, "Expected aggregate name after 'aggregate'").getLexeme();

    if (peekTokenType() == TokenType::LeftSquare) {
        DISCARD(consumeToken());
        while (!done() && peekTokenType() != TokenType::RightSquare) {
            std::string genericName = (expectToken(TokenType::Identifier, "Expected a generic type name").getLexeme());

            if (std::ranges::find(genericTypes, genericName) != genericTypes.end()) {
                logError(peekToken().getLine(), peekToken().getColumn(),
                         "Generic type '{}' in aggregate '{}' was already declared", genericName, name);
            } else {
                genericTypes.push_back(genericName);
            }
            if (peekTokenType() != TokenType::RightSquare) {
                expectToken(TokenType::Comma,
                            "Expected a ',' to separate generic types, or a ']' to close the generic type list");
            }
        }
        expectToken(TokenType::RightSquare, "Expected ']' to close generic type list");
    }
    expectToken(TokenType::LeftBrace, "Expected a '{'");

    while (!done()) {
        if (peekTokenType() == TokenType::RightBrace) {
            break;  // Done declaration
        }
        if (peekTokenType() != TokenType::Identifier) {
            logError(peekToken().getLine(), peekToken().getColumn(),
                     "Unexpected token '{}' in aggregate declaration. Expected field name.", peekToken().getLexeme());
            DISCARD(consumeToken());  // Skip the unexpected token to avoid infinite loop
        }
        Token t = consumeToken();
        std::string fieldName = t.getLexeme();
        expectToken(TokenType::Colon, "Expected a ':' to declare an aggregate field type.");
        bool isMutable = false;
        if (peekTokenType() == TokenType::Mut) {
            isMutable = true;
            DISCARD(consumeToken());
        }
        ast::Type* type = parseType(Precedence::Default);
        expectToken(TokenType::Semicolon, "Expected a ';'");

        if (auto duplicate = std::ranges::find(fields, fieldName, &ast::AggregateField::name);
            duplicate != fields.end()) {
            logError(t.getLine(), t.getColumn(),
                     "Duplicate field '{}' in aggregate '{}' (previously declared at line {}, column {})", fieldName,
                     name, duplicate->line, duplicate->column);
        } else {
            fields.push_back(ast::AggregateField{
                .name = fieldName, .type = type, .isMutable = isMutable, .line = t.getLine(), .column = t.getColumn()});
        }
    }

    expectToken(TokenType::RightBrace);

    // Move since AggregateField contains a unique_ptr which is not copyable
    return arena.emplace<ast::AggregateDeclarationStatement>(std::move(name), std::move(genericTypes),
                                                             std::move(fields));
}

ast::Statement* Parser::parseAliasStatement() {
    flags.parsingAliasStatement = true;
    DISCARD(consumeToken());  // consume alias
    std::string alias = expectToken(TokenType::Identifier, "Expected an alias name").getLexeme();
    expectToken(TokenType::Assignment, "Expected '=' after an alias name to introduce the aliased type.");
    ast::Type* baseType = parseType(Precedence::Default);
    expectToken(TokenType::Semicolon, "Expected a ';' after an alias statement");
    flags.parsingAliasStatement = false;
    return arena.emplace<ast::AliasStatement>(baseType, std::move(alias));
}

ast::Statement* Parser::parseBreakStatement() {
    DISCARD(consumeToken());
    expectToken(TokenType::Semicolon);
    return arena.emplace<ast::BreakStatement>();
}

ast::Statement* Parser::parseContinueStatement() {
    DISCARD(consumeToken());
    expectToken(TokenType::Semicolon);
    return arena.emplace<ast::ContinueStatement>();
}

ast::Statement* Parser::parseDoWhileLoopStatement() {
    DISCARD(consumeToken());
    ast::Block body = parseBlock("do-while body");
    expectToken(TokenType::While, "Expected 'while' after a 'do' block");
    expectToken(TokenType::LeftParen, "Expected '(' to introduce while condition");
    ast::Expression* condition = parseExpression(Precedence::Default);
    expectToken(TokenType::RightParen, "Expected ')' to end a while condition");
    expectToken(TokenType::Semicolon, "Expected a ';' after a while clause");
    return arena.emplace<ast::WhileLoopStatement>(std::move(body), std::move(condition), /*isDoWhile=*/true);
}

ast::Statement* Parser::parseEnumDeclarationStatement() {
    Token enumStartToken = consumeToken();
    std::string name = expectToken(TokenType::Identifier, "Expected enum name after 'enum'").getLexeme();
    ast::Type* baseType = nullptr;  // default if no type specified or if there's an error
    std::vector<ast::EnumValue> values;
    if (peekTokenType() == TokenType::Colon) {
        DISCARD(consumeToken());
        Token underlyingTok = peekToken();
        if (!underlyingTok.isInteger()) {
            logError(underlyingTok.getLine(), underlyingTok.getColumn(),
                     "Enums can only have integral types as their underlying type, not {}", underlyingTok.getLexeme());
            DISCARD(consumeToken());
        } else if (underlyingTok.isPrimitiveType()) {
            baseType = arena.emplace<ast::SymbolType>(underlyingTok.getLexeme());
            DISCARD(consumeToken());
        } else {
            logError(underlyingTok.getLine(), underlyingTok.getColumn(), "Expected an underlying type for an enum");
            // If underlying type was just missing, don't skip the opening brace since that error will cascade
            if (underlyingTok.getType() != TokenType::LeftBrace) { DISCARD(consumeToken()); }
        }
    }
    if (!baseType) { baseType = arena.emplace<ast::SymbolType>("int32"); }
    expectToken(TokenType::LeftBrace, "Expected '{' to start the enum body");
    while (!done() && peekTokenType() != TokenType::RightBrace) {
        std::string valueName = expectToken(TokenType::Identifier, "Expected enum value name").getLexeme();
        ast::Expression* valueExpression = nullptr;
        if (peekTokenType() == TokenType::Assignment) {
            DISCARD(consumeToken());
            valueExpression = parseExpression(Precedence::Default);
        }

        if (auto duplicate = std::ranges::find(values, valueName, &ast::EnumValue::name); duplicate != values.end()) {
            logError(peekToken().getLine(), peekToken().getColumn(),
                     "Duplicate enum value '{}' in enum '{}' (previously declared at line {}, column {})", valueName,
                     name, duplicate->line, duplicate->column);
        } else {
            values.push_back(ast::EnumValue{.name = std::move(valueName),
                                            .value = valueExpression,
                                            .line = peekToken().getLine(),
                                            .column = peekToken().getColumn()});
        }
        if (peekTokenType() != TokenType::RightBrace) {
            expectToken(TokenType::Comma, "Expected ',' to separate enum values");
        }
    }
    expectToken(TokenType::RightBrace, "Expected '}' to end the enum body");
    if (values.empty()) {
        logError(enumStartToken.getLine(), enumStartToken.getColumn(), "Enum '{}' has no values", name);
    }
    return arena.emplace<ast::EnumDeclarationStatement>(std::move(name), baseType, std::move(values));
}

ast::Statement* Parser::parseForLoopStatement() {
    DISCARD(consumeToken());  // consume 'for'

    expectToken(TokenType::LeftParen, "Expected '(' to introduce for loop");

    // Initialization Clause
    ast::Statement* init = nullptr;
    if (peekTokenType() != TokenType::Semicolon) {
        if (peekTokenType() == TokenType::Let) {
            init = parseVariableDeclarationStatement();
        } else {
            ast::Expression* initExpr = parseExpression(Precedence::Default);
            init = arena.emplace<ast::ExpressionStatement>(initExpr);
            expectToken(TokenType::Semicolon, "Expected ';' after for-loop initializer");
        }
    } else {
        expectToken(TokenType::Semicolon, "Expected ';' after for-loop initializer");
    }

    // Stop condition
    ast::Expression* condition = nullptr;
    if (peekTokenType() != TokenType::Semicolon) { condition = parseExpression(Precedence::Default); }
    expectToken(TokenType::Semicolon, "Expected ';' after for-loop condition");

    // Post clause (what runs after each loop)
    ast::Expression* post = nullptr;
    if (peekTokenType() != TokenType::RightParen) { post = parseExpression(Precedence::Default); }
    expectToken(TokenType::RightParen, "Expected ')' to end for loop header");

    ast::Block body = parseBlock("for loop body");

    return arena.emplace<ast::ForLoopStatement>(init, condition, post, std::move(body));
}

ast::Statement* Parser::parseFunctionDeclarationStatement() {
    // TODO: Handle function attributes
    DISCARD(consumeToken());
    std::string name = expectToken(TokenType::Identifier, "Expected function name").getLexeme();
    std::vector<ast::FunctionParameter> params;
    std::vector<std::string> genericTypes;
    ast::Type* returnType = nullptr;
    ast::Block body;
    bool hasDefaultParameter = false;
    bool hasVariadicParameter = false;

    if (peekTokenType() == TokenType::LeftSquare) {
        // Generics
        DISCARD(consumeToken());
        while (!done()) {
            if (peekTokenType() == TokenType::RightSquare) {
                break;  // End of generics
            }
            if (peekTokenType() != TokenType::Identifier) {
                logError(peekToken().getLine(), peekToken().getColumn(), "Expected a generic type name");
                DISCARD(consumeToken());  // Skip the unexpected token to avoid infinite loop
            }
            Token genericToken = expectToken(TokenType::Identifier, "Expected a generic type name");
            std::string genericName = genericToken.getLexeme();

            if (std::ranges::find(genericTypes, genericName) != genericTypes.end()) {
                logError(genericToken.getLine(), genericToken.getColumn(),
                         "Duplicate generic type '{}' in function '{}'", genericName, name);
            } else {
                genericTypes.push_back(std::move(genericName));
            }
            if (peekTokenType() != TokenType::RightSquare) {
                expectToken(TokenType::Comma,
                            "Expected a ',' to separate generic types, or a ']' to close the generic type list");
            }
        }
        expectToken(TokenType::RightSquare, "Expected ']' to close generic type list");
    }
    expectToken(TokenType::LeftParen);

    while (!done()) {
        if (peekTokenType() == TokenType::RightParen) { break; }
        bool isMutable = false;
        bool isVariadic = false;
        ast::Expression* defaultValue = nullptr;

        Token t = expectToken(TokenType::Identifier, "Expected a variable name");
        std::string paramName = t.getLexeme();
        if (peekTokenType() == TokenType::Ellipsis) {
            DISCARD(consumeToken());
            if (hasVariadicParameter) {
                logError(t.getLine(), t.getColumn(), "Only one variadic parameter is allowed in function '{}'", name);
            } else {
                isVariadic = true;
                hasVariadicParameter = true;
            }
        } else if (hasVariadicParameter) {
            logError(t.getLine(), t.getColumn(), "Parameter '{}' cannot follow a variadic parameter", paramName);
        }

        expectToken(TokenType::Colon);
        if (peekTokenType() == TokenType::Mut) {
            DISCARD(consumeToken());
            isMutable = true;
        }
        ast::Type* param_type = parseType(Precedence::Default);

        if (peekTokenType() == TokenType::Assignment) {
            DISCARD(consumeToken());

            hasDefaultParameter = true;
            defaultValue = parseExpression(Precedence::Default);

            if (isVariadic) {
                logError(t.getLine(), t.getColumn(), "Variadic parameter '{}' cannot have a default value", paramName);
            }

        } else if (hasDefaultParameter) {
            logError(t.getLine(), t.getColumn(), "Non-default parameter '{}' cannot follow a default parameter",
                     paramName);
        }

        if (auto duplicate = std::ranges::find(params, paramName, &ast::FunctionParameter::name);
            duplicate != params.end()) {
            logError(t.getLine(), t.getColumn(),
                     "Duplicate parameter '{}' in function '{}' (previously declared at line {}, column {})", paramName,
                     name, duplicate->line, duplicate->column);
        } else {
            params.push_back(ast::FunctionParameter{.name = paramName,
                                                    .type = param_type,
                                                    .isMutable = isMutable,
                                                    .isVariadic = isVariadic,
                                                    .defaultValue = defaultValue,
                                                    .line = t.getLine(),
                                                    .column = t.getColumn()});
        }
        if (peekTokenType() != TokenType::RightParen && peekTokenType() != TokenType::EndOfFile) {
            expectToken(TokenType::Comma,
                        "Expected a ',' to separate function parameters, or a ) to close the parameter list");
        }
    }
    expectToken(TokenType::RightParen);
    if (peekTokenType() == TokenType::Arrow) {
        DISCARD(consumeToken());
        returnType = parseType(Precedence::Default);
    }
    return arena.emplace<ast::FunctionDeclarationStatement>(std::move(name), std::move(genericTypes), std::move(params),
                                                            returnType, parseBlock("function body"));
}

ast::Statement* Parser::parseIfStatement() {
    DISCARD(consumeToken());

    expectToken(TokenType::LeftParen, "Expected '(' to introduce if condition");
    ast::Expression* condition = parseExpression(Precedence::Default);
    expectToken(TokenType::RightParen, "Expected ')' to end if condition");
    ast::Block body = parseBlock("if body");

    std::vector<ast::ElifClause> elifs;
    while (peekTokenType() == TokenType::Elif) {
        DISCARD(consumeToken());

        expectToken(TokenType::LeftParen, "Expected '(' to introduce elif condition");
        ast::Expression* elifCondition = parseExpression(Precedence::Default);

        expectToken(TokenType::RightParen, "Expected ')' to end elif condition");

        elifs.emplace_back(elifCondition, parseBlock("elif body"));
    }
    ast::Block elseBody;
    if (peekTokenType() == TokenType::Else) {
        DISCARD(consumeToken());
        elseBody = parseBlock("else body");
        if (elseBody.empty()) {
            // If the body was empty, this means there's a dangling else
            // We still want this printed out, but the toString method determines the presence of an else block
            // by checking the the body is empty
            // in this edge case, there is a body but it's empty, which makes the printing logic think there isn't one
            // to get around this, add a dummy statement (doesn't print anything) so the empty else block is still
            // printed
            elseBody.push_back(ast::getEmptyStatement());
        }
    }
    return arena.emplace<ast::IfStatement>(condition, std::move(body), std::move(elifs), std::move(elseBody));
}

ast::Statement* Parser::parseImportStatement() {
    const std::size_t startLine = peekToken().getLine();
    const std::size_t startColumn = peekToken().getColumn();

    if (flags.hasParsedFileHeader) {
        logging::logWarning(startLine, startColumn, "Imports should go at the top of the file");
    }
    DISCARD(consumeToken());
    std::vector<std::string> path;
    path.push_back(expectToken(TokenType::Identifier, "Expected a module name or path").getLexeme());
    while (peekTokenType() == TokenType::ScopeResolution) {
        DISCARD(consumeToken());  // Consume '::'
        path.push_back(expectToken(TokenType::Identifier, "Expected identifier after '::'").getLexeme());
    }
    std::string alias;
    if (peekTokenType() == TokenType::As) {
        DISCARD(consumeToken());
        alias = expectToken(TokenType::Identifier, "Expected an identifier as an import alias").getLexeme();
    }
    expectToken(TokenType::Semicolon, "Expected a ';' to end an import statement");

    bool duplicate = false;
    for (const auto& [existingPath, existingAlias] : imports) {
        if (path == existingPath) {
            std::string imported
                = std::accumulate(existingPath.begin() + 1, existingPath.end(),
                                  existingPath[0],  // existingPath should never be empty
                                  [](const std::string& a, const std::string& b) { return a + "::" + b; });

            logging::logWarning(startLine, startColumn, "Duplicate import of {}", imported);
            duplicate = true;
            break;

        } else if (alias == existingAlias && !alias.empty()) {
            logging::logWarning(startLine, startColumn, "Alias {} was already used", existingAlias);
            duplicate = true;
            break;
        }
    }
    if (!duplicate) { imports.push_back({.path = std::move(path), .alias = std::move(alias)}); }
    // Dummy node since imports are stored separately
    return ast::getEmptyStatement();
}

ast::Statement* Parser::parseModuleDeclarationStatement() {
    const lexer::Token temp = consumeToken();
    const std::size_t startLine = temp.getLine();
    const std::size_t startColumn = temp.getColumn();
    if (flags.hasParsedFileHeader) {
        logging::logWarning(startLine, startColumn, "Module declarations should go at the top of the file");
    }
    const std::string name = expectToken(TokenType::Identifier, "Expected a module name").getLexeme();
    expectToken(TokenType::Semicolon, "Expected a ';' after a module declaration");
    if (!this->moduleName.empty()) {
        logError(
            startLine, startColumn,
            "A module name has previously been declared in this file. Files can only have one module declaration.");
    } else {
        this->moduleName = name;
    }

    // Dummy node since there's no need to semantically analyze module declarations (that happens here)
    return ast::getEmptyStatement();
}

ast::Statement* Parser::parseNamespace() {
    DISCARD(consumeToken());  // skip 'namespace'
    std::string name = expectToken(TokenType::Identifier, "Expected a namespace name after 'namespace'").getLexeme();
    ast::Block body = parseBlock("namespace " + name);
    return arena.emplace<ast::NamespaceStatement>(std::move(name), std::move(body));
}

ast::Statement* Parser::parseRedundantSemicolon() {
    DISCARD(consumeToken());
    return ast::getEmptyStatement();
}

ast::Statement* Parser::parseReturnStatement() {
    DISCARD(consumeToken());
    ast::Expression* expression = nullptr;
    if (peekTokenType() != TokenType::Semicolon) {
        // If the next token is not a semicolon, parse an expression
        // If it is, this is a null return statement
        expression = parseExpression(Precedence::Default);
    }
    expectToken(TokenType::Semicolon, "Expected semicolon after return statement");
    return arena.emplace<ast::ReturnStatement>(expression);
}

ast::Statement* Parser::parseSwitchStatement() {
    Token temp = consumeToken();
    std::size_t startLine = temp.getLine(), startColumn = temp.getColumn();
    expectToken(TokenType::LeftParen, "Expected '(' to introduce switch variable");

    ast::Expression* variable = parseExpression(Precedence::Default);

    expectToken(TokenType::RightParen, "Expected ')' to end switch variable");
    expectToken(TokenType::LeftBrace, "Expected '{' to start the switch body");

    std::vector<ast::CaseClause> cases;
    ast::Block defaultBody;
    bool hasDefault = false;

    while (peekTokenType() == TokenType::Case) {
        DISCARD(consumeToken());  // consume 'case'
        std::vector<ast::Expression*> caseValues;

        do {
            caseValues.push_back(parseExpression(Precedence::Default));
            if (peekTokenType() == TokenType::Comma) {
                DISCARD(consumeToken());  // consume ','
            } else {
                break;  // no more cases
            }
        } while (true);
        ast::Block caseBody;
        expectToken(TokenType::Colon,
                    std::format("Expected ':' after case value{}", (caseValues.size() > 1 ? "s" : "")));

        while (peekTokenType() != TokenType::Case && peekTokenType() != TokenType::Default
               && peekTokenType() != TokenType::RightBrace) {
            caseBody.push_back(parseStatement());
        }
        cases.push_back(ast::CaseClause{.values = std::move(caseValues), .body = std::move(caseBody)});
    }
    if (peekTokenType() == TokenType::Default) {
        hasDefault = true;
        DISCARD(consumeToken());
        expectToken(TokenType::Colon, "Expected ':' after default case");
        while (peekTokenType() != TokenType::RightBrace) { defaultBody.push_back(parseStatement()); }
    }
    if (cases.empty() && defaultBody.empty()) {
        logging::logWarning(startLine, startColumn, "Switch statement has no cases or default body");
    }
    if (hasDefault && defaultBody.empty()) {
        // This means there's a dangling default (i.e. nothing should happen in the default case)
        // The printing logic determines the presence of a default statement by checking if the body is empty
        // However in this edge case, there is a default statement with no body which makes the printing logic think
        // there's no default statement (leading to a confusing printout)
        // To get around this, push a dummy statement (doesn't print anything) so the printing logic recognizes
        // there's a body but still prints nothing after it
        defaultBody.push_back(ast::getEmptyStatement());
    }
    expectToken(TokenType::RightBrace, "Expected '}' to end the switch body");

    return arena.emplace<ast::SwitchStatement>(variable, std::move(cases), std::move(defaultBody));
}

ast::Statement* Parser::parseVisibilityAffectedStatement() {
    ast::Visibility visibility;
    switch (consumeToken().getType()) {
        case TokenType::Private: visibility = ast::Visibility::Private; break;
        case TokenType::Public: visibility = ast::Visibility::Public; break;
        default:
            ASSERT_UNREACHABLE("Unexpected token type in parseVisibilityAffectedStatement: "
                               + lexer ::tokenTypeToString(peekTokenType()));
    }
    std::size_t startLine = peekToken().getLine(), startColumn = peekToken().getColumn();
    switch (peekTokenType()) {
        case TokenType::Alias: {
            auto* tempAlias = static_cast<ast::AliasStatement*>(parseAliasStatement());
            tempAlias->visibility = visibility;
            return tempAlias;
        }
        case TokenType::Aggregate: {
            auto* tempAggregate
                = static_cast<ast::AggregateDeclarationStatement*>(parseAggregateDeclarationStatement());
            tempAggregate->visibility = visibility;
            return tempAggregate;
        }
        case TokenType::Enum: {
            auto* tempEnum = static_cast<ast::EnumDeclarationStatement*>(parseEnumDeclarationStatement());
            tempEnum->visibility = visibility;
            return tempEnum;
        }
        case TokenType::Func: {
            auto* tempFunction = static_cast<ast::FunctionDeclarationStatement*>(parseFunctionDeclarationStatement());
            tempFunction->visibility = visibility;
            return tempFunction;
        }
        default:
            logError(startLine, startColumn, "{} cannot follow a visibility modifier",
                     lexer::tokenTypeToString(peekTokenType()));
            // Parse the statement as if it had no visibility modifier
            return parseStatement();
    }
}

ast::Statement* Parser::parseWhileLoopStatement() {
    DISCARD(consumeToken());
    expectToken(TokenType::LeftParen, "Expected '(' to introduce while condition");
    ast::Expression* condition = parseExpression(Precedence::Default);
    expectToken(TokenType::RightParen, "Expected ')' to end while condition");

    return arena.emplace<ast::WhileLoopStatement>(parseBlock("while loop body"), condition);
}
}  // namespace Manganese::parser
