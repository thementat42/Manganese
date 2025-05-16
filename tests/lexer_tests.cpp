#include <cassert>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "../src/frontend/lexer/include/lexer.h"
#include "../src/frontend/lexer/include/token.h"
#include "../src/global_macros.h"
#include "testrunner.h"

using Manganese::lexer::Lexer;
using Manganese::lexer::Mode;
using Manganese::lexer::Token;
using TokenList = std::vector<Token>;
using Manganese::lexer::TokenType;

// Utility function to convert string to tokens using the class-based Lexer
std::vector<Token> tokensFromString(const std::string& source) {
    Lexer lexer(source, Mode::String);
    std::vector<Token> tokens;

    // Consume tokens until we hit EOF
    while (true) {
        Token token = lexer.consumeToken();
        if (token.getType() == TokenType::EndOfFile) {
            break;
        }
        tokens.push_back(token);
    }

    return tokens;
}

bool checkToken(const Token& token, TokenType expectedType, const std::string& expectedLexeme) {
    if (token.getType() != expectedType) {
        std::cout << "Expected token type " << Token::tokenTypeToString(expectedType)
                  << " but got " << Token::tokenTypeToString(token.getType()) << " (lexeme was " << token.getLexeme() << ")" << std::endl;
        return false;
    }
    if (token.getLexeme() != expectedLexeme) {
        std::cout << "Expected lexeme '" << expectedLexeme
                  << "' but got '" << token.getLexeme() << "'" << std::endl;
        return false;
    }
    return true;
}

bool testEmptyString() {
    auto tokens = tokensFromString("");
    return tokens.empty();
}

bool testWhitespace() {
    auto tokens = tokensFromString("  \t\n\r  ");
    return tokens.empty();
}

