/**
 * @file parser_statements.cpp
 * @brief This file contains the implementation of statement parsing in the parser. It is split into its own file for readability and maintainability.
 */

#include <frontend/parser.h>
#include <global_macros.h>

#include <format>

MANGANESE_BEGIN
namespace parser {

StatementPtr Parser::parseStatement() {
    auto it = statementLookup.find(currentToken().getType());
    if (it != statementLookup.end()) {
        // Call the handler for the current token type
        return it->second(this);
    }
    ExpressionPtr expr = parseExpression(OperatorBindingPower::Default);
    expectToken(TokenType::Semicolon, "Expected semicolon after expression");
    return std::make_unique<ast::ExpressionStatement>(std::move(expr));
}

StatementPtr Parser::parseVariableDeclarationStatement() {
    Token declarationToken = advance();
    bool isConstant = declarationToken.getType() == TokenType::Const;

    str identifier = expectToken(
                         TokenType::Identifier,
                         std::format("Expected a variable name after {}", declarationToken.getLexeme()))
                         .getLexeme();

    expectToken(TokenType::Assignment, "Expected an '=' after a variable declaration");
    ExpressionPtr value = parseExpression(OperatorBindingPower::Assignment);
    expectToken(TokenType::Semicolon, "Expected a ';' to end a variable declaration");

    // TODO: Parse type declarations
    return std::make_unique<ast::VariableDeclarationStatement>(
        isConstant, identifier, defaultVisibility, std::move(value));
}

}  // namespace parser

MANGANESE_END