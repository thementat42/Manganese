/**
 * @file lexer_escape_handling.cpp
 * @brief Helper file for processing string/char literals with escape sequences
 */

#include <format>
#include <frontend/lexer.hpp>
#include <io/logging.hpp>
#include <optional>
#include <string>
#include <utils/number_utils.hpp>


namespace Manganese {
namespace lexer {
// ~ Constants for UTF-8 encoding
constexpr uint32_t UTF8_1B_MAX = 0x7F;
constexpr uint32_t UTF8_2B_MAX = 0x7FF;
constexpr uint32_t UTF8_3B_MAX = 0xFFFF;
constexpr uint32_t UTF8_4B_MAX = 0x10FFFF;
constexpr uint32_t UTF16_SURROGATE_MIN = 0xD800;
constexpr uint32_t UTF16_SURROGATE_MAX = 0xDFFF;

constexpr uint8_t UTF8_2B_PRE = 0xC0;
constexpr uint8_t UTF8_3B_PRE = 0xE0;
constexpr uint8_t UTF8_4B_PRE = 0xF0;
constexpr uint8_t UTF8_CONT_PRE = 0x80;

constexpr uint8_t UTF8_2B_MASK = 0x1F;
constexpr uint8_t UTF8_3B_MASK = 0x0F;
constexpr uint8_t UTF8_4B_MASK = 0x07;
constexpr uint8_t UTF8_CONT_MASK = 0x3F;

constexpr uint8_t UTF8_CONT_SHIFT = 6;
constexpr uint8_t UTF8_2B_SHIFT = 12;
constexpr uint8_t UTF8_3B_SHIFT = 18;

std::optional<std::string> Lexer::resolveEscapeCharacters(const std::string& escapeString) {
    std::string processed;
    processed.reserve(escapeString.length() - 1);
    size_t i = 0;
    while (i < escapeString.length()) {
        if (escapeString[i] != '\\') [[likely]] {  // Most characters are not escaped
            processed += escapeString[i++];
            continue;
        }
        ++i;  // skip the backslash
        if (i >= escapeString.length()) {
            logging::logError(getLine(), getCol(), "Incomplete escape sequence at end of string");
            return NONE;
        }
        std::optional<char32_t> escapeChar;
        uint8_t skipLength = 1;
        if (escapeString[i] == 'u') {
            std::string escDigits = escapeString.substr(i + 1, 4);  // 4 for uXXXX
            escapeChar = resolveUnicodeCharacters(escDigits, getLine(), getCol());
            skipLength = 5;
        } else if (escapeString[i] == 'U') {
            std::string escDigits = escapeString.substr(i + 1, 8);  // 8 for UXXXXXXXX
            escapeChar = resolveUnicodeCharacters(escDigits, getLine(), getCol(), /*isLongUnicode=*/ true);
            skipLength = 9;
        } else if (escapeString[i] == 'x') [[unlikely]] {  // Hex escape sequences aren't usually used
            std::string escDigits = escapeString.substr(i + 1, 2);  // 2 for xXX
            escapeChar = resolveHexCharacters(escDigits);
            skipLength = 3;
        } else {
            escapeChar = getEscapeCharacter(escapeString[i], getLine(), getCol());
        }
        if (!escapeChar) {
            if (escapeString[i] == 'x') {
                logging::logError(getLine(), getCol(), "Invalid hex escape sequence (expected \\xXX)");
            } else if (escapeString[i] == 'u') {
                logging::logError(getLine(), getCol(), "Invalid unicode escape sequence (expected \\uXXXX)");
            }
            return NONE;
        }
        i += skipLength;
        processed += encodeUTF8String(*escapeChar);
    }
    return processed;
}

void Lexer::processCharEscapeSequence(const std::string& charLiteral) {
    std::optional<std::string> resolved = resolveEscapeCharacters(charLiteral);
    if (!resolved) {
        logging::logError(getLine(), getCol(), "Invalid character literal", charLiteral);
        tokenStream.emplace_back(TokenType::CharLiteral, charLiteral, getLine(), getCol(), true);
        return;
    }
    std::string processed = *resolved;
    // For escaped characters, we need to check if it represents a single code point
    // not necessarily the same as the length of the resolved string being 1
    size_t byteCount = processed.length();
    bool isValidSingleCodePoint = true;
    if (byteCount > 1) {
        unsigned char firstByte = static_cast<unsigned char>(processed[0]);
        isValidSingleCodePoint = (byteCount == 2 && (firstByte & 0xE0) == 0xC0) ||  // 2-byte UTF-8 character
            (byteCount == 3 && (firstByte & 0xF0) == 0xE0) ||  // 3-byte UTF-8 character
            (byteCount == 4 && (firstByte & 0xF8) == 0xF0);  // 4-byte UTF-8 character
    }
    if (!isValidSingleCodePoint) {
        logging::logError(getLine(), getCol(), "Invalid character literal ", charLiteral);
        tokenStream.emplace_back(TokenType::CharLiteral, charLiteral, getLine(), getCol());
        return;
    }
    tokenStream.emplace_back(TokenType::CharLiteral, processed, getLine(), getCol());
}

std::optional<char> getEscapeCharacter(const char escapeChar, size_t line, size_t col) {
    switch (escapeChar) {
        case '\\': return '\\';
        case '\'': return '\'';
        case '\"': return '\"';
        case 'a': return '\a';
        case 'b': return '\b';
        case 'f': return '\f';
        case 'n': return '\n';
        case 'r': return '\r';
        case 't': return '\t';
        case 'v': return '\v';
        case '0': return '\0';
        default:
            logging::logError(
                line, col,
                "\\{} is not a valid escape sequence. If you meant to type a backslash ('\\'), use two backslashes ",
                escapeChar);
            return NONE;
    }
}

inline int hexDigitToInt(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;  // Not a valid hex digit
}

std::optional<char32_t> resolveHexCharacters(const std::string& esc) {
    // Check that the string is exactly 2 characters long
    if (esc.length() != 2) { return NONE; }
    // Check that both characters are hex digits
    if (!std::isxdigit(esc[0]) || !std::isxdigit(esc[1])) { return NONE; }
    char32_t hexChar = static_cast<char32_t>(hexDigitToInt(esc[0]));
    hexChar <<= 4;  // Make room for the second hex digit
    hexChar |= static_cast<char32_t>(hexDigitToInt(esc[1]));
    return hexChar;
}

std::optional<char32_t> resolveUnicodeCharacters(const std::string& esc, size_t line, size_t col, bool isLongUnicode) {
    size_t expectedLength = isLongUnicode ? 8 : 4;  // 8 for \UXXXXXXXX, 4 for \uXXXX
    if (esc.length() != expectedLength) { return NONE; }
    char32_t unicodeChar = 0;
    for (char c : esc) {
        if (!std::isxdigit(c)) { return NONE; }
        unicodeChar <<= 4;
        unicodeChar |= static_cast<char32_t>(hexDigitToInt(c));
    }
    if (unicodeChar >= UTF16_SURROGATE_MIN && unicodeChar <= UTF16_SURROGATE_MAX) {
        // Invalid Unicode character in the surrogate range
        logging::logError(line, col, "Error: Invalid Unicode character in the surrogate range");
        return NONE;
    }
    if (unicodeChar > UTF8_4B_MAX) {
        // Unicode character is outside the valid range for UTF-8
        logging::logError(line, col, "Error: Unicode character is outside the valid range for UTF-8");
        return NONE;
    }
    return unicodeChar;
}

std::string encodeUTF8String(char32_t wideChar) {
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
