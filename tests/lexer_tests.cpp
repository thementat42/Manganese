#include <cassert>
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "../src/frontend/include/lexer.h"
#include "../src/frontend/include/token.h"
#include "../src/global_macros.h"
#include "testrunner.h"
#include "../src/io/include/logging.h"

// TODO: Replace this with purely file-based tests

MANGANESE_BEGIN
namespace tests {
using lexer::Lexer;
using lexer::Mode;
using lexer::Token;
using TokenList = std::vector<Token>;
using lexer::TokenType;
using lexer::tokenTypeToString;

// Utility function to print tokens in cyan color
inline void printAllTokens(const std::vector<Token>& tokens) {
    if (tokens.empty()) {
        std::cout << "No tokens found." << std::endl;
        return;
    }
    std::cout << "Tokens: " << CYAN;
    for (const auto& token : tokens) {
        std::cout << token.getLexeme() << " ";
    }
    std::cout << RESET << std::endl;
}

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

std::vector<Token> tokensFromFile(const std::filesystem::path& filename) {
    std::filesystem::path fullPath = std::filesystem::current_path() / filename;
    Lexer lexer(fullPath.string(), Mode::File);
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
        std::cout << "Expected token type " << tokenTypeToString(expectedType)
                  << " but got " << tokenTypeToString(token.getType()) << " (lexeme was " << token.getLexeme() << ")" << std::endl;
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
    printAllTokens(tokens);
    return tokens.empty();
}

bool testWhitespace() {
    auto tokens = tokensFromString("  \t\n\r  ");
    printAllTokens(tokens);
    return tokens.empty();
}

bool testComments() {
    auto tokens = tokensFromString("# This is a comment\nint x; /*This is\n a\n multiline comment!*/");
    printAllTokens(tokens);
    if (tokens.size() != 3) {
        std::cout << "Expected 3 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    return checkToken(tokens[0], TokenType::Int32, "int32") &&
           checkToken(tokens[1], TokenType::Identifier, "x") &&
           checkToken(tokens[2], TokenType::Semicolon, ";");
}

bool testIdentifiers() {
    auto tokens = tokensFromString("foo bar baz _var var123");
    printAllTokens(tokens);
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
    auto tokens = tokensFromString("alias as blueprint bool break bundle case cast char const foo");
    printAllTokens(tokens);
    if (tokens.size() != 11) {
        std::cout << "Expected 11 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    return checkToken(tokens[0], TokenType::Alias, "alias") &&
           checkToken(tokens[1], TokenType::As, "as") &&
           checkToken(tokens[2], TokenType::Blueprint, "blueprint") &&
           checkToken(tokens[3], TokenType::Bool, "bool") &&
           checkToken(tokens[4], TokenType::Break, "break") &&
           checkToken(tokens[5], TokenType::Bundle, "bundle") &&
           checkToken(tokens[6], TokenType::Case, "case") &&
           checkToken(tokens[7], TokenType::Cast, "cast") &&
           checkToken(tokens[8], TokenType::Char, "char") &&
           checkToken(tokens[9], TokenType::Const, "const") &&
           checkToken(tokens[10], TokenType::Identifier, "foo");
}

bool testIntegerLiterals() {
    auto tokens = tokensFromString("0 123 456789 0xFFF 0b1001 0o33 0x1.23p4 1.23e-4");
    printAllTokens(tokens);
    if (tokens.size() != 8) {
        std::cout << "Expected 8 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    return checkToken(tokens[0], TokenType::IntegerLiteral, "0") &&
           checkToken(tokens[1], TokenType::IntegerLiteral, "123") &&
           checkToken(tokens[2], TokenType::IntegerLiteral, "456789") &&
           checkToken(tokens[3], TokenType::IntegerLiteral, "0xFFF") &&
           checkToken(tokens[4], TokenType::IntegerLiteral, "0b1001") &&
           checkToken(tokens[5], TokenType::IntegerLiteral, "0o33") &&
           checkToken(tokens[6], TokenType::FloatLiteral, "0x1.23p4") &&
           checkToken(tokens[7], TokenType::FloatLiteral, "1.23e-4");
}

bool testFloatLiterals() {
    auto tokens = tokensFromString("0.0 1.23 456.789");
    printAllTokens(tokens);
    if (tokens.size() != 3) {
        std::cout << "Expected 3 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    return checkToken(tokens[0], TokenType::FloatLiteral, "0.0") &&
           checkToken(tokens[1], TokenType::FloatLiteral, "1.23") &&
           checkToken(tokens[2], TokenType::FloatLiteral, "456.789");
}

bool testCharLiterals() {
    auto tokens = tokensFromString("'a' '\\n' '\\'' '\\\\' '\\t' '\\u1234'");
    printAllTokens(tokens);
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
    printAllTokens(tokens);
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
    printAllTokens(tokens);
    if (tokens.size() != 37) {
        std::cout << "Expected 37 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    return checkToken(tokens[0], TokenType::Plus, "+") &&
           checkToken(tokens[1], TokenType::Minus, "-") &&
           checkToken(tokens[2], TokenType::Mul, "*") &&
           checkToken(tokens[3], TokenType::Div, "/") &&
           checkToken(tokens[4], TokenType::FloorDiv, "//") &&
           checkToken(tokens[5], TokenType::Mod, "%") &&
           checkToken(tokens[6], TokenType::Exp, "**") &&
           checkToken(tokens[7], TokenType::Inc, "++") &&
           checkToken(tokens[8], TokenType::Dec, "--") &&
           checkToken(tokens[9], TokenType::PlusAssign, "+=") &&
           checkToken(tokens[10], TokenType::MinusAssign, "-=") &&
           checkToken(tokens[11], TokenType::MulAssign, "*=") &&
           checkToken(tokens[12], TokenType::DivAssign, "/=") &&
           checkToken(tokens[13], TokenType::FloorDivAssign, "//=") &&
           checkToken(tokens[14], TokenType::ModAssign, "%=") &&
           checkToken(tokens[15], TokenType::ExpAssign, "**=") &&
           checkToken(tokens[16], TokenType::Equal, "==") &&
           checkToken(tokens[17], TokenType::NotEqual, "!=") &&
           checkToken(tokens[18], TokenType::And, "&&") &&
           checkToken(tokens[19], TokenType::Or, "||") &&
           checkToken(tokens[20], TokenType::Not, "!") &&
           checkToken(tokens[21], TokenType::BitAnd, "&") &&
           checkToken(tokens[22], TokenType::BitOr, "|") &&
           checkToken(tokens[23], TokenType::BitNot, "~") &&
           checkToken(tokens[24], TokenType::BitXor, "^") &&
           checkToken(tokens[25], TokenType::BitAndAssign, "&=") &&
           checkToken(tokens[26], TokenType::BitOrAssign, "|=") &&
           checkToken(tokens[27], TokenType::BitNotAssign, "~=") &&
           checkToken(tokens[28], TokenType::BitXorAssign, "^=") &&
           checkToken(tokens[29], TokenType::AddressOf, "?") &&
           checkToken(tokens[30], TokenType::Dereference, "@") &&
           checkToken(tokens[31], TokenType::MemberAccess, ".") &&
           checkToken(tokens[32], TokenType::Colon, ":") &&
           checkToken(tokens[33], TokenType::ScopeResolution, "::") &&
           checkToken(tokens[34], TokenType::Assignment, "=") &&
           checkToken(tokens[35], TokenType::Arrow, "->") &&
           checkToken(tokens[36], TokenType::Ellipsis, "...");
}

bool testBrackets() {
    auto tokens = tokensFromString("( ) { } [ ] < >");
    printAllTokens(tokens);
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
    printAllTokens(tokens);
    if (tokens.size() != 7) {
        std::cout << "Expected 7 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    return checkToken(tokens[0], TokenType::Semicolon, ";") &&
           checkToken(tokens[1], TokenType::Comma, ",") &&
           checkToken(tokens[2], TokenType::MemberAccess, ".") &&
           checkToken(tokens[3], TokenType::AddressOf, "?") &&
           checkToken(tokens[4], TokenType::Dereference, "@") &&
           checkToken(tokens[5], TokenType::Colon, ":") &&
           checkToken(tokens[6], TokenType::ScopeResolution, "::");
}

bool testCompleteProgram() {
    auto tokens = tokensFromFile("tests/lexer_tests.mn");
    printAllTokens(tokens);
    if (tokens.empty()) {
        std::cout << "Expected non-empty tokens" << std::endl;
        return false;
    }
    return checkToken(tokens[0], TokenType::Func, "func") &&
           checkToken(tokens[1], TokenType::Identifier, "main") &&
           checkToken(tokens[2], TokenType::LeftParen, "(") &&
           checkToken(tokens[3], TokenType::RightParen, ")") &&
           checkToken(tokens[4], TokenType::Arrow, "->") &&
           checkToken(tokens[5], TokenType::Int32, "int32") &&
           checkToken(tokens[6], TokenType::LeftBrace, "{") &&
           checkToken(tokens[7], TokenType::Int32, "int32") &&
           checkToken(tokens[8], TokenType::Identifier, "x") &&
           checkToken(tokens[9], TokenType::Assignment, "=") &&
           checkToken(tokens[10], TokenType::IntegerLiteral, "5") &&
           checkToken(tokens[11], TokenType::Semicolon, ";") &&
           checkToken(tokens[12], TokenType::Float32, "float32") &&
           checkToken(tokens[13], TokenType::Identifier, "y") &&
           checkToken(tokens[14], TokenType::Assignment, "=") &&
           checkToken(tokens[15], TokenType::FloatLiteral, "10.5") &&
           checkToken(tokens[16], TokenType::Semicolon, ";") &&
           checkToken(tokens[17], TokenType::Identifier, "print") &&
           checkToken(tokens[18], TokenType::LeftParen, "(") &&
           checkToken(tokens[19], TokenType::Identifier, "x") &&
           checkToken(tokens[20], TokenType::RightParen, ")") &&
           checkToken(tokens[21], TokenType::Semicolon, ";") &&
           checkToken(tokens[22], TokenType::Identifier, "print") &&
           checkToken(tokens[23], TokenType::LeftParen, "(") &&
           checkToken(tokens[24], TokenType::Identifier, "y") &&
           checkToken(tokens[25], TokenType::RightParen, ")") &&
           checkToken(tokens[26], TokenType::Semicolon, ";") &&
           checkToken(tokens[27], TokenType::RightBrace, "}");
}

bool testNestedBrackets() {
    auto tokens = tokensFromString("arr<arr<int16>> foo");
    printAllTokens(tokens);
    if (tokens.size() != 8) {
        std::cout << "Expected 8 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    return checkToken(tokens[0], TokenType::Identifier, "arr") &&
           checkToken(tokens[1], TokenType::LeftAngle, "<") &&
           checkToken(tokens[2], TokenType::Identifier, "arr") &&
           checkToken(tokens[3], TokenType::LeftAngle, "<") &&
           checkToken(tokens[4], TokenType::Int16, "int16") &&
           checkToken(tokens[5], TokenType::RightAngle, ">") &&
           checkToken(tokens[6], TokenType::RightAngle, ">") &&
           checkToken(tokens[7], TokenType::Identifier, "foo");
}

bool testInvalidChar() {
    auto tokens = tokensFromString("'too long' '\\z' '\\u9Z99' ");
    printAllTokens(tokens);
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
    printAllTokens(tokens);
    if (tokens.size() != 1) {
        std::cout << "Expected 1 token, got " << tokens.size() << std::endl;
        return false;
    }

    return tokens[0].getType() == TokenType::Invalid;
}

void runLexerTests(TestRunner& runner) {
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
    runner.runTest("Nested Brackets", testNestedBrackets);
    runner.runTest("Invalid Character", testInvalidChar);
    runner.runTest("Invalid Escape Sequence", testInvalidEscapeSequence);
    runner.runTest("Complete Program", testCompleteProgram);
}
}  // namespace tests
MANGANESE_END
