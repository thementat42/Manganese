#include <cassert>
#include <filesystem>
#include <frontend/lexer.hpp>
#include <io/logging.hpp>
#include <iostream>
#include <string>
#include <vector>

#include "testrunner.hpp"

namespace Manganese::tests {
using lexer::Token, lexer::TokenType;

// Helpers

namespace {

inline void printAllTokens(const std::vector<Token>& tokens, bool verbose = true) {
    if (tokens.empty()) {
        std::cout << "No tokens found." << '\n';
        return;
    }
    std::cout << "Tokens:\n" << CYAN;
    for (const Token& token : tokens) {
        if (verbose) {
            std::cout << token.toString() << "\n";
        } else {
            std::cout << token.getLexeme() << "\n";
        }
    }
    std::cout << RESET << '\n';
}

std::vector<Token> tokensFromString(const std::string& source) {
    lexer::Lexer lexer(source, lexer::Mode::String);
    std::vector<Token> tokens;

    // Consume tokens until we hit EOF
    while (true) {
        Token token = lexer.consumeToken();
        if (token.getType() == TokenType::EndOfFile) { break; }
        tokens.push_back(token);
    }

    return tokens;
}

std::vector<Token> tokensFromFile(const std::filesystem::path& filename) {
    std::filesystem::path fullPath = std::filesystem::current_path() / filename;
    lexer::Lexer lexer(fullPath.string(), lexer::Mode::File);
    std::vector<Token> tokens;

    // Consume tokens until we hit EOF
    while (true) {
        Token token = lexer.consumeToken();
        if (token.getType() == TokenType::EndOfFile) { break; }
        tokens.push_back(token);
    }

    return tokens;
}

bool checkToken(const Token& token, TokenType expectedType, const std::string& expectedLexeme,
                bool wantInvalid = false) {
    const static auto printBytes = [](std::string_view str) {
        for (char c : str) { std::cout << std::format("{:02X} ", (unsigned char)c); }
        std::cout << '\n';
    };
    if (token.getType() != expectedType) {
        std::cout << "Expected token type " << tokenTypeToString(expectedType) << " but got "
                  << tokenTypeToString(token.getType()) << " (lexeme was " << token.getLexeme() << ")" << '\n';
        return false;
    }
    if (token.getLexeme() != expectedLexeme) {
        std::cout << "Expected lexeme '" << expectedLexeme << "' but got '" << token.getLexeme() << "'" << '\n';

        std::cout << "Expected bytes: ";
        printBytes(expectedLexeme);

        std::cout << "Actual bytes:   ";
        printBytes(token.getLexeme());
        return false;
    }
    if (wantInvalid && !token.isInvalid()) {
        std::cout << "Expected token to be invalid, but was incorrectly classified as valid" << " (lexeme was "
                  << token.getLexeme() << ")" << '\n';
        return false;
    }
    return true;
}
}  // namespace

static bool testEmptyString() {
    std::vector<Token> tokens = tokensFromString("");
    printAllTokens(tokens);
    return tokens.empty();
}

static bool testWhitespace() {
    std::vector<Token> tokens = tokensFromString("  \t\n\r  ");
    printAllTokens(tokens);
    return tokens.empty();
}

static bool testComments() {
    std::vector<Token> tokens = tokensFromString("# This is a comment\nint x; /*This is\n a\n multiline comment!*/");
    printAllTokens(tokens);
    if (tokens.size() != 3) {
        std::cout << "Expected 3 tokens, got " << tokens.size() << '\n';
        return false;
    }

    return checkToken(tokens[0], TokenType::Int32, "int32") && checkToken(tokens[1], TokenType::Identifier, "x")
        && checkToken(tokens[2], TokenType::Semicolon, ";");
}

static bool testIdentifiers() {
    std::vector<Token> tokens = tokensFromString("foo bar baz _var var123");
    printAllTokens(tokens);
    if (tokens.size() != 5) {
        std::cout << "Expected 5 tokens, got " << tokens.size() << '\n';
        return false;
    }

    return checkToken(tokens[0], TokenType::Identifier, "foo") && checkToken(tokens[1], TokenType::Identifier, "bar")
        && checkToken(tokens[2], TokenType::Identifier, "baz") && checkToken(tokens[3], TokenType::Identifier, "_var")
        && checkToken(tokens[4], TokenType::Identifier, "var123");
}

static bool testKeywords() {
    std::vector<Token> tokens
        = tokensFromString("alias as uint128 bool break aggregate case char mut foo while string");

    printAllTokens(tokens);
    if (tokens.size() != 12) {
        std::cout << "Expected 12 tokens, got " << tokens.size() << '\n';
        return false;
    }

    return checkToken(tokens[0], TokenType::Alias, "alias") && checkToken(tokens[1], TokenType::As, "as")
        && checkToken(tokens[2], TokenType::UInt128, "uint128") && checkToken(tokens[3], TokenType::Bool, "bool")
        && checkToken(tokens[4], TokenType::Break, "break") && checkToken(tokens[5], TokenType::Aggregate, "aggregate")
        && checkToken(tokens[6], TokenType::Case, "case") && checkToken(tokens[7], TokenType::Char, "char")
        && checkToken(tokens[8], TokenType::Mut, "mut") && checkToken(tokens[9], TokenType::Identifier, "foo")
        && checkToken(tokens[10], TokenType::While, "while") && checkToken(tokens[11], TokenType::String, "string");
}

static bool testIntegerLiterals() {
    std::vector<Token> tokens = tokensFromString("0 123 123u64 456789i8 1i128 "
                                                 "0xFFF 0xABCD_DEFE "
                                                 "0b1001 0b1010_0101 "
                                                 "0o33 0o755");

    printAllTokens(tokens);

    if (tokens.size() != 11) {
        std::cout << "Expected 11 tokens, got " << tokens.size() << '\n';
        return false;
    }

    return checkToken(tokens[0], TokenType::IntegerLiteral, "0")
        && checkToken(tokens[1], TokenType::IntegerLiteral, "123")
        && checkToken(tokens[2], TokenType::IntegerLiteral, "123u64")
        && checkToken(tokens[3], TokenType::IntegerLiteral, "456789i8")
        && checkToken(tokens[4], TokenType::IntegerLiteral, "1i128")
        && checkToken(tokens[5], TokenType::IntegerLiteral, "0xFFF")
        && checkToken(tokens[6], TokenType::IntegerLiteral, "0xABCDDEFE")
        && checkToken(tokens[7], TokenType::IntegerLiteral, "0b1001")
        && checkToken(tokens[8], TokenType::IntegerLiteral, "0b10100101")
        && checkToken(tokens[9], TokenType::IntegerLiteral, "0o33")
        && checkToken(tokens[10], TokenType::IntegerLiteral, "0o755");
}

static bool testFloatLiterals() {
    std::vector<Token> tokens = tokensFromString("0.0 0.0f32 1.23f64 456.789 "
                                                 "1.23e4 1.23e-4 1.23e+4 "
                                                 "1.23e4f32 1.23e4f64");

    printAllTokens(tokens);

    if (tokens.size() != 9) {
        std::cout << "Expected 9 tokens, got " << tokens.size() << '\n';
        return false;
    }

    return checkToken(tokens[0], TokenType::FloatLiteral, "0.0")
        && checkToken(tokens[1], TokenType::FloatLiteral, "0.0f32")
        && checkToken(tokens[2], TokenType::FloatLiteral, "1.23f64")
        && checkToken(tokens[3], TokenType::FloatLiteral, "456.789")
        && checkToken(tokens[4], TokenType::FloatLiteral, "1.23e4")
        && checkToken(tokens[5], TokenType::FloatLiteral, "1.23e-4")
        && checkToken(tokens[6], TokenType::FloatLiteral, "1.23e+4")
        && checkToken(tokens[7], TokenType::FloatLiteral, "1.23e4f32")
        && checkToken(tokens[8], TokenType::FloatLiteral, "1.23e4f64");
}

static bool testInvalidNumberLiterals() {
    std::vector<Token> tokens = tokensFromString("0x1.23 "
                                                 "1.2.3 "
                                                 "1e "
                                                 "1e+ "
                                                 "1e- "
                                                 "123f32 "
                                                 "123i16 "
                                                 "1.5i32 "
                                                 "1.5f128 "
                                                 "0b102 "
                                                 "0o789");

    printAllTokens(tokens);

    if (tokens.size() != 11) {
        std::cout << "Expected 11 tokens, got " << tokens.size() << '\n';
        return false;
    }

    return checkToken(tokens[0], TokenType::IntegerLiteral, "0x123", true)
        && checkToken(tokens[1], TokenType::FloatLiteral, "1.23", true)
        && checkToken(tokens[2], TokenType::FloatLiteral, "1e", true)
        && checkToken(tokens[3], TokenType::FloatLiteral, "1e+", true)
        && checkToken(tokens[4], TokenType::FloatLiteral, "1e-", true)
        && checkToken(tokens[5], TokenType::IntegerLiteral, "123f32", true)
        && checkToken(tokens[6], TokenType::IntegerLiteral, "123i16")
        && checkToken(tokens[7], TokenType::FloatLiteral, "1.5i32", true)
        && checkToken(tokens[8], TokenType::FloatLiteral, "1.5f128", true)
        && checkToken(tokens[9], TokenType::IntegerLiteral, "0b10", true)
        && checkToken(tokens[10], TokenType::IntegerLiteral, "0o7", true);
}

static bool testCharLiterals() {
    const std::vector<Token> tokens = tokensFromString("'a' "
                                                       "'\\n' "
                                                       "'\\'' "
                                                       "'\\\\' "
                                                       "'\\t' "
                                                       "'\\u1234' "
                                                       "'λ' "
                                                       "'😀'");

    printAllTokens(tokens);

    if (tokens.size() != 8) {
        std::cout << "Expected 8 tokens, got " << tokens.size() << '\n';
        return false;
    }

    const std::string expected1234{static_cast<char>(0xE1), static_cast<char>(0x88), static_cast<char>(0xB4)};
    const std::string expectedEmoji{static_cast<char>(0xF0), static_cast<char>(0x9F), static_cast<char>(0x98),
                                    static_cast<char>(0x80)};

    return checkToken(tokens[0], TokenType::CharLiteral, "a") && checkToken(tokens[1], TokenType::CharLiteral, "\n")
        && checkToken(tokens[2], TokenType::CharLiteral, "'") && checkToken(tokens[3], TokenType::CharLiteral, "\\")
        && checkToken(tokens[4], TokenType::CharLiteral, "\t")
        && checkToken(tokens[5], TokenType::CharLiteral, expected1234)  // U+1234
        && checkToken(tokens[6], TokenType::CharLiteral, "\xCE\xBB")  // U+03BB
        && checkToken(tokens[7], TokenType::CharLiteral, expectedEmoji);
}

static bool testStringLiterals() {
    const std::vector<Token> tokens = tokensFromString("\"hello\" "
                                                       "\"world\" "
                                                       "\"escaped \\\"quote\\\"\" "
                                                       "\"newline\\nindent\\ttab\" "
                                                       "\"unicode \\u1234\" "
                                                       "\"emoji \\u0001F600\" "
                                                       "\"literal λ 😀\"");

    printAllTokens(tokens);

    if (tokens.size() != 7) {
        std::cout << "Expected 7 tokens, got " << tokens.size() << '\n';
        return false;
    }

    return checkToken(tokens[0], TokenType::StrLiteral, "hello")
        && checkToken(tokens[1], TokenType::StrLiteral, "world")
        && checkToken(tokens[2], TokenType::StrLiteral, "escaped \"quote\"")
        && checkToken(tokens[3], TokenType::StrLiteral, "newline\nindent\ttab")
        && checkToken(tokens[4], TokenType::StrLiteral, "unicode \xE1\x88\xB4")
        && checkToken(tokens[5], TokenType::StrLiteral, "emoji \xF0\x9F\x98\x80")
        && checkToken(tokens[6], TokenType::StrLiteral, "literal \xCE\xBB \xF0\x9F\x98\x80");
}

static bool testInvalidCharLiterals() {
    const std::vector<Token> tokens = tokensFromString("'' "
                                                       "'ab' "
                                                       "'\\x' "
                                                       "'\\u123' "
                                                       "'\\u12345' "
                                                       "'\\uD800' "
                                                       "'\\u110000' "
                                                       "'\\u123456789' "
                                                       "'\\xFF'");

    printAllTokens(tokens);

    if (tokens.size() != 9) {
        std::cout << "Expected 9 tokens, got " << tokens.size() << '\n';
        return false;
    }

    return checkToken(tokens[0], TokenType::CharLiteral, "", true)
        && checkToken(tokens[1], TokenType::CharLiteral, "ab", true)
        && checkToken(tokens[2], TokenType::CharLiteral, "\\x", true)
        && checkToken(tokens[3], TokenType::CharLiteral, "\\u123", true)
        && checkToken(tokens[4], TokenType::CharLiteral, "\\u12345", true)
        && checkToken(tokens[5], TokenType::CharLiteral, "\\uD800", true)
        && checkToken(tokens[6], TokenType::CharLiteral, "\\u110000", true)
        && checkToken(tokens[7], TokenType::CharLiteral, "\\u123456789", true)
        && checkToken(tokens[8], TokenType::CharLiteral, "\\xFF", true);
}

static bool testInvalidStringLiterals() {
    const std::vector<Token> tokens = tokensFromString("\"unterminated\\n\" "
                                                       "\"bad escape \\x\" "
                                                       "\"bad unicode \\u123\" "
                                                       "\"bad unicode \\uD800\" "
                                                       "\"bad unicode \\u110000\" "
                                                       "\"bad unicode \\u123456789\"");

    printAllTokens(tokens);

    if (tokens.size() != 6) {
        std::cout << "Expected 6 tokens, got " << tokens.size() << '\n';
        return false;
    }

    return checkToken(tokens[0], TokenType::StrLiteral, "unterminated\n", false)
        && checkToken(tokens[1], TokenType::StrLiteral, "bad escape \\x", true)
        && checkToken(tokens[2], TokenType::StrLiteral, "bad unicode \\u123", true)
        && checkToken(tokens[3], TokenType::StrLiteral, "bad unicode \\uD800", true)
        && checkToken(tokens[4], TokenType::StrLiteral, "bad unicode \\u110000", true)
        && checkToken(tokens[5], TokenType::StrLiteral, "bad unicode \\u123456789", true);
}

static bool testOperators() {
    std::vector<Token> tokens = tokensFromString(
        "+ - * / // % ++ -- += -= *= /= //= %= == != && || ! & | ~ ^ &= |= ... ^= . : :: = -> ... @ < <= > >= << >> <<= >>=");

    printAllTokens(tokens);
    if (tokens.size() != 42) {
        std::cout << "Expected 44 tokens, got " << tokens.size() << '\n';
        return false;
    }

    return checkToken(tokens[0], TokenType::Plus, "+") && checkToken(tokens[1], TokenType::Minus, "-")
        && checkToken(tokens[2], TokenType::Mul, "*") && checkToken(tokens[3], TokenType::Div, "/")
        && checkToken(tokens[4], TokenType::FloorDiv, "//") && checkToken(tokens[5], TokenType::Mod, "%")
        && checkToken(tokens[6], TokenType::Inc, "++") && checkToken(tokens[7], TokenType::Dec, "--")
        && checkToken(tokens[8], TokenType::PlusAssign, "+=") && checkToken(tokens[9], TokenType::MinusAssign, "-=")
        && checkToken(tokens[10], TokenType::MulAssign, "*=") && checkToken(tokens[11], TokenType::DivAssign, "/=")
        && checkToken(tokens[12], TokenType::FloorDivAssign, "//=")
        && checkToken(tokens[13], TokenType::ModAssign, "%=") && checkToken(tokens[14], TokenType::Equal, "==")
        && checkToken(tokens[15], TokenType::NotEqual, "!=") && checkToken(tokens[16], TokenType::And, "&&")
        && checkToken(tokens[17], TokenType::Or, "||") && checkToken(tokens[18], TokenType::Not, "!")
        && checkToken(tokens[19], TokenType::BitAnd, "&") && checkToken(tokens[20], TokenType::BitOr, "|")
        && checkToken(tokens[21], TokenType::BitNot, "~") && checkToken(tokens[22], TokenType::BitXor, "^")
        && checkToken(tokens[23], TokenType::BitAndAssign, "&=") && checkToken(tokens[24], TokenType::BitOrAssign, "|=")
        && checkToken(tokens[25], TokenType::Ellipsis, "...")
        && checkToken(tokens[26], TokenType::BitXorAssign, "^=") && checkToken(tokens[27], TokenType::MemberAccess, ".")
        && checkToken(tokens[28], TokenType::Colon, ":") && checkToken(tokens[29], TokenType::ScopeResolution, "::")
        && checkToken(tokens[30], TokenType::Assignment, "=") && checkToken(tokens[31], TokenType::Arrow, "->")
        && checkToken(tokens[32], TokenType::Ellipsis, "...") && checkToken(tokens[33], TokenType::At, "@")
        && checkToken(tokens[34], TokenType::LessThan, "<") && checkToken(tokens[35], TokenType::LessThanOrEqual, "<=")
        && checkToken(tokens[36], TokenType::GreaterThan, ">")
        && checkToken(tokens[37], TokenType::GreaterThanOrEqual, ">=")
        && checkToken(tokens[38], TokenType::BitLShift, "<<") && checkToken(tokens[39], TokenType::BitRShift, ">>")
        && checkToken(tokens[40], TokenType::BitLShiftAssign, "<<=")
        && checkToken(tokens[41], TokenType::BitRShiftAssign, ">>=");
}

static bool testBrackets() {
    std::vector<Token> tokens = tokensFromString("( ) { } [ ]");
    printAllTokens(tokens);
    if (tokens.size() != 6) {
        std::cout << "Expected 6 tokens, got " << tokens.size() << '\n';
        return false;
    }

    return checkToken(tokens[0], TokenType::LeftParen, "(") && checkToken(tokens[1], TokenType::RightParen, ")")
        && checkToken(tokens[2], TokenType::LeftBrace, "{") && checkToken(tokens[3], TokenType::RightBrace, "}")
        && checkToken(tokens[4], TokenType::LeftSquare, "[") && checkToken(tokens[5], TokenType::RightSquare, "]");
}

static bool testPunctuation() {
    std::vector<Token> tokens = tokensFromString("; , . : ::");
    printAllTokens(tokens);
    if (tokens.size() != 5) {
        std::cout << "Expected 5 tokens, got " << tokens.size() << '\n';
        return false;
    }

    return checkToken(tokens[0], TokenType::Semicolon, ";") && checkToken(tokens[1], TokenType::Comma, ",")
        && checkToken(tokens[2], TokenType::MemberAccess, ".") && checkToken(tokens[3], TokenType::Colon, ":")
        && checkToken(tokens[4], TokenType::ScopeResolution, "::");
}

static bool testCompleteProgram() {
    std::vector<Token> tokens = tokensFromFile("tests/lexer_tests.mn");
    printAllTokens(tokens);
    if (tokens.empty()) {
        std::cout << "Expected non-empty tokens" << '\n';
        return false;
    }
    return checkToken(tokens[0], TokenType::Func, "func") && checkToken(tokens[1], TokenType::Identifier, "main")
        && checkToken(tokens[2], TokenType::LeftParen, "(") && checkToken(tokens[3], TokenType::RightParen, ")")
        && checkToken(tokens[4], TokenType::Arrow, "->") && checkToken(tokens[5], TokenType::Int32, "int32")
        && checkToken(tokens[6], TokenType::LeftBrace, "{") && checkToken(tokens[7], TokenType::Int32, "int32")
        && checkToken(tokens[8], TokenType::Identifier, "x") && checkToken(tokens[9], TokenType::Assignment, "=")
        && checkToken(tokens[10], TokenType::IntegerLiteral, "5") && checkToken(tokens[11], TokenType::Semicolon, ";")
        && checkToken(tokens[12], TokenType::Float32, "float32") && checkToken(tokens[13], TokenType::Identifier, "y")
        && checkToken(tokens[14], TokenType::Assignment, "=") && checkToken(tokens[15], TokenType::FloatLiteral, "10.5")
        && checkToken(tokens[16], TokenType::Semicolon, ";") && checkToken(tokens[17], TokenType::Identifier, "print")
        && checkToken(tokens[18], TokenType::LeftParen, "(") && checkToken(tokens[19], TokenType::Identifier, "x")
        && checkToken(tokens[20], TokenType::RightParen, ")") && checkToken(tokens[21], TokenType::Semicolon, ";")
        && checkToken(tokens[22], TokenType::Identifier, "print") && checkToken(tokens[23], TokenType::LeftParen, "(")
        && checkToken(tokens[24], TokenType::Identifier, "y") && checkToken(tokens[25], TokenType::RightParen, ")")
        && checkToken(tokens[26], TokenType::Semicolon, ";") && checkToken(tokens[27], TokenType::RightBrace, "}");
}

static bool testNestedBrackets() {
    std::vector<Token> tokens = tokensFromString("arr@[arr@[int16]] foo");
    printAllTokens(tokens);
    if (tokens.size() != 10) {
        std::cout << "Expected 10 tokens, got " << tokens.size() << '\n';
        return false;
    }

    return checkToken(tokens[0], TokenType::Identifier, "arr") && checkToken(tokens[1], TokenType::At, "@")
        && checkToken(tokens[2], TokenType::LeftSquare, "[") && checkToken(tokens[3], TokenType::Identifier, "arr")
        && checkToken(tokens[4], TokenType::At, "@") && checkToken(tokens[5], TokenType::LeftSquare, "[")
        && checkToken(tokens[6], TokenType::Int16, "int16") && checkToken(tokens[7], TokenType::RightSquare, "]")
        && checkToken(tokens[8], TokenType::RightSquare, "]") && checkToken(tokens[9], TokenType::Identifier, "foo");
}

static bool testInvalidChar() {
    std::vector<Token> tokens = tokensFromString("'too long' '\\z' '\\u9Z99' ");
    printAllTokens(tokens);
    if (tokens.size() != 3) {
        std::cout << "Expected 3 token, got " << tokens.size() << '\n';
        return false;
    }

    return tokens[0].getType() == TokenType::CharLiteral && tokens[0].isInvalid()
        && tokens[1].getType() == TokenType::CharLiteral && tokens[1].isInvalid()
        && tokens[2].getType() == TokenType::CharLiteral && tokens[2].isInvalid();
}

static bool testInvalidEscapeSequence() {
    std::vector<Token> tokens = tokensFromString("'\\z'");
    printAllTokens(tokens);
    if (tokens.size() != 1) {
        std::cout << "Expected 1 token, got " << tokens.size() << '\n';
        return false;
    }

    return tokens[0].getType() == TokenType::CharLiteral;
}

static bool testBadFileAccess() {
    try {
        tokensFromFile("__nonexistentfile.mn");
    } catch (const std::runtime_error& e) { return true; }
    return false;
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
    runner.runTest("Invalid Number Literals", testInvalidNumberLiterals);
    runner.runTest("Character Literals", testCharLiterals);
    runner.runTest("String Literals", testStringLiterals);
    runner.runTest("Invalid Character Literals", testInvalidCharLiterals);
    runner.runTest("Invalid String Literals", testInvalidStringLiterals);
    runner.runTest("Brackets", testBrackets);
    runner.runTest("Punctuation", testPunctuation);
    runner.runTest("Nested Brackets", testNestedBrackets);
    runner.runTest("Invalid Character", testInvalidChar);
    runner.runTest("Invalid Escape Sequence", testInvalidEscapeSequence);
    runner.runTest("Complete Program", testCompleteProgram);
    runner.runTest("Invalid File", testBadFileAccess);
}
}  // namespace Manganese::tests
