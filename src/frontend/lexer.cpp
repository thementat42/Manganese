/**
 * @file lexer.cpp
 * @brief This file contains the implementation of the lexer for the Manganese compiler.
 *
 * The lexer takes the source code and splits it into tokens (see token.h)
 * These tokens are then passed on to the parser to build the AST.
 *
 *
 * @note: The lexer strips out comments and whitespace; the parser never sees this
 *
 * @note: The main loop does not advance the reader position, it just peeks the current character.
 * Each specific tokenization function should advance the reader position once its token has been generated.
 * E.g. the string tokenizing function will advance the reader past the quotes
 */

#include <frontend/lexer.h>
#include <frontend/token.h>
#include <global_macros.h>
#include <io/filereader.h>
#include <io/logging.h>
#include <io/reader.h>
#include <io/stringreader.h>

#include <algorithm>
#include <format>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

namespace Manganese {

namespace lexer {

//~ Core Lexer Functions

Lexer::Lexer(const str& source, const Mode mode) : tokenStartLine(1), tokenStartCol(1) {
    switch (mode) {
        case Mode::String:
            reader = std::make_unique<io::StringReader>(source);
            break;
        case Mode::File:
            reader = std::make_unique<io::FileReader>(source);
            break;
    }
}

void Lexer::lex(size_t numTokens) {
    if (done()) {
        // Nothing else to do
        return;
    }
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
            advance(2);  // Skip the /*
            while (!done() && !(peekChar() == '*' && peekChar(1) == '/')) {
                advance();  // Skip the comment
            }
            if (done()) [[unlikely]] {
                logging::logUser("Unclosed multiline comment", logging::LogLevel::Error, getLine(), getCol());
                return;
            }
            advance(2);  // Skip the */
        } else if (std::isspace(currentChar)) [[likely]] {
            advance();  // Skip whitespace
        } else if (isalpha(currentChar) || currentChar == '_') [[likely]] {
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
        tokenStartLine = getLine();
        tokenStartCol = getCol();
    }
    if (done()) {
        // Just finished tokenizing
        tokenStream.emplace_back(TokenType::EndOfFile, "EOF", getLine(), getCol());
    }
}

Token Lexer::peekToken(size_t offset) noexcept {
    if (done() && offset >= tokenStream.size()) {
        // Only return EOF if we are done tokenizing and trying to read past the end
        return Token(TokenType::EndOfFile, "EOF", getLine(), getCol());
    }
    if (tokenStream.empty()) {
        size_t numToMake = offset == 0 ? 1 : offset;
        numToMake = std::max(numToMake, QUEUE_LOOKAHEAD_AMOUNT);
        lex(numToMake);  // If queue empty, generate {offset} tokens to read
    } else if (tokenStream.size() <= offset) {
        size_t numToMake = offset - tokenStream.size();
        numToMake = std::max(numToMake, QUEUE_LOOKAHEAD_AMOUNT);
        lex(numToMake);  // fill the queue with more tokens
    }
    // If the token stream can't be filled up with enough tokens, we're reading past the end -- indicate that
    return offset < tokenStream.size() ? tokenStream[offset] : Token(TokenType::EndOfFile, "EOF", getLine(), getCol());
}

Token Lexer::consumeToken() noexcept {
    if (tokenStream.empty()) {
        lex(QUEUE_LOOKAHEAD_AMOUNT);  // If queue empty, generate 1 token to read
    }
    if (tokenStream.empty()) {
        // still empty -- we are done tokenizing
        return Token(TokenType::EndOfFile, "EOF", getLine(), getCol());
    }
    Token token = tokenStream.front();
    tokenStream.pop_front();  // get rid of the token
    return token;
}

//~ Main State Machine Functions

void Lexer::tokenizeCharLiteral() {
    advance();  // Move past the opening quote
    str charLiteral;
    // For simplicity, just extract a chunk of text, handle it later
    // Look for a closing quote
    while (true) {
        if (done()) {
            logging::logUser("Unclosed character literal", logging::LogLevel::Error, getLine(), getCol());
            tokenStream.emplace_back(TokenType::Invalid, "INVALID", tokenStartLine, tokenStartCol);
            return;
        }
        if (peekChar() == '\'') {
            break;
        }
        if (peekChar() == '\n') {
            logging::logUser("Unclosed character literal", logging::LogLevel::Error, getLine(), getCol());

            tokenStream.emplace_back(TokenType::Invalid, "INVALID", tokenStartLine, tokenStartCol);
            return;
        }
        if (peekChar() == '\\') {
            // Skip past a \ so that in '\'' the ' preceded by a \ doesn't get misinterpreted as a closing quote
            charLiteral += consumeChar();  // Add the backslash to the string
        }
        charLiteral += consumeChar();  // Add the character to the string
    }
    advance();
    if (charLiteral[0] == '\\') {
        processCharEscapeSequence(charLiteral);
        return;
    } else if (charLiteral.length() > 1) {
        logging::logUser("Character literal exceeds 1 character limit", logging::LogLevel::Error, getLine(), getCol());
        tokenStream.emplace_back(TokenType::Invalid, charLiteral, tokenStartLine, tokenStartCol);
        return;
    }
    tokenStream.emplace_back(TokenType::CharLiteral, charLiteral, tokenStartLine, tokenStartCol);
}

void Lexer::tokenizeStringLiteral() {
    advance();  // Move past the opening quote
    bool containsEscapeSequence = false;
    str stringLiteral;

    // for simplicity, just extract a chunk of text until the closing quote -- check it afterwards
    while (true) {
        if (done()) {
            logging::logUser("Unclosed string literal", logging::LogLevel::Error, getLine(), getCol());
            tokenStream.emplace_back(TokenType::Invalid, "INVALID", tokenStartLine, tokenStartCol);
            return;
        }
        if (peekChar() == '"') {
            break;
        }
        if (peekChar() == '\\') {
            if (peekChar(1) == '\n') {
                // continuing a string literal across lines
                advance(2);  // Skip the backslash and the newline
                continue;
            }
            // Escape sequence -- skip past the next character (e.g., don't consider a \" as a closing quote)
            stringLiteral += consumeChar();  // Add the backslash to the string
            containsEscapeSequence = true;
        } else if (peekChar() == '\n') {
            logging::logUser(
                {"String literal cannot span multiple lines.",
                 "If you wanted a string literal that spans lines,\
                add a backslash ('\\') at the end of the line"},
                logging::LogLevel::Error, getLine(), getCol());

            tokenStream.emplace_back(TokenType::Invalid, stringLiteral, tokenStartLine, tokenStartCol);
            return;
        }
        stringLiteral += consumeChar();  // Add the character to the string
    }

    advance();
    if (containsEscapeSequence) {
        auto result = resolveEscapeCharacters(stringLiteral);
        if (!result) {
            tokenStream.emplace_back(TokenType::Invalid, "INVALID", tokenStartLine, tokenStartCol);
            return;
        }
        stringLiteral = std::move(result.value());
    }
    tokenStream.emplace_back(TokenType::StrLiteral, stringLiteral, tokenStartLine, tokenStartCol);
}

void Lexer::tokenizeKeywordOrIdentifier() {
    str lexeme = "";
    while (!done() && (isalnum(peekChar()) || peekChar() == '_')) {
        lexeme += consumeChar();
    }
    auto it = keywordMap.find(lexeme);

    tokenStream.emplace_back(
        it != keywordMap.end() ? TokenType::Keyword : TokenType::Identifier,
        lexeme,
        tokenStartLine, tokenStartCol);
}

void Lexer::tokenizeNumber() {
    str numberLiteral;
    bool isFloat = false;
    std::function<bool(char)> isValidBaseChar;
    Base base = processNumberPrefix(isValidBaseChar, numberLiteral);
    char currentChar = peekChar();  // If there was a number prefix, update the current char

    while (!done() && (isValidBaseChar(currentChar) || currentChar == '.' || currentChar == '_')) {
        if (currentChar == '_') {
            // Ignore underscores
            advance();
            currentChar = peekChar();
            continue;
        } else if (currentChar == '.') {
            if (isFloat) {
                // Invalid number -- two decimal points
                logging::logUser("Invalid number literal: multiple decimal points",
                                 logging::LogLevel::Error,
                                 getLine(), getCol());
                tokenStream.emplace_back(TokenType::Invalid, numberLiteral, tokenStartLine, tokenStartCol);
                return;
            }
            // Reject floating point for octal and binary numbers
            if (base == Base::Octal || base == Base::Binary) {
                logging::logUser(
                    std::format("Invalid number literal: floating point not allowed for {} numbers",
                                (base == Base::Octal ? "octal" : "binary")),
                    logging::LogLevel::Error, getLine(), getCol());
                tokenStream.emplace_back(TokenType::Invalid, numberLiteral, tokenStartLine, tokenStartCol);
                return;
            }
            isFloat = true;
        }
        numberLiteral += consumeChar();
        currentChar = peekChar();
    }

    // Handle scientific notation (e.g., 1.23e4), size suffixes (e.g., 1.23f), etc.
    processNumberSuffix(base, numberLiteral, isFloat);
    tokenStream.emplace_back(
        isFloat ? TokenType::FloatLiteral : TokenType::IntegerLiteral,
        numberLiteral,
        tokenStartLine, tokenStartCol);
}

void Lexer::tokenizeSymbol() {
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
        case ')':
            type = TokenType::RightParen;
            break;
        case '}':
            type = TokenType::RightBrace;
            break;
        case ']':
            type = TokenType::RightSquare;
            break;

        // ~ Boolean / Bitwise operators
        case '&':  // Let the parser decide if '&' is a bitwise AND or an address-of operator, default to bitwise AND
        case '|':
            type = TokenType::Operator;
            if (next == current || next == '=') {
                // If next is the same character, it's a logical operator (&& or ||)
                // or a bitwise assignment operator (&= or |=)
                lexeme += next;
            }
            // Otherwise, just a single & or | operator
            break;
        case '^':  // Bitwise XOR
            type = TokenType::Operator;
            if (next == '=') {
                // Bitwise assignment operator (^=)
                lexeme += '=';
            } else if (next == '^') {
                // Exponentiation operator (^^)
                lexeme += '^';
                lexeme += (nextnext == '=') ? "=" : "";  // ^^=, in place exponentiation
            }
            break;
        case '!':  // NOT
        case '~':  // Bitwise NOT
        case '=':  // Assignment
            type = TokenType::Operator;
            if (next == '=') {
                // Assignment operator (!= or ~=)
                // or equality check (==)
                lexeme += '=';
            }
            break;
        case '<':
        case '>':
            type = TokenType::Operator;
            if (next == '=') {
                // <= (less than or equal to) or >= (greater than or equal to)
                lexeme += '=';
            } else if (next == current) {
                // << (bitwise left shift) or >> (bitwise right shift)
                lexeme += next;
                // <<= (in place left shift) or >>= (in place right shift)
                lexeme += (nextnext == '=') ? "=" : "";
            }
            // Otherwise, just a regular comparison
            break;

        // ~ Other punctuation
        case ';':
            type = TokenType::Semicolon;
            break;
        case ',':
            type = TokenType::Comma;
            break;
        case '.':
            if (next == '.' && nextnext == '.') {
                lexeme = "...";
            }
            type = TokenType::Operator;
            break;
        case ':':
            type = (next == ':') ? TokenType::Operator : TokenType::Colon;
            lexeme = (next == ':') ? "::" : ":";
            break;
        case '@':
            type = TokenType::Operator;
            break;

        //~ Arithmetic operators
        case '+':
            type = TokenType::Operator;
            if (next == '+' || next == '=') {
                // ++ (increment) or += (in-place addition)
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
        case '*':
            type = TokenType::Operator;
            if (next == '=') {
                // %= or *= (in-place modulus or multiplication)
                lexeme += '=';
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
            logging::logUser(
                std::format("Invalid character: '{}'", current),
                logging::LogLevel::Error,
                getLine(), getCol());
            tokenStream.emplace_back(TokenType::Invalid, lexeme, tokenStartLine, tokenStartCol);
            break;
    }
    advance(lexeme.length());
    tokenStream.emplace_back(type, lexeme, tokenStartLine, tokenStartCol);
}

//~ Helper Functions

optional<wchar_t> Lexer::resolveHexAndUnicodeCharacters(const str& esc, const bool& isUnicode, size_t& skipLength) {
    // Length is checked in caller (when passing in substring)
    auto isNotHex = [](char c) { return !std::isxdigit(static_cast<unsigned char>(c)); };
    auto x = std::find_if(esc.begin(), esc.begin() + (isUnicode ? 4 : 2), isNotHex);
    if (x != esc.end()) {
        logging::logUser(
            std::format("Error: Invalid {} escape sequence: \\{}{}",
                        (isUnicode ? "unicode" : "hex"),
                        (isUnicode ? "u" : "x"),
                        esc),
            logging::LogLevel::Error, getLine(), getCol());
        return NONE;
    }
    wchar_t unicodeChar = 0;
    size_t length = isUnicode ? 4 : 2;
    unicodeChar = 0;
    for (size_t i = 0; i < length; ++i) {
        unicodeChar <<= 4;
        unicodeChar |= static_cast<wchar_t>(std::stoi(std::string(1, esc[i]), nullptr, 16));
    }
    skipLength = length;
    return unicodeChar;
}

std::optional<str> Lexer::resolveEscapeCharacters(const str& escapeString) {
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
            logging::logUser("Error: incomplete escape sequence at end of string", logging::LogLevel::Error, 0, 0);
            return NONE;
        }
        optional<wchar_t> escapeSequence;
        if (escapeString[i] == 'u' || escapeString[i] == 'x') {
            unsigned int length = (escapeString[i] == 'u') ? 4 : 2;
            if (i + length >= escapeString.length()) {
                logging::logUser("Error: incomplete escape sequence at end of string", logging::LogLevel::Error, 0, 0);
                return NONE;
            }
            str escDigits = escapeString.substr(i + 1, length);
            size_t skipLength;
            escapeSequence = resolveHexAndUnicodeCharacters(escDigits, escapeString[i] == 'u', skipLength);
            if (!escapeSequence) {
                return NONE;
            }
            i += skipLength + 1;  // skip the escape sequence (u or x) and the digits
        } else {
            escapeSequence = getEscapeCharacter(escapeString[i]);
            ++i;
        }
        if (!escapeSequence) {
            return NONE;
        }
        // Convert the escape sequence to a string and add it to the processed string
        wchar_t wideChar = *escapeSequence;
#ifndef _WIN32
        // On unix, wchar_t exceeds the UTF-8 4-byte limit, so we need an additional range check
        if (wideChar > UTF8_4B_MAX) {
            logging::logUser(
                std::format(
                    "Error: invalid unicode escape sequence: \\u{:X}",
                    static_cast<unsigned int>(wideChar)),
                logging::LogLevel::Error, tokenStartLine, tokenStartCol);
            return NONE;
        }
#endif  // _WIN32
        processed += convertWideCharToUTF8(wideChar);
    }
    return processed;
}