bool testComments() {
    auto tokens = tokensFromString("# This is a comment\nint x; /*This is\n a\n multiline comment!*/");
    std::cout << "Tokens: ";
    for (const auto& token : tokens) {
        std::cout << token.getLexeme() << " ";
    }
    std::cout << std::endl;
    if (tokens.size() != 3) {
        std::cout << "Expected 3 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    return checkToken(tokens[0], TokenType::Keyword, "int") &&
           checkToken(tokens[1], TokenType::Identifier, "x") &&
           checkToken(tokens[2], TokenType::Semicolon, ";");
}

bool testIdentifiers() {
    auto tokens = tokensFromString("foo bar baz _var var123");
    std::cout << "Tokens: ";
    for (const auto& token : tokens) {
        std::cout << token.getLexeme() << " ";
    }
    std::cout << std::endl;
    if (tokens.size() != 5) {
        std::cout << "Expected 5 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    return checkToken(tokens[0], TokenType::Identifier, "foo") &&
           checkToken(tokens[1], TokenType::Identifier, "bar") &&
           checkToken(tokens[2], TokenType::Identifier, "baz") &&
           checkToken(tokens[3], TokenType::Identifier, "_var") &&
           checkToken(tokens[4], TokenType::Identifier, "var123");
}

bool testKeywords() {
    auto tokens = tokensFromString("alias arr as blueprint bool break bundle case cast char const continue default do elif else enum false float float32 float64 for func garbage if import int int16 int32 int64 int8 lambda map module owns ptr public readonly repeat return set str switch true typeof uint uint8 uint16 uint32 uint64 vec while foo");
    std::cout << "Tokens: ";
    for (const auto& token : tokens) {
        std::cout << token.getLexeme() << " ";
    }
    std::cout << std::endl;
    if (tokens.size() != 53) {
        std::cout << "Expected 53 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    return checkToken(tokens[0], TokenType::Keyword, "alias") &&
           checkToken(tokens[1], TokenType::Keyword, "arr") &&
           checkToken(tokens[2], TokenType::Keyword, "as") &&
           checkToken(tokens[3], TokenType::Keyword, "blueprint") &&
           checkToken(tokens[4], TokenType::Keyword, "bool") &&
           checkToken(tokens[5], TokenType::Keyword, "break") &&
           checkToken(tokens[6], TokenType::Keyword, "bundle") &&
           checkToken(tokens[7], TokenType::Keyword, "case") &&
           checkToken(tokens[8], TokenType::Keyword, "cast") &&
           checkToken(tokens[9], TokenType::Keyword, "char") &&
           checkToken(tokens[10], TokenType::Keyword, "const") &&
           checkToken(tokens[11], TokenType::Keyword, "continue") &&
           checkToken(tokens[12], TokenType::Keyword, "default") &&
           checkToken(tokens[13], TokenType::Keyword, "do") &&
           checkToken(tokens[14], TokenType::Keyword, "elif") &&
           checkToken(tokens[15], TokenType::Keyword, "else") &&
           checkToken(tokens[16], TokenType::Keyword, "enum") &&
           checkToken(tokens[17], TokenType::Keyword, "false") &&
           checkToken(tokens[18], TokenType::Keyword, "float") &&
           checkToken(tokens[19], TokenType::Keyword, "float32") &&
           checkToken(tokens[20], TokenType::Keyword, "float64") &&
           checkToken(tokens[21], TokenType::Keyword, "for") &&
           checkToken(tokens[22], TokenType::Keyword, "func") &&
           checkToken(tokens[23], TokenType::Keyword, "garbage") &&
           checkToken(tokens[24], TokenType::Keyword, "if") &&
           checkToken(tokens[25], TokenType::Keyword, "import") &&
           checkToken(tokens[26], TokenType::Keyword, "int") &&
           checkToken(tokens[27], TokenType::Keyword, "int16") &&
           checkToken(tokens[28], TokenType::Keyword, "int32") &&
           checkToken(tokens[29], TokenType::Keyword, "int64") &&
           checkToken(tokens[30], TokenType::Keyword, "int8") &&
           checkToken(tokens[31], TokenType::Keyword, "lambda") &&
           checkToken(tokens[32], TokenType::Keyword, "map") &&
           checkToken(tokens[33], TokenType::Keyword, "module") &&
           checkToken(tokens[34], TokenType::Keyword, "owns") &&
           checkToken(tokens[35], TokenType::Keyword, "ptr") &&
           checkToken(tokens[36], TokenType::Keyword, "public") &&
           checkToken(tokens[37], TokenType::Keyword, "readonly") &&
           checkToken(tokens[38], TokenType::Keyword, "repeat") &&
           checkToken(tokens[39], TokenType::Keyword, "return") &&
           checkToken(tokens[40], TokenType::Keyword, "set") &&
           checkToken(tokens[41], TokenType::Keyword, "str") &&
           checkToken(tokens[42], TokenType::Keyword, "switch") &&
           checkToken(tokens[43], TokenType::Keyword, "true") &&
           checkToken(tokens[44], TokenType::Keyword, "typeof") &&
           checkToken(tokens[45], TokenType::Keyword, "uint") &&
           checkToken(tokens[46], TokenType::Keyword, "uint8") &&
           checkToken(tokens[47], TokenType::Keyword, "uint16") &&
           checkToken(tokens[48], TokenType::Keyword, "uint32") &&
           checkToken(tokens[49], TokenType::Keyword, "uint64") &&
           checkToken(tokens[50], TokenType::Keyword, "vec") &&
           checkToken(tokens[51], TokenType::Keyword, "while") &&
           checkToken(tokens[52], TokenType::Identifier, "foo");
}

bool testIntegerLiterals() {
    auto tokens = tokensFromString("0 123 456789 0xFFF 0b1001 0o33");
    std::cout << "Tokens: ";
    for (const auto& token : tokens) {
        std::cout << token.getLexeme() << "";
    }
    std::cout << std::endl;
    if (tokens.size() != 6) {
        std::cout << "Expected 6 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    return checkToken(tokens[0], TokenType::Integer, "0") &&
           checkToken(tokens[1], TokenType::Integer, "123") &&
           checkToken(tokens[2], TokenType::Integer, "456789") &&
           checkToken(tokens[3], TokenType::Integer, "0xFFF") &&
           checkToken(tokens[4], TokenType::Integer, "0b1001") &&
           checkToken(tokens[5], TokenType::Integer, "0o33");
}

bool testFloatLiterals() {
    auto tokens = tokensFromString("0.0 1.23 456.789");
    std::cout << "Tokens: ";
    for (const auto& token : tokens) {
        std::cout << token.getLexeme() << " ";
    }
    std::cout << std::endl;
    if (tokens.size() != 3) {
        std::cout << "Expected 3 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    return checkToken(tokens[0], TokenType::Float, "0.0") &&
           checkToken(tokens[1], TokenType::Float, "1.23") &&
           checkToken(tokens[2], TokenType::Float, "456.789");
}

bool testCharLiterals() {
    auto tokens = tokensFromString("'a' '\\n' '\\'' '\\\\' '\\t' '\\u1234'");
    std::cout << "Tokens: ";
    for (const auto& token : tokens) {
        std::cout << token.getLexeme() << " ";
    }
    std::cout << std::endl;
    if (tokens.size() != 6) {
        std::cout << "Expected 6 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    return checkToken(tokens[0], TokenType::CharLiteral, "a") &&
           checkToken(tokens[1], TokenType::CharLiteral, "\n") &&
           checkToken(tokens[2], TokenType::CharLiteral, "\'") &&
           checkToken(tokens[3], TokenType::CharLiteral, "\\") &&
           checkToken(tokens[4], TokenType::CharLiteral, "\t") &&
           checkToken(tokens[5], TokenType::CharLiteral, "\u1234");
}

bool testStringLiterals() {
    auto tokens = tokensFromString("\"hello\" \"world\" \"escaped \\\"quote\\\"\"");
    std::cout << "Tokens: ";
    for (const auto& token : tokens) {
        std::cout << token.getLexeme() << " ";
    }
    std::cout << std::endl;
    if (tokens.size() != 3) {
        std::cout << "Expected 3 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    return checkToken(tokens[0], TokenType::StrLiteral, "hello") &&
           checkToken(tokens[1], TokenType::StrLiteral, "world") &&
           checkToken(tokens[2], TokenType::StrLiteral, "escaped \"quote\"");
}

bool testOperators() {
    auto tokens = tokensFromString("+ - * / // % ** ++ -- += -= *= /= //= %= **= == != && || ! & | ~ ^ &= |= ~= ^=  ? @ . : :: = -> ...");
    std::cout << "Tokens: ";
    for (const auto& token : tokens) {
        std::cout << token.getLexeme() << " ";
    }
    std::cout << std::endl;
    if (tokens.size() != 37) {
        std::cout << "Expected 37 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    return checkToken(tokens[0], TokenType::Operator, "+") &&
           checkToken(tokens[1], TokenType::Operator, "-") &&
           checkToken(tokens[2], TokenType::Operator, "*") &&
           checkToken(tokens[3], TokenType::Operator, "/") &&
           checkToken(tokens[4], TokenType::Operator, "//") &&
           checkToken(tokens[5], TokenType::Operator, "%") &&
           checkToken(tokens[6], TokenType::Operator, "**") &&
           checkToken(tokens[7], TokenType::Operator, "++") &&
           checkToken(tokens[8], TokenType::Operator, "--") &&
           checkToken(tokens[9], TokenType::Operator, "+=") &&
           checkToken(tokens[10], TokenType::Operator, "-=") &&
           checkToken(tokens[11], TokenType::Operator, "*=") &&
           checkToken(tokens[12], TokenType::Operator, "/=") &&
           checkToken(tokens[13], TokenType::Operator, "//=") &&
           checkToken(tokens[14], TokenType::Operator, "%=") &&
           checkToken(tokens[15], TokenType::Operator, "**=") &&
           checkToken(tokens[16], TokenType::Operator, "==") &&
           checkToken(tokens[17], TokenType::Operator, "!=") &&
           checkToken(tokens[18], TokenType::Operator, "&&") &&
           checkToken(tokens[19], TokenType::Operator, "||") &&
           checkToken(tokens[20], TokenType::Operator, "!") &&
           checkToken(tokens[21], TokenType::Operator, "&") &&
           checkToken(tokens[22], TokenType::Operator, "|") &&
           checkToken(tokens[23], TokenType::Operator, "~") &&
           checkToken(tokens[24], TokenType::Operator, "^") &&
           checkToken(tokens[25], TokenType::Operator, "&=") &&
           checkToken(tokens[26], TokenType::Operator, "|=") &&
           checkToken(tokens[27], TokenType::Operator, "~=") &&
           checkToken(tokens[28], TokenType::Operator, "^=") &&
           checkToken(tokens[29], TokenType::Operator, "?") &&
           checkToken(tokens[30], TokenType::Operator, "@") &&
           checkToken(tokens[31], TokenType::Operator, ".") &&
           checkToken(tokens[32], TokenType::Colon, ":") &&
           checkToken(tokens[33], TokenType::Operator, "::") &&
           checkToken(tokens[34], TokenType::Operator, "=") &&
           checkToken(tokens[35], TokenType::Operator, "->") &&
           checkToken(tokens[36], TokenType::Operator, "...");
}

bool testBrackets() {
    auto tokens = tokensFromString("( ) { } [ ] < >");
    std::cout << "Tokens: ";
    for (const auto& token : tokens) {
        std::cout << token.getLexeme() << " ";
    }
    std::cout << std::endl;
    if (tokens.size() != 8) {
        std::cout << "Expected 8 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    return checkToken(tokens[0], TokenType::LeftParen, "(") &&
           checkToken(tokens[1], TokenType::RightParen, ")") &&
           checkToken(tokens[2], TokenType::LeftBrace, "{") &&
           checkToken(tokens[3], TokenType::RightBrace, "}") &&
           checkToken(tokens[4], TokenType::LeftSquare, "[") &&
           checkToken(tokens[5], TokenType::RightSquare, "]") &&
           checkToken(tokens[6], TokenType::LeftAngle, "<") &&
           checkToken(tokens[7], TokenType::RightAngle, ">");
}

bool testPunctuation() {
    auto tokens = tokensFromString("; , . ? @ : ::");
    std::cout << "Tokens: ";
    for (const auto& token : tokens) {
        std::cout << token.getLexeme() << " ";
    }
    std::cout << std::endl;
    if (tokens.size() != 7) {
        std::cout << "Expected 7 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    return checkToken(tokens[0], TokenType::Semicolon, ";") &&
           checkToken(tokens[1], TokenType::Comma, ",") &&
           checkToken(tokens[2], TokenType::Operator, ".") &&
           checkToken(tokens[3], TokenType::Operator, "?") &&
           checkToken(tokens[4], TokenType::Operator, "@") &&
           checkToken(tokens[5], TokenType::Colon, ":") &&
           checkToken(tokens[6], TokenType::Operator, "::");
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

    auto tokens = tokensFromString(program);
    std::cout << "Tokens: ";
    for (const auto& token : tokens) {
        std::cout << token.getLexeme() << " ";
    }
    std::cout << std::endl;
    if (tokens.empty()) {
        std::cout << "Expected non-empty tokens" << std::endl;
        return false;
    }
    return checkToken(tokens[0], TokenType::Keyword, "func") &&
           checkToken(tokens[1], TokenType::Identifier, "main") &&
           checkToken(tokens[2], TokenType::LeftParen, "(") &&
           checkToken(tokens[3], TokenType::RightParen, ")") &&
           checkToken(tokens[4], TokenType::Operator, "->") &&
           checkToken(tokens[5], TokenType::Keyword, "int") &&
           checkToken(tokens[6], TokenType::LeftBrace, "{");
}

bool testNestedBrackets() {
    auto tokens = tokensFromString("arr<arr<int>> foo");
    std::cout << "Tokens: ";
    for (const auto& token : tokens) {
        std::cout << token.getLexeme() << " ";
    }
    std::cout << std::endl;
    if (tokens.size() != 8) {
        std::cout << "Expected 8 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    return checkToken(tokens[0], TokenType::Keyword, "arr") &&
           checkToken(tokens[1], TokenType::LeftAngle, "<") &&
           checkToken(tokens[2], TokenType::Keyword, "arr") &&
           checkToken(tokens[3], TokenType::LeftAngle, "<") &&
           checkToken(tokens[4], TokenType::Keyword, "int") &&
           checkToken(tokens[5], TokenType::RightAngle, ">") &&
           checkToken(tokens[6], TokenType::RightAngle, ">") &&
           checkToken(tokens[7], TokenType::Identifier, "foo");
}

bool testInvalidChar() {
    auto tokens = tokensFromString("'too long' '\\z' '\\u9Z99' ");
    if (tokens.size() != 3) {
        std::cout << "Expected 3 token, got " << tokens.size() << std::endl;
        return false;
    }

    return tokens[0].getType() == TokenType::Invalid &&
           tokens[1].getType() == TokenType::Invalid &&
           tokens[2].getType() == TokenType::Invalid;
}

bool testInvalidEscapeSequence() {
    auto tokens = tokensFromString("'\\z'");
    std::cout << "Tokens: ";
    for (const auto& token : tokens) {
        std::cout << token.getLexeme() << " ";
    }
    std::cout << std::endl;
    if (tokens.size() != 1) {
        std::cout << "Expected 1 token, got " << tokens.size() << std::endl;
        return false;
    }

    return tokens[0].getType() == TokenType::Invalid;
}

int runLexerTests() {
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