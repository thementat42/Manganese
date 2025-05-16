#include "include/lex2.h"

#include <algorithm>

MANG_BEGIN
namespace lexer {
Lexer::Lexer(const str& source, Mode mode) {
    switch (mode) {
        case Mode::String:
            reader = std::make_unique<io::StringReader>(source);
            break;
        case Mode::File:
            reader = std::make_unique<io::FileReader>(source);
            break;
    }
    tokenStream = std::queue<Token>();
    isTokenizingDone = false;
}

optional<wchar_t> resolveHexAndUnicodeCharacters(const str& esc, const bool& isUnicode, size_t& skipLength) {
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

inline optional<wchar_t> getEscapeCharacter(const char& escapeChar) {
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

inline std::string convertWideCharToUTF8(uint32_t wideChar) {
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

str resolveEscapeCharacters(const str& escapeString) {
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
        tokenStream.emplace(TokenType::Invalid, "INVALID CharLiteral LITERAL", getLine(), getCol());
        return;
    }
}

void Lexer::tokenizeCharLiteral() {
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
        fprintf(stderr, "Unclosed character literal at line %zu column %zu\n", getLine(), getCol());
        tokenStream.emplace(TokenType::Invalid, "INVALID", getLine(), getCol());
        return;
    }
    // go past closing quote so it doesn't get interpreted as an opening quote in the main tokenizing function
    advance();
    if (charLiteral[0] == '\\') {
        processCharEscapeSequence(charLiteral);
        return;
    } else if (charLiteral.length() > 1) {
        fprintf(stderr, "Error: Character literal at line %zu column %zu exceeds 1 character limit.\n", getLine(), getCol());
        tokenStream.emplace(TokenType::Invalid, "INVALID CharLiteral LITERAL", getLine(), getCol());
        return;
    }
    tokenStream.emplace(TokenType::CharLiteral, charLiteral, getLine(), getCol());
}

}  // namespace lexer
MANG_END