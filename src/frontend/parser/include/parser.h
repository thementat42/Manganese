#ifndef PARSER_H
#define PARSER_H

#include <memory>
#include <optional>

#include "../../global_macros.h"
#include "../../lexer/include/lexer.h"
#include "nodes.h"

MANG_BEGIN
namespace parser {
constexpr auto NONE = std::nullopt;
class Parser {
   private:
    std::unique_ptr<lexer::Lexer> lexer;
    std::vector<lexer::Token> tokenBuffer;  // save some tokens to allow for easy lookahead/lookbehind
    lexer::Token currentToken, nextToken;

    bool expectKeyword(lexer::KeywordType kwtype, const char* errMsg);
    bool expectOperator(lexer::OperatorType optype, const char* errMsg);
    bool expectToken(lexer::TokenType type, const char* errMsg);
    bool expectToken(lexer::Token& token, const char* errMsg);

    //~ Expression Parsing
    ExpressionPtr parseExpression();
    ExpressionPtr parseBinaryExpression(int precedence = 0);
    ExpressionPtr parseUnaryExpression();
    ExpressionPtr parsePrimaryExpression();
    ExpressionPtr parseLiteral();
    ExpressionPtr parseFunctionCall(NodePtr callee);
    ExpressionPtr parseBinaryOpAssignment();

    //~ Statement parsing
    StatementPtr parseStatement();
    StatementPtr parseBlock();

    StatementNode parseVariableDecl();
    NodePtr parseType();
    NodePtr parseTypeQualifiers();

    StatementNode parseConditionalStatement();
    StatementNode parseSwitchStatement();
    StatementNode parseReturnStatement();
    StatementNode parseLoopStatement();
    StatementNode parseFunctionDecl();
    StatementNode parseLambdaStatement();
    StatementNode parseParameterList();

    NodePtr parseBundleDecl();
    NodePtr parseEnumDecl();

    // Precedence helpers
    static std::optional<uint16_t> getBinaryOperatorPrecedence(const lexer::Token& token);
    static std::optional<uint16_t> getUnaryOperatorPrecedence(const lexer::Token& token);

    // Helper functions
    void advance();
    bool doneParsing() const;


   public:
    Parser() = default;
    Parser(const std::string& source, lexer::Mode mode = lexer::Mode::File);
    ~Parser();

    NodePtr parse();
};

}  // namespace parser
MANG_END

#endif  // PARSER_H