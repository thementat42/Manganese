

/**
 * @file lexer.cpp
 * @brief This file contains the implementation of the lexer for the Manganese compiler.
 *
 * The lexer takes the source code and splits it into tokens (see token.h)
 * These tokens are then passed on to the parser to build the AST.
 *
 * @note: all angle brackets are tokenized as angle brackets, even if they are actually comparison or bitwise shift operators.
 * This is because determining which operator it is would require looking ahead in the source code, which is not possible in a single-pass lexer.
 * The parser will handle this ambiguity and determine the correct operator.
 *
 * @note: The lexer strips out comments and whitespace -- the parser never sees this
 *
 * @note: The main loop does not advance the reader position, it just peeks the current character. Each specific tokenization function should advance the reader position once its token has been generated.
 *  E.g. the string tokenizing function will advance the reader past the quotes, the operator tokenizing function will advance the reader past the operator, etc.
 */

#include "include/lexer.h"

#include <algorithm>
#include <functional>

#include "../global_macros.h"
#include "../io/include/filereader.h"
#include "../io/include/reader.h"
#include "../io/include/stringreader.h"
#include "../core/include/keywords.h"
#include "../core/include/operators.h"
#include "../core/include/token.h"

MANG_BEGIN
namespace lexer {
Lexer::Lexer(const str& source, const Mode mode) {
    switch (mode) {
        case Mode::String:
            reader = std::make_unique<io::StringReader>(source);
            break;
        case Mode::File:
            reader = std::make_unique<io::FileReader>(source);
            break;
    }
    tokenStream = std::deque<Token>();
    isTokenizingDone = false;
}

static optional<wchar_t> resolveHexAndUnicodeCharacters(const str& esc, const bool& isUnicode, size_t& skipLength) {
    // Length is checked in caller (when passing in substring)
    const char* sequenceType = isUnicode ? "unicode" : "hex";
    auto isNotHex = [](char c) { return !std::isxdigit(static_cast<unsigned char>(c)); };
    auto x = std::find_if(esc.begin(), esc.begin() + (isUnicode ? 4 : 2), isNotHex);
    if (x != esc.end()) {
        fprintf(stderr,
                "Error: Invalid %s escape sequence: \\%c%s\n",
                sequenceType, isUnicode ? 'u' : 'x', esc.c_str());
        return NONE;
    }
    wchar_t unicodeChar = 0;
    if (isUnicode) {
        // Convert to unicode character
        for (size_t i = 0; i < 4; ++i) {
            unicodeChar <<= 4;
            unicodeChar |= static_cast<wchar_t>(std::stoi(std::string(1, esc[i]), nullptr, 16));
        }
        skipLength = 4;
    } else {
        // Convert to hex character
        for (size_t i = 0; i < 2; ++i) {
            unicodeChar <<= 4;
            unicodeChar |= static_cast<wchar_t>(std::stoi(esc.substr(i, 1), nullptr, 16));
        }
        skipLength = 2;
    }
    return unicodeChar;
}

static inline optional<wchar_t> getEscapeCharacter(const char& escapeChar) {
    // This is a separate function mainly for readability (hence inline)
    switch (escapeChar) {
        case '\\':
            return '\\';
        case '\'':
            return '\'';
        case '\"':
            return '\"';
        case 'a':
            return '\a';
        case 'b':
            return '\b';
        case 'f':
            return '\f';
        case 'n':
            return '\n';
        case 'r':
            return '\r';
        case 't':
            return '\t';
        case 'v':
            return '\v';
        case '0':
            return '\0';
        default:
            fprintf(stderr,
                    "\\%c is not a valid escape sequence. If you meant to type a backslash ('\\'), use two backslashes ('\\\\%c')\n",
                    escapeChar, escapeChar);
            return NONE;
    }
}

static inline std::string convertWideCharToUTF8(uint32_t wideChar) {
    std::string result;
    if (wideChar <= UTF8_1B_MAX) {
        result += static_cast<char>(wideChar);  // Narrow character
    } else if (wideChar <= UTF8_2B_MAX) {
        // 2-byte UTF-8 character
        result += static_cast<char>(UTF8_2B_PRE | ((wideChar >> UTF8_CONT_SHIFT) & UTF8_2B_MASK));
        result += static_cast<char>(UTF8_CONT_PRE | (wideChar & UTF8_CONT_MASK));
    } else if (wideChar <= UTF8_3B_MAX) {
        // 3-byte UTF-8 character
        result += static_cast<char>(UTF8_3B_PRE | ((wideChar >> UTF8_2B_SHIFT) & UTF8_3B_MASK));
        result += static_cast<char>(UTF8_CONT_PRE | ((wideChar >> UTF8_CONT_SHIFT) & UTF8_CONT_MASK));
        result += static_cast<char>(UTF8_CONT_PRE | (wideChar & UTF8_CONT_MASK));
    } else if (wideChar <= UTF8_4B_MAX) {
        // 4-byte UTF-8 character (outside the Basic Multilingual Plane)
        result += static_cast<char>(UTF8_4B_PRE | ((wideChar >> UTF8_3B_SHIFT) & UTF8_4B_MASK));
        result += static_cast<char>(UTF8_CONT_PRE | ((wideChar >> UTF8_2B_SHIFT) & UTF8_CONT_MASK));
        result += static_cast<char>(UTF8_CONT_PRE | ((wideChar >> UTF8_CONT_SHIFT) & UTF8_CONT_MASK));
        result += static_cast<char>(UTF8_CONT_PRE | (wideChar & UTF8_CONT_MASK));
    }
    return result;
}

static str resolveEscapeCharacters(const str& escapeString) {
    str processed;
    processed.reserve(escapeString.length() - 1);
    size_t i = 0;
    while (i < escapeString.length()) {
        if (escapeString[i] != '\\') {
            processed += escapeString[i];
            ++i;
            continue;
        }
        ++i;  // skip the backslash
        if (i >= escapeString.length()) {
            fprintf(stderr, "error: incomplete escape sequence at end of string\n");
            return "INVALID ESCAPE SEQUENCE";
        }
        optional<wchar_t> escapeSequence;
        if (escapeString[i] == 'u' || escapeString[i] == 'x') {
            unsigned int length = (escapeString[i] == 'u') ? 4 : 2;
            if (i + length >= escapeString.length()) {
                fprintf(stderr, "error: incomplete escape sequence at end of string\n");
                return "INVALID ESCAPE SEQUENCE";
            }
            str escDigits = escapeString.substr(i + 1, length);
            size_t skipLength;
            escapeSequence = resolveHexAndUnicodeCharacters(escDigits, escapeString[i] == 'u', skipLength);
            if (!escapeSequence) {
                return "INVALID ESCAPE SEQUENCE";
            }
            i += skipLength + 1;  // skip the escape sequence (u or x) and the digits
        } else {
            escapeSequence = getEscapeCharacter(escapeString[i]);
            ++i;
        }
        if (!escapeSequence) {
            return "INVALID ESCAPE SEQUENCE";
        }
        // Convert the escape sequence to a string and add it to the processed string
        uint32_t wideChar = *escapeSequence;
        if (wideChar > UTF8_4B_MAX) {
            fprintf(stderr, "error: invalid unicode escape sequence\n");
            return "INVALID ESCAPE SEQUENCE";
        }
        processed += convertWideCharToUTF8(wideChar);
    }
    return processed;
}

void Lexer::processCharEscapeSequence(const str& charLiteral) {
    str processed = resolveEscapeCharacters(charLiteral);
    if (processed == "INVALID ESCAPE SEQUENCE") {
        fprintf(stderr, "Error: Invalid character literal at line %zu column %zu\n", getLine(), getCol());
        tokenStream.emplace_back(TokenType::Invalid, "INVALID CHARACTER LITERAL", getLine(), getCol());
        return;
    }
    // For escaped characters, we need to check if it represents a single code point
    // not necessarily the same as the length of the resolved string being 1
    size_t byteCount = processed.length();
    bool isValidSingleCodePoint = true;
    if (byteCount > 1) {
        unsigned char firstByte = static_cast<unsigned char>(processed[0]);
        isValidSingleCodePoint = (byteCount == 2 && (firstByte & 0xE0) == 0xC0) ||  // 2-byte UTF-8 character
                                 (byteCount == 3 && (firstByte & 0xF0) == 0xE0) ||  // 3-byte UTF-8 character
                                 (byteCount == 4 && (firstByte & 0xF8) == 0xF0);    // 4-byte UTF-8 character
    }
    if (!isValidSingleCodePoint) {
        fprintf(stderr, "Error: Invalid character literal at line %zu column %zu\n", getLine(), getCol());
        tokenStream.emplace_back(TokenType::Invalid, "INVALID CHARACTER LITERAL", getLine(), getCol());
        return;
    }
    tokenStream.emplace_back(TokenType::CharLiteral, processed, getLine(), getCol());
}

void Lexer::tokenizeCharLiteral() {
    size_t startLine = getLine(), startCol = getCol();
    advance();  // Move past the opening quote
    str charLiteral;
    // For simplicity, just extract a chunk of text, handle it later
    // Look for a closing quote
    while (!done() && peekChar() != '\'') {
        if (peekChar() == '\\') {
            // skip past a \ so that in '\'' the ' preceded by a \ doesn't get misinterpreted as a closing quote
            charLiteral += consumeChar();
        }
        charLiteral += consumeChar();
    }
    if (done()) {
        fprintf(stderr, "Unclosed character literal at line %zu column %zu\n", startLine, startCol);
        tokenStream.emplace_back(TokenType::Invalid, "INVALID", startLine, startCol);
        return;
    }
    // go past closing quote so it doesn't get interpreted as an opening quote in the main tokenizing function
    advance();
    if (charLiteral[0] == '\\') {
        processCharEscapeSequence(charLiteral);
        return;
    } else if (charLiteral.length() > 1) {
        fprintf(stderr, "Error: Character literal at line %zu column %zu exceeds 1 character limit.\n", startLine, startCol);
        tokenStream.emplace_back(TokenType::Invalid, "INVALID CHARACTER LITERAL", startLine, startCol);
        return;
    }
    tokenStream.emplace_back(TokenType::CharLiteral, charLiteral, startLine, startCol);
}

void Lexer::tokenizeStringLiteral() {
    size_t startLine = getLine(), startCol = getCol();
    advance();  // Move past the opening quote
    bool containsEscapeSequence = false;
    str stringLiteral;

    // for simplicity, just extract a chunk of text until the closing quote -- check it afterwards
    while (!done() && peekChar() != '"') {
        if (peekChar() == '\\') {            // Escape sequence -- skip past the next character (e.g., don't consider a \" as a closing quote)
            stringLiteral += consumeChar();  // Add the backslash to the string
            containsEscapeSequence = true;
        }
        stringLiteral += consumeChar();  // Add the character to the string
    }
    if (done()) {
        // No closing quote found
        fprintf(stderr, "Error: Unterminated string literal at line %zu column %zu\n", startLine, startCol);
        tokenStream.emplace_back(TokenType::Invalid, "INVALID", startLine, startCol);
        return;
    }
    // Move past the closing quote so it doesn't get interpreted as an opening quote in the main tokenizing function
    advance();
    if (containsEscapeSequence) {
        stringLiteral = resolveEscapeCharacters(stringLiteral);
        if (stringLiteral == "INVALID ESCAPE SEQUENCE") {
            tokenStream.emplace_back(TokenType::Invalid, "INVALID", startLine, startCol);
            return;
        }
    }
    tokenStream.emplace_back(TokenType::StrLiteral, stringLiteral, startLine, startCol);
}

void Lexer::tokenizeKeywordOrIdentifier() {
    size_t startLine = getLine(), startCol = getCol();
    str lexeme = "";
    while (!done() && (isalnum(peekChar()) || peekChar() == '_')) {
        lexeme += consumeChar();
    }
    auto it = keyword_map.find(lexeme);

    tokenStream.emplace_back(
        it != keyword_map.end() ? TokenType::Keyword : TokenType::Identifier,
        lexeme,
        startLine, startCol);
}

void Lexer::tokenizeNumber() {
    size_t startLine = getLine(), startCol = getCol();
    str numberLiteral;
    char currentChar = peekChar();
    bool isFloat = false;
    std::function<bool(char)> isValidBaseChar;
    // TODO: Add floating point support for hex numbers (but not octal or binary)
    if (currentChar == '0') {
        // Could be a base indicator (0x, 0b, 0o) -- check next char
        char baseChar = peekChar(1);
        switch (baseChar) {
            case 'x':
            case 'X':
                // Hexadecimal number
                isValidBaseChar = [](char c) { return isxdigit(static_cast<unsigned char>(c)); };
                advance(2);             // Skip the 0x
                numberLiteral += "0x";  // Add the base indicator so the parser can handle it
                break;
            case 'b':
            case 'B':
                isValidBaseChar = [](char c) { return c == '0' || c == '1'; };
                advance(2);             // Skip the 0b
                numberLiteral += "0b";  // Add the base indicator so the parser can handle it
                break;
            case 'o':
            case 'O':
                isValidBaseChar = [](char c) { return c >= '0' && c <= '7'; };
                advance(2);             // Skip the 0o
                numberLiteral += "0o";  // Add the base indicator so the parser can handle it
                break;
            default:
                // Not a valid base indicator -- just treat it as a decimal number
                isValidBaseChar = [](char c) { return isdigit(static_cast<unsigned char>(c)); };
                break;
        }
        currentChar = peekChar();  // if there was a base indicator, update the current char
    } else {
        // Decimal number
        isValidBaseChar = [](char c) { return isdigit(c); };
    }

    while (!done() && (isValidBaseChar(currentChar) || currentChar == '.')) {
        numberLiteral += consumeChar();
        if (currentChar == '.') {
            if (isFloat) {
                // Invalid number -- two decimal points
                fprintf(stderr, "Error: Invalid number at line %zu column %zu\n", startLine, startCol);
                return;
            }
            isFloat = true;
        }
        currentChar = peekChar();
    }
    // TODO: Add support for scientific notation (e.g., 1.23e4)
    tokenStream.emplace_back(
        isFloat ? TokenType::Float : TokenType::Integer,
        numberLiteral,
        startLine, startCol);
}

void Lexer::tokenizeSymbol() {
    size_t startLine = getLine(), startCol = getCol();
    TokenType type;
    char current = peekChar();
    char next = peekChar(1);
    char nextnext = peekChar(2);
    str lexeme = std::string(1, current);
    switch (current) {
        //~ Brackets
        case '(':
            type = TokenType::LeftParen;
            break;
        case '{':
            type = TokenType::LeftBrace;
            break;
        case '[':
            type = TokenType::LeftSquare;
            break;
        case '<':
            // Don't handle all the possible cases here -- let parser handle it
            type = TokenType::LeftAngle;
            break;
        case ')':
            type = TokenType::RightParen;
            break;
        case '}':
            type = TokenType::RightBrace;
            break;
        case ']':
            type = TokenType::RightSquare;
            break;
        case '>':
            // Don't handle all the possible cases here -- let parser handle it
            type = TokenType::RightAngle;
            break;

        // ~ Boolean / Bitwise operators
        case '&':
        case '|':
            type = TokenType::Operator;
            if (next == current) {
                // Logical operator (&& or ||)
                lexeme += current;
            } else if (next == '=') {
                // Bitwise assignment operator (&= or |=)
                lexeme += '=';
            } else {
                // Bitwise operator (& or |)
                // Do nothing
            }
            break;
        case '^':
        case '!':
        case '~':
        case '=':
            type = TokenType::Operator;
            if (next == '=') {
                // Assignment operator (^=, != or ~=)
                // or equality check (==)
                lexeme += '=';
            }
            break;

        // ~ Other punctuation
        case ';':
            type = TokenType::Semicolon;
            break;
        case ',':
            type = TokenType::Comma;
            break;
        case '.':
            lexeme = (next == '.' && nextnext == '.') ? "..." : ".";
            [[fallthrough]];  // Intentionally fall through to set the type
        case '?':
        case '@':
            type = TokenType::Operator;
            break;
        case ':':
            type = (next == ':') ? TokenType::Operator : TokenType::Colon;
            lexeme = (next == ':') ? "::" : ":";
            break;

        //~ Arithmetic operators
        case '+':
            type = TokenType::Operator;
            if (next == '+' || next == '=') {
                //* ++ (increment) or += (in-place addition)
                lexeme += next;
            }
            break;
        case '-':
            type = TokenType::Operator;
            if (next == '-' || next == '=' || next == '>') {
                //* -- (decrement) or -= (in-place subtraction) or -> (arrow operator)
                lexeme += next;
            }
            break;
        case '%':
            type = TokenType::Operator;
            if (next == '=') {
                // %= (in-place modulus)
                lexeme += '=';
            }
            break;
        case '*':
            type = TokenType::Operator;
            if (next == '=') {
                //* *= (in-place multiplication)
                lexeme += '=';
            } else if (next == '*') {
                // ** (exponentiation) or **= (in-place exponentiation)
                lexeme += next;
                lexeme += (nextnext == '=') ? "=" : "";
            }
            break;
        case '/':
            type = TokenType::Operator;
            if (next == '=') {
                //* /= (in-place division)
                lexeme += '=';
            } else if (next == '/') {
                //* // (floor division) or //= (in-place floor division)
                lexeme += next;
                lexeme += (nextnext == '=') ? "=" : "";
            }
            // Multiline comments handled in the main loop
            break;
        default:
            type = TokenType::Invalid;
            break;
    }
    advance(lexeme.length());
    tokenStream.emplace_back(type, lexeme, startLine, startCol);
}

void Lexer::makeTokens(size_t numTokens) {
    size_t numTokensMade = 0;
    char currentChar = peekChar();
    while (!done() && numTokensMade < numTokens) {
        if (currentChar == '#') {
            // Single line comment
            do {
                advance();
                currentChar = peekChar();
            } while (!done() && currentChar != '\n');
            advance();  // Skip the newline
        } else if (currentChar == '/' && peekChar(1) == '*') {
            // Multiline comment
            size_t startLine = getLine(), startCol = getCol();
            advance(2);  // Skip the /*
            while (!done() && !(peekChar() == '*' && peekChar(1) == '/')) {
                advance();  // Skip the comment
            }
            if (done()) {
                printf("Error: Unclosed comment at line %zu column %zu\n", startLine, startCol);
                return;
            }
            advance(2);  // Skip the */
        } else if (std::isspace(currentChar)) {
            advance();  // Skip whitespace
        } else if (isalpha(currentChar) || currentChar == '_') {
            tokenizeKeywordOrIdentifier();
            numTokensMade++;
        } else if (currentChar == '\'') {
            tokenizeCharLiteral();
            numTokensMade++;
        } else if (currentChar == '"') {
            tokenizeStringLiteral();
            numTokensMade++;
        } else if (std::isdigit(currentChar)) {
            tokenizeNumber();
            numTokensMade++;
        } else {
            tokenizeSymbol();
            numTokensMade++;
        }
        currentChar = peekChar();
    }
    if (done()) {
        isTokenizingDone = true;
        tokenStream.emplace_back(TokenType::EndOfFile, "EOF", getLine(), getCol());
    }
}

Token Lexer::peekToken(size_t offset) {
    if (isTokenizingDone && offset >= tokenStream.size()) {
        // Only return EOF if we are done tokenizing and trying to read past the end
        return Token(TokenType::EndOfFile, "EOF", getLine(), getCol());
    }
    if (tokenStream.empty()) {
        size_t numToMake = offset == 0 ? 1 : offset;
        numToMake = std::max(numToMake, QUEUE_LOOKAHEAD_AMOUNT);
        makeTokens(numToMake);  // If queue empty, generate {offset} tokens to read
    } else if (tokenStream.size() <= offset) {
        size_t numToMake = offset - tokenStream.size();
        numToMake = std::max(numToMake, QUEUE_LOOKAHEAD_AMOUNT);
        makeTokens(numToMake);  // fill the queue with more tokens
    }
    return offset < tokenStream.size() ? tokenStream[offset] : Token(TokenType::EndOfFile, "EOF", getLine(), getCol());
}

Token Lexer::consumeToken() {
    if (tokenStream.empty()) {
        makeTokens(1);  // If queue empty, generate 1 token to read
    }
    if (tokenStream.empty()) {
        // still empty -- we are done tokenizing
        isTokenizingDone = true;
        return Token(TokenType::EndOfFile, "EOF", getLine(), getCol());
    }
    Token token = tokenStream.front();
    tokenStream.pop_front();  // get rid of the token
    return token;
}
}  // namespace lexer
MANG_END