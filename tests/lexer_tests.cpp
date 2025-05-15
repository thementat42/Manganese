#include <cassert>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "../src/frontend/lexer/include/lexer.h"
#include "../src/frontend/lexer/include/token.h"
#include "../src/global_macros.h"
#include "testrunner.h"

using Manganese::lexer::Mode;
using Manganese::lexer::Token;
using Manganese::lexer::tokenize;
using Manganese::lexer::TokenType;

bool checkToken(const Token& token, TokenType expectedType, const std::string& expectedLexeme) {
    if (token.type != expectedType) {
        std::cout << "Expected token type " << Token::tokenTypeToString(expectedType)
                  << " but got " << Token::tokenTypeToString(token.type) << " (lexeme was " << token.lexeme << ")" << std::endl;
        return false;
    }
    if (token.lexeme != expectedLexeme) {
        std::cout << "Expected lexeme '" << expectedLexeme
                  << "' but got '" << token.lexeme << "'" << std::endl;
        return false;
    }
    return true;
}

void removeEOFToken(std::vector<Token>& tokens) {
    // Lexer always pushes an EOF token at the end of the token stream
    // This messes up the testing, since it was not included in the test cases
    // So we remove it here
    if (!tokens.empty() && tokens.back().type == TokenType::END_OF_FILE) {
        tokens.pop_back();
    }
}

bool testEmptyString() {
    auto tokens = tokenize("", Mode::STRING);
    removeEOFToken(tokens);
    return tokens.empty();
}

bool testWhitespace() {
    auto tokens = tokenize("  \t\n\r  ", Mode::STRING);
    removeEOFToken(tokens);
    return tokens.empty();
}

bool testComments() {
    auto tokens = tokenize("# This is a comment\nint x; /*This is\n a\n multiline comment!*/", Mode::STRING);
    removeEOFToken(tokens);
    std::cout << "Tokens: ";
    for (const auto& token : tokens) {
        std::cout << token.lexeme << " ";
    }
    std::cout << std::endl;
    if (tokens.size() != 3) {
        std::cout << "Expected 3 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    return checkToken(tokens[0], TokenType::KEYWORD, "int") &&
           checkToken(tokens[1], TokenType::IDENTIFIER, "x") &&
           checkToken(tokens[2], TokenType::SEMICOLON, ";");
}

bool testIdentifiers() {
    auto tokens = tokenize("foo bar baz _var var123", Mode::STRING);
    std::cout << "Tokens: ";
    for (const auto& token : tokens) {
        std::cout << token.lexeme << " ";
    }
    std::cout << std::endl;
    removeEOFToken(tokens);
    if (tokens.size() != 5) {
        std::cout << "Expected 5 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    return checkToken(tokens[0], TokenType::IDENTIFIER, "foo") &&
           checkToken(tokens[1], TokenType::IDENTIFIER, "bar") &&
           checkToken(tokens[2], TokenType::IDENTIFIER, "baz") &&
           checkToken(tokens[3], TokenType::IDENTIFIER, "_var") &&
           checkToken(tokens[4], TokenType::IDENTIFIER, "var123");
}

bool testKeywords() {
    auto tokens = tokenize("int float char string if else while for return", Mode::STRING);
    std::cout << "Tokens: ";
    for (const auto& token : tokens) {
        std::cout << token.lexeme << " ";
    }
    std::cout << std::endl;
    removeEOFToken(tokens);
    if (tokens.size() != 9) {
        std::cout << "Expected 9 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    return checkToken(tokens[0], TokenType::KEYWORD, "int") &&
           checkToken(tokens[1], TokenType::KEYWORD, "float") &&
           checkToken(tokens[2], TokenType::KEYWORD, "char") &&
           checkToken(tokens[3], TokenType::KEYWORD, "string") &&
           checkToken(tokens[4], TokenType::KEYWORD, "if") &&
           checkToken(tokens[5], TokenType::KEYWORD, "else") &&
           checkToken(tokens[6], TokenType::KEYWORD, "while") &&
           checkToken(tokens[7], TokenType::KEYWORD, "for") &&
           checkToken(tokens[8], TokenType::KEYWORD, "return");
}

bool testIntegerLiterals() {
    auto tokens = tokenize("0 123 456789 0xFFF 0b1001 0o33", Mode::STRING);
    std::cout << "Tokens: ";
    for (const auto& token : tokens) {
        std::cout << token.lexeme << "";
    }
    std::cout << std::endl;
    removeEOFToken(tokens);
    if (tokens.size() != 6) {
        std::cout << "Expected 6 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    return checkToken(tokens[0], TokenType::INTEGER, "0") &&
           checkToken(tokens[1], TokenType::INTEGER, "123") &&
           checkToken(tokens[2], TokenType::INTEGER, "456789") &&
           checkToken(tokens[3], TokenType::INTEGER, "0xFFF") &&
           checkToken(tokens[4], TokenType::INTEGER, "0b1001") &&
           checkToken(tokens[5], TokenType::INTEGER, "0o33");
}

bool testFloatLiterals() {
    auto tokens = tokenize("0.0 1.23 456.789", Mode::STRING);
    std::cout << "Tokens: ";
    for (const auto& token : tokens) {
        std::cout << token.lexeme << " ";
    }
    std::cout << std::endl;
    removeEOFToken(tokens);
    if (tokens.size() != 3) {
        std::cout << "Expected 3 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    return checkToken(tokens[0], TokenType::FLOAT, "0.0") &&
           checkToken(tokens[1], TokenType::FLOAT, "1.23") &&
           checkToken(tokens[2], TokenType::FLOAT, "456.789");
}

bool testCharLiterals() {
    auto tokens = tokenize("'a' '\\n' '\\'' '\\\\' '\\t' '\\u1234'", Mode::STRING);
    std::cout << "Tokens: ";
    for (const auto& token : tokens) {
        std::cout << token.lexeme << " ";
    }
    std::cout << std::endl;
    removeEOFToken(tokens);
    if (tokens.size() != 6) {
        std::cout << "Expected 6 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    return checkToken(tokens[0], TokenType::CHARACTER, "a") &&
           checkToken(tokens[1], TokenType::CHARACTER, "\n") &&
           checkToken(tokens[2], TokenType::CHARACTER, "\'") &&
           checkToken(tokens[3], TokenType::CHARACTER, "\\") &&
           checkToken(tokens[4], TokenType::CHARACTER, "\t") &&
           checkToken(tokens[5], TokenType::CHARACTER, "\u1234");
}

bool testStringLiterals() {
    auto tokens = tokenize("\"hello\" \"world\" \"escaped \\\"quote\\\"\"", Mode::STRING);
    std::cout << "Tokens: ";
    for (const auto& token : tokens) {
        std::cout << token.lexeme << " ";
    }
    std::cout << std::endl;
    removeEOFToken(tokens);
    if (tokens.size() != 3) {
        std::cout << "Expected 3 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    return checkToken(tokens[0], TokenType::STRING_LITERAL, "hello") &&
           checkToken(tokens[1], TokenType::STRING_LITERAL, "world") &&
           checkToken(tokens[2], TokenType::STRING_LITERAL, "escaped \"quote\"");
}

bool testOperators() {
    auto tokens = tokenize("+ - * / // % ** ++ -- += -= *= /= //= %= **= == != && || ! & | ~ ^ &= |= ~= ^=  ? @ . : :: = -> ...", Mode::STRING);
    std::cout << "Tokens: ";
    for (const auto& token : tokens) {
        std::cout << token.lexeme << " ";
    }
    std::cout << std::endl;
    removeEOFToken(tokens);
    std::cout << '\n';
    if (tokens.size() != 37) {
        std::cout << "Expected 37 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    return checkToken(tokens[0], TokenType::OPERATOR, "+") &&
           checkToken(tokens[1], TokenType::OPERATOR, "-") &&
           checkToken(tokens[2], TokenType::OPERATOR, "*") &&
           checkToken(tokens[3], TokenType::OPERATOR, "/") &&
           checkToken(tokens[4], TokenType::OPERATOR, "//") &&
           checkToken(tokens[5], TokenType::OPERATOR, "%") &&
           checkToken(tokens[6], TokenType::OPERATOR, "**") &&
           checkToken(tokens[7], TokenType::OPERATOR, "++") &&
           checkToken(tokens[8], TokenType::OPERATOR, "--") &&
           checkToken(tokens[9], TokenType::OPERATOR, "+=") &&
           checkToken(tokens[10], TokenType::OPERATOR, "-=") &&
           checkToken(tokens[11], TokenType::OPERATOR, "*=") &&
           checkToken(tokens[12], TokenType::OPERATOR, "/=") &&
           checkToken(tokens[13], TokenType::OPERATOR, "//=") &&
           checkToken(tokens[14], TokenType::OPERATOR, "%=") &&
           checkToken(tokens[15], TokenType::OPERATOR, "**=") &&
           checkToken(tokens[16], TokenType::OPERATOR, "==") &&
           checkToken(tokens[17], TokenType::OPERATOR, "!=") &&
           checkToken(tokens[18], TokenType::OPERATOR, "&&") &&
           checkToken(tokens[19], TokenType::OPERATOR, "||") &&
           checkToken(tokens[20], TokenType::OPERATOR, "!") &&
           checkToken(tokens[21], TokenType::OPERATOR, "&") &&
           checkToken(tokens[22], TokenType::OPERATOR, "|") &&
           checkToken(tokens[23], TokenType::OPERATOR, "~") &&
           checkToken(tokens[24], TokenType::OPERATOR, "^") &&
           checkToken(tokens[25], TokenType::OPERATOR, "&=") &&
           checkToken(tokens[26], TokenType::OPERATOR, "|=") &&
           checkToken(tokens[27], TokenType::OPERATOR, "~=") &&
           checkToken(tokens[28], TokenType::OPERATOR, "^=") &&
           checkToken(tokens[29], TokenType::OPERATOR, "?") &&
           checkToken(tokens[30], TokenType::OPERATOR, "@") &&
           checkToken(tokens[31], TokenType::OPERATOR, ".") &&
           checkToken(tokens[32], TokenType::COLON, ":") &&
           checkToken(tokens[33], TokenType::OPERATOR, "::") &&
           checkToken(tokens[34], TokenType::OPERATOR, "=") &&
           checkToken(tokens[35], TokenType::OPERATOR, "->") &&
           checkToken(tokens[36], TokenType::OPERATOR, "...");
}

bool testBrackets() {
    auto tokens = tokenize("( ) { } [ ]", Mode::STRING);
    std::cout << "Tokens: ";
    for (const auto& token : tokens) {
        std::cout << token.lexeme << " ";
    }
    std::cout << std::endl;
    removeEOFToken(tokens);
    if (tokens.size() != 6) {
        std::cout << "Expected 6 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    return checkToken(tokens[0], TokenType::LEFT_PAREN, "(") &&
           checkToken(tokens[1], TokenType::RIGHT_PAREN, ")") &&
           checkToken(tokens[2], TokenType::LEFT_BRACE, "{") &&
           checkToken(tokens[3], TokenType::RIGHT_BRACE, "}") &&
           checkToken(tokens[4], TokenType::LEFT_SQUARE, "[") &&
           checkToken(tokens[5], TokenType::RIGHT_SQUARE, "]");
}

bool testPunctuation() {
    auto tokens = tokenize("; , . ? @ : ::", Mode::STRING);
    std::cout << "Tokens: ";
    for (const auto& token : tokens) {
        std::cout << token.lexeme << " ";
    }
    std::cout << std::endl;
    removeEOFToken(tokens);
    if (tokens.size() != 7) {
        std::cout << "Expected 7 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    return checkToken(tokens[0], TokenType::SEMICOLON, ";") &&
           checkToken(tokens[1], TokenType::COMMA, ",") &&
           checkToken(tokens[2], TokenType::OPERATOR, ".") &&
           checkToken(tokens[3], TokenType::OPERATOR, "?") &&
           checkToken(tokens[4], TokenType::OPERATOR, "@") &&
           checkToken(tokens[5], TokenType::COLON, ":") &&
           checkToken(tokens[6], TokenType::OPERATOR, "::");
}

bool testCompleteProgram() {
    const std::string program = R"(
func main() -> int {
    int x = 5;
    float y = 10.5;
    print(x);
    print(y);
}
    )";

    auto tokens = tokenize(program, Mode::STRING);
    std::cout << "Tokens: ";
    for (const auto& token : tokens) {
        std::cout << token.lexeme << " ";
    }
    std::cout << std::endl;
    removeEOFToken(tokens);

    if (tokens.empty()) {
        std::cout << "Expected non-empty tokens" << std::endl;
        return false;
    }
    return checkToken(tokens[0], TokenType::KEYWORD, "func") &&
           checkToken(tokens[1], TokenType::IDENTIFIER, "main") &&
           checkToken(tokens[2], TokenType::LEFT_PAREN, "(") &&
           checkToken(tokens[3], TokenType::RIGHT_PAREN, ")") &&
           checkToken(tokens[4], TokenType::OPERATOR, "->") &&
           checkToken(tokens[5], TokenType::KEYWORD, "int") &&
           checkToken(tokens[6], TokenType::LEFT_BRACE, "{");
}

bool testNestedBrackets() {
    auto tokens = tokenize("arr<arr<int>> foo", Mode::STRING);
    std::cout << "Tokens: ";
    for (const auto& token : tokens) {
        std::cout << token.lexeme << " ";
    }
    std::cout << std::endl;
    removeEOFToken(tokens);
    if (tokens.size() != 8) {
        std::cout << "Expected 8 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    return checkToken(tokens[0], TokenType::KEYWORD, "arr") &&
           checkToken(tokens[1], TokenType::LEFT_ANGLE, "<") &&
           checkToken(tokens[2], TokenType::KEYWORD, "arr") &&
           checkToken(tokens[3], TokenType::LEFT_ANGLE, "<") &&
           checkToken(tokens[4], TokenType::KEYWORD, "int") &&
           checkToken(tokens[5], TokenType::RIGHT_ANGLE, ">") &&
           checkToken(tokens[6], TokenType::RIGHT_ANGLE, ">") &&
           checkToken(tokens[7], TokenType::IDENTIFIER, "foo");
}

bool testInvalidChar() {
    auto tokens = tokenize("'too long' '\\z' '\\u9Z99' ", Mode::STRING);
    removeEOFToken(tokens);
    if (tokens.size() != 3) {
        std::cout << "Expected 3 token, got " << tokens.size() << std::endl;
        return false;
    }

    return tokens[0].type == TokenType::INVALID &&
           tokens[1].type == TokenType::INVALID &&
           tokens[2].type == TokenType::INVALID;
}

bool testInvalidEscapeSequence() {
    auto tokens = tokenize("'\\z'", Mode::STRING);
    std::cout << "Tokens: ";
    for (const auto& token : tokens) {
        std::cout << token.lexeme << " ";
    }
    std::cout << std::endl;
    removeEOFToken(tokens);
    if (tokens.size() != 1) {
        std::cout << "Expected 1 token, got " << tokens.size() << std::endl;
        return false;
    }

    return tokens[0].type == TokenType::INVALID;
}

int main() {
    TestRunner runner;

    // Register all tests
    runner.runTest("Empty String", testEmptyString);
    runner.runTest("Whitespace", testWhitespace);
    runner.runTest("Comments", testComments);
    runner.runTest("Identifiers", testIdentifiers);
    runner.runTest("Keywords", testKeywords);
    runner.runTest("Operators", testOperators);
    runner.runTest("Integer Literals", testIntegerLiterals);
    runner.runTest("Float Literals", testFloatLiterals);
    runner.runTest("Character Literals", testCharLiterals);
    runner.runTest("String Literals", testStringLiterals);
    runner.runTest("Brackets", testBrackets);
    runner.runTest("Punctuation", testPunctuation);
    runner.runTest("Complete Program", testCompleteProgram);
    runner.runTest("Nested Brackets", testNestedBrackets);
    runner.runTest("Invalid Character", testInvalidChar);
    runner.runTest("Invalid Escape Sequence", testInvalidEscapeSequence);

    runner.printSummary();

    return runner.allTestsPassed() ? 0 : 1;

    // g++ tests/test_lexer.cpp tests/test_runner.cpp src/frontend/keywords.cpp src/frontend/lexer.cpp src/frontend/operators.cpp src/frontend/token.cpp -o tl; ./tl
}