void Lexer::processCharEscapeSequence(const str& charLiteral) {
    std::optional<str> resolved = resolveEscapeCharacters(charLiteral);
    if (!resolved) {
        logging::logUser("Error: Invalid character literal", logging::LogLevel::Error, getLine(), getCol());
        tokenStream.emplace_back(TokenType::Invalid, "INVALID CHARACTER LITERAL", getLine(), getCol());
        return;
    }
    str processed = *resolved;
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
        logging::logUser("Error: Invalid character literal", logging::LogLevel::Error, getLine(), getCol());
        tokenStream.emplace_back(TokenType::Invalid, "INVALID CHARACTER LITERAL", getLine(), getCol());
        return;
    }
    tokenStream.emplace_back(TokenType::CharLiteral, processed, getLine(), getCol());
}

Base Lexer::processNumberPrefix(std::function<bool(char)>& isValidBaseChar, str& numberLiteral) {
    char currentChar = peekChar();
    if (currentChar != '0') {
        // Decimal number
        isValidBaseChar = [](char c) { return isdigit(c); };
        return Base::Decimal;
    }
    // Could be a base indicator (0x, 0b, 0o) -- check next char
    switch (peekChar(1)) {
        case 'x':
        case 'X':
            // Hexadecimal number
            isValidBaseChar = [](char c) { return isxdigit(static_cast<unsigned char>(c)); };
            advance(2);
            numberLiteral += "0x";
            return Base::Hexadecimal;
        case 'b':
        case 'B':
            isValidBaseChar = [](char c) { return c == '0' || c == '1'; };
            advance(2);
            numberLiteral += "0b";
            return Base::Binary;
        case 'o':
        case 'O':
            isValidBaseChar = [](char c) { return c >= '0' && c <= '7'; };
            advance(2);
            numberLiteral += "0o";
            return Base::Octal;
        default:
            // Not a valid base indicator -- just treat it as a decimal number
            isValidBaseChar = [](char c) { return isdigit(static_cast<unsigned char>(c)); };
            return Base::Decimal;
    }
}

bool Lexer::processNumberSuffix(Base base, str& numberLiteral, bool isFloat) {
    DISABLE_CONVERSION_WARNING
    // All suffixes are case-insensitive, so use tolower for simplicity

    char currentChar = tolower(peekChar());
    auto readDigits = [this]() -> std::string {
        std::string digits;
        while (!done() && isdigit(peekChar())) {
            digits += consumeChar();
        }
        return digits;
    };

    if (currentChar == 'i' || currentChar == 'u' || currentChar == 'f') {
        /*
        The valid numeric suffixes are:
        - i8, i16, i32, i64 (signed integers with the corresponding bit width)
        - u8, u16, u32, u64 (unsigned integers with the corresponding bit width)
        - f32, f64 (floating-point numbers with the corresponding bit width)
        NOTE: These are case-insensitive, so 'I', 'U', and 'F' are also valid.
        */

        std::string suffix;
        suffix += tolower(consumeChar());
        currentChar = peekChar();

        // Read the numeric part of the suffix
        std::string numPart = readDigits();
        if (suffix[0] == 'i' || suffix[0] == 'u') {
            // Integer types: i8, i16, i32, i64, u8, u16, u32, u64
            if (numPart != "8" && numPart != "16" && numPart != "32" && numPart != "64") {
                logging::logUser(
                    "Invalid integer suffix: must be 8, 16, 32, or 64",
                    logging::LogLevel::Error,
                    getLine(),
                    getCol());
                return false;
            }
        } else if (numPart != "32" && numPart != "64") {
            // Float types: f32, f64
            logging::logUser("Invalid float suffix: must be 32 or 64", logging::LogLevel::Error, getLine(), getCol());
            return false;
        }

        numberLiteral += suffix + numPart;  // Append the suffix to the number literal

        // Type validation - float suffix only for float literals and vice versa
        if (suffix[0] == 'f' && !isFloat) {
            logging::logUser(
                "Float suffix can only be used with floating-point literals",
                logging::LogLevel::Error,
                getLine(), getCol());
            return false;
        }
        if ((suffix[0] == 'i' || suffix[0] == 'u') && isFloat) {
            logging::logUser("Integer suffix cannot be used with floating-point literals",
                             logging::LogLevel::Error,
                             getLine(),
                             getCol());
            return false;
        }
    }
    currentChar = tolower(peekChar());  // Update current character after processing the suffix
    // Scientific notation handling
    if (currentChar == 'e' && base == Base::Decimal) {
        // Scientific notation
        numberLiteral += tolower(consumeChar());  // Add the 'e' or 'E'
        currentChar = peekChar();
        if (currentChar == '+' || currentChar == '-') {
            numberLiteral += consumeChar();  // Add the sign
            currentChar = peekChar();
        }
        if (!isdigit(currentChar)) {
            logging::logUser(
                "Invalid scientific notation: exponent must be a number",
                logging::LogLevel::Error,
                getLine(),
                getCol());
            return false;
        }
        numberLiteral += readDigits();
    } else if (base == Base::Hexadecimal) {
        // Hexadecimal floats must have a 'p' or 'P' exponent if they are floats
        if (isFloat) {
            if (currentChar != 'p') {
                logging::logUser(
                    "Invalid hexadecimal float: must have 'p' or 'P' exponent",
                    logging::LogLevel::Error,
                    getLine(),
                    getCol());
                return false;
            }
            // Hexadecimal exponentiation (e.g., 0x1.23p4)
            numberLiteral += tolower(consumeChar());  // Add the 'p' or 'P'
            currentChar = peekChar();
            if (currentChar == '+' || currentChar == '-') {
                numberLiteral += consumeChar();  // Add the sign
                currentChar = peekChar();
            }
            if (!isdigit(currentChar)) {
                logging::logUser("Invalid hexadecimal float: exponent must be a decimal number",
                                 logging::LogLevel::Error,
                                 getLine(),
                                 getCol());
                return false;
            }
            numberLiteral += readDigits();  // Add the exponent digits
        }
    }
    ENABLE_CONVERSION_WARNING

    return true;
}

//~ Static Helper Functions
optional<wchar_t> getEscapeCharacter(const char& escapeChar) {
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
            logging::logUser(
                std::format(
                    "\\{} is not a valid escape sequence.\
                    If you meant to type a backslash ('\\'), use two backslashes ('\\\\{}')",
                    escapeChar, escapeChar),
                logging::LogLevel::Error, 0, 0);
            return NONE;
    }
}

std::string convertWideCharToUTF8(wchar_t wideChar) {
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
    } else {  // No need to check the upper limit -- that was done in the escape sequence resolver
        // 4-byte UTF-8 character (outside the Basic Multilingual Plane)
        result += static_cast<char>(UTF8_4B_PRE | ((wideChar >> UTF8_3B_SHIFT) & UTF8_4B_MASK));
        result += static_cast<char>(UTF8_CONT_PRE | ((wideChar >> UTF8_2B_SHIFT) & UTF8_CONT_MASK));
        result += static_cast<char>(UTF8_CONT_PRE | ((wideChar >> UTF8_CONT_SHIFT) & UTF8_CONT_MASK));
        result += static_cast<char>(UTF8_CONT_PRE | (wideChar & UTF8_CONT_MASK));
    }

    return result;
}

}  // namespace lexer
}  // namespace Manganese
