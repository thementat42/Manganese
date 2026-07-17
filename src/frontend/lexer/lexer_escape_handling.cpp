#include <format>
#include <frontend/lexer.hpp>
#include <io/logging.hpp>
#include <mnstl/number.hxx>
#include <optional>
#include <string>

// TODO: Rework to properly handle unicode, validation, etc.

namespace Manganese {
namespace lexer {
// ~ Constants for UTF-8 encoding
constexpr inline uint32_t UTF8_1B_MAX = 0x7F;
constexpr inline uint32_t UTF8_2B_MAX = 0x7FF;
constexpr inline uint32_t UTF8_3B_MAX = 0xFFFF;
constexpr inline uint32_t UTF8_4B_MAX = 0x10FFFF;
constexpr inline uint32_t UTF16_SURROGATE_MIN = 0xD800;
constexpr inline uint32_t UTF16_SURROGATE_MAX = 0xDFFF;

constexpr inline uint8_t UTF8_2B_PRE = 0xC0;
constexpr inline uint8_t UTF8_3B_PRE = 0xE0;
constexpr inline uint8_t UTF8_4B_PRE = 0xF0;
constexpr inline uint8_t UTF8_CONT_PRE = 0x80;

constexpr inline uint8_t UTF8_2B_MASK = 0x1F;
constexpr inline uint8_t UTF8_3B_MASK = 0x0F;
constexpr inline uint8_t UTF8_4B_MASK = 0x07;
constexpr inline uint8_t UTF8_CONT_MASK = 0x3F;

constexpr inline uint8_t UTF8_CONT_SHIFT = 6;
constexpr inline uint8_t UTF8_2B_SHIFT = 12;
constexpr inline uint8_t UTF8_3B_SHIFT = 18;

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
            return std::nullopt;
        }
        std::optional<char32_t> escapeChar;
        uint8_t skipLength = 1;
        if (escapeString[i] == 'u') {
            std::string escDigits = escapeString.substr(i + 1, 4);  // 4 for uXXXX
            escapeChar = resolveUnicodeCharacters(escDigits, getLine(), getCol());
            skipLength = 5;
        } else if (escapeString[i] == 'U') {
            std::string escDigits = escapeString.substr(i + 1, 8);  // 8 for UXXXXXXXX
            escapeChar = resolveUnicodeCharacters(escDigits, getLine(), getCol(), /*isLongUnicode=*/true);
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
            return std::nullopt;
        }
        i += skipLength;
        processed += encodeUTF8String(*escapeChar);
    }
    return processed;
}

Result Lexer::processCharEscapeSequence(const std::string& charLiteral) {
    std::optional<std::string> resolved = resolveEscapeCharacters(charLiteral);
    if (!resolved) {
        logging::logError(getLine(), getCol(), "Invalid character literal", charLiteral);
        tokenStream.emplace_back(TokenType::CharLiteral, charLiteral, getLine(), getCol(), /*invalid=*/true);
        return Result::Failure;
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
    Result result = Result::Success;
    if (!isValidSingleCodePoint) {
        logging::logError(getLine(), getCol(), "Invalid character literal ", charLiteral);
        result = Result::Failure;
    }
    tokenStream.emplace_back(TokenType::CharLiteral, processed, getLine(), getCol(),
                             /*invalid=*/result == Result::Failure);
    return result;
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
            return std::nullopt;
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
    if (esc.length() != 2) { return std::nullopt; }
    // Check that both characters are hex digits
    if (!isxdigit(esc[0]) || !isxdigit(esc[1])) { return std::nullopt; }
    char32_t hexChar = static_cast<char32_t>(hexDigitToInt(esc[0]));
    hexChar <<= 4;  // Make room for the second hex digit
    hexChar |= static_cast<char32_t>(hexDigitToInt(esc[1]));
    return hexChar;
}

std::optional<char32_t> resolveUnicodeCharacters(const std::string& esc, size_t line, size_t col, bool isLongUnicode) {
    size_t expectedLength = isLongUnicode ? 8 : 4;  // 8 for \UXXXXXXXX, 4 for \uXXXX
    if (esc.length() != expectedLength) { return std::nullopt; }
    char32_t unicodeChar = 0;
    for (char c : esc) {
        if (!isxdigit(c)) { return std::nullopt; }
        unicodeChar <<= 4;
        unicodeChar |= static_cast<char32_t>(hexDigitToInt(c));
    }
    if (unicodeChar >= UTF16_SURROGATE_MIN && unicodeChar <= UTF16_SURROGATE_MAX) {
        // Invalid Unicode character in the surrogate range
        logging::logError(line, col, "Error: Invalid Unicode character in the surrogate range");
        return std::nullopt;
    }
    if (unicodeChar > UTF8_4B_MAX) {
        // Unicode character is outside the valid range for UTF-8
        logging::logError(line, col, "Error: Unicode character is outside the valid range for UTF-8");
        return std::nullopt;
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
