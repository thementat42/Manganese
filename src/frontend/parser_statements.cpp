/**
 * @file parser_statements.cpp
 * @brief This file contains the implementation of statement parsing in the parser. It is split into its own file for readability and maintainability.
 */

#include "../global_macros.h"
#include "include/parser.h"

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

}  // namespace parser

MANGANESE_END