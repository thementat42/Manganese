#include <cstddef>
#include <cstdint>
#include <format>
#include <frontend/lexer.hpp>
#include <io/logging.hpp>
#include <mnstl/number.hxx>
#include <optional>
#include <string>
#include <utils/result.hpp>

namespace Manganese::lexer {

constexpr inline std::uint32_t UTF8_1B_MAX = 0x7F;
constexpr inline std::uint32_t UTF8_2B_MAX = 0x7FF;
constexpr inline std::uint32_t UTF8_3B_MAX = 0xFFFF;
constexpr inline std::uint32_t UTF8_4B_MAX = 0x10FFFF;
constexpr inline std::uint32_t UTF16_SURROGATE_MIN = 0xD800;
constexpr inline std::uint32_t UTF16_SURROGATE_MAX = 0xDFFF;

constexpr inline std::uint8_t UTF8_2B_PRE = 0xC0;
constexpr inline std::uint8_t UTF8_3B_PRE = 0xE0;
constexpr inline std::uint8_t UTF8_4B_PRE = 0xF0;
constexpr inline std::uint8_t UTF8_CONT_PRE = 0x80;

constexpr inline std::uint8_t UTF8_2B_MASK = 0x1F;
constexpr inline std::uint8_t UTF8_3B_MASK = 0x0F;
constexpr inline std::uint8_t UTF8_4B_MASK = 0x07;
constexpr inline std::uint8_t UTF8_CONT_MASK = 0x3F;

constexpr inline std::uint8_t UTF8_CONT_SHIFT = 6;
constexpr inline std::uint8_t UTF8_2B_SHIFT = 12;
constexpr inline std::uint8_t UTF8_3B_SHIFT = 18;

namespace {
// Code point helpers
constexpr bool isSurrogate(char32_t codepoint) noexcept {
    return UTF16_SURROGATE_MIN <= codepoint && codepoint <= UTF16_SURROGATE_MAX;
}
constexpr bool isValidCodePoint(char32_t codepoint) noexcept {
    return static_cast<std::uint32_t>(codepoint) <= UTF8_4B_MAX && !isSurrogate(codepoint);
}

// Hex helpers

constexpr int hexDigitToInt(char c) noexcept { return mnstl::detail::_chtoi(c); }

std::optional<char32_t> parseHexCodePoint(std::string_view digits, std::size_t line, std::size_t col) {
    if (digits.empty()) {
        logging::logError(line, col, "Unicode code point cannot be empty");
        return std::nullopt;
    }
    std::uint32_t codepoint = 0;

    for (char c : digits) {
        std::uint32_t value;

        if (!isxdigit(c)) {
            logging::logError(line, col, "Invalid hexadecimal digit '{}'", c);
            return std::nullopt;
        }
        value = static_cast<std::uint32_t>(hexDigitToInt(c));
        codepoint = (codepoint << 4) | value;
    }

    const auto result = static_cast<char32_t>(codepoint);
    if (!isValidCodePoint(result)) {
        logging::logError(line, col, "Invalid unicode code point: U+{:X}", codepoint);
        return std::nullopt;
    }
    return result;
}

// Unicode -> UTF8

std::optional<std::string> encodeUTF8(char32_t codepoint) {
    const auto p = static_cast<std::uint32_t>(codepoint);

    if (!isValidCodePoint(codepoint)) { return std::nullopt; }

    if (p <= UTF8_1B_MAX) { return std::string{static_cast<char>(p)}; }
    if (p <= UTF8_2B_MAX) {
        return std::string{static_cast<char>(UTF8_2B_PRE | (p >> UTF8_2B_SHIFT)),
                           static_cast<char>(UTF8_CONT_PRE | (p & UTF8_CONT_MASK))};
    }
    if (p <= UTF8_3B_MAX) {
        return std::string{static_cast<char>(UTF8_3B_PRE | (p >> UTF8_2B_SHIFT)),
                           static_cast<char>(UTF8_CONT_PRE | ((p >> UTF8_CONT_SHIFT) & UTF8_CONT_MASK)),
                           static_cast<char>(UTF8_CONT_PRE | (p & UTF8_CONT_MASK))};
    }

    return std::string{static_cast<char>(UTF8_4B_PRE | (p >> UTF8_3B_SHIFT)),
                       static_cast<char>(UTF8_CONT_PRE | ((p >> UTF8_2B_SHIFT) & UTF8_CONT_MASK)),
                       static_cast<char>(UTF8_CONT_PRE | ((p >> UTF8_CONT_SHIFT) & UTF8_CONT_MASK)),
                       static_cast<char>(UTF8_CONT_PRE | (p & UTF8_CONT_MASK))};
}

// Escape sequences

std::optional<char32_t> getEscapeCharacter(char escapeChar, std::size_t line, std::size_t col) {
    switch (escapeChar) {
        // \, ', ", abfnrtv0
        case '\\': return U'\\';
        case '\'': return U'\'';
        case '"': return U'"';
        case 'a': return U'\a';
        case 'b': return U'\b';
        case 'f': return U'\f';
        case 'n': return U'\n';
        case 'r': return U'\r';
        case 't': return U'\t';
        case 'v': return U'\v';
        case '0': return U'\0';
        default:
            logging::logError(
                line, col,
                "\\{} is not a valid escape sequence. If you meant to type a backslash ('\\'), use two backslashes",
                escapeChar);
            return std::nullopt;
    }
}

std::optional<char32_t> parseEscapeSequence(std::string_view input, std::size_t& index, std::size_t line,
                                            std::size_t column) {
    // Index should point at the backslash
    if (index >= input.size() || input[index] != '\\') {
        logging::logError(line, column, R"(Expected '\\' to begin escape sequence)");
        return std::nullopt;
    }
    ++index;

    // a backslash at the end of the input is an incomplete escape sequence
    if (index >= input.size()) {
        logging::logError(line, column, "Incomplete escape sequence");
        return std::nullopt;
    }

    const char escapeChar = input[index];

    if (tolower(escapeChar) == 'u') {
        ++index;  // skip the u
        const std::size_t digitsStart = index;
        std::size_t digitCount = 0;

        while (index < input.size() && digitCount < 9) {
            if (!isxdigit(input[index])) { break; }
            ++index;
            ++digitCount;
        }

        if (digitCount != 4 && digitCount != 8) {
            logging::logError(line, column, "Unicode escape sequence must contain exactly 4 or 8 hexadecimal digits");
            return std::nullopt;
        }

        const std::string_view digits = input.substr(digitsStart, digitCount);
        return parseHexCodePoint(digits, line, column);
    }

    // regular escape sequence
    const auto result = getEscapeCharacter(escapeChar, line, column);
    if (result) { ++index; }
    return result;
}

}  // namespace

std::optional<DecodedUTF8> Lexer::decodeUTF8(std::string_view input, std::size_t offset) {
    if (offset >= input.size()) { return std::nullopt; }
    const auto byte = [](char c) -> std::uint8_t { return static_cast<std::uint8_t>(static_cast<unsigned char>(c)); };

    const std::uint8_t first = byte(input[offset]);
    if (first <= UTF8_1B_MAX) { return DecodedUTF8{.codepoint = static_cast<char32_t>(first), .bytecount = 1}; }

    std::size_t bytecount;
    std::uint32_t codepoint;

    if ((first & 0xE0) == UTF8_2B_PRE) {
        bytecount = 2;
        codepoint = first & UTF8_2B_MASK;
    } else if ((first & 0xF0) == UTF8_3B_PRE) {
        bytecount = 3;
        codepoint = first & UTF8_3B_MASK;
    } else if ((first & 0xF8) == UTF8_4B_PRE) {
        bytecount = 4;
        codepoint = first & UTF8_4B_MASK;
    } else {
        return std::nullopt;
    }

    // make sure the entire byte sequence is present
    if (offset + bytecount > input.size()) { return std::nullopt; }

    // decode continuation bytes
    for (std::size_t i = 1; i < bytecount; ++i) {
        const std::uint8_t current = byte(input[offset + i]);
        if ((current & 0xC0) != UTF8_CONT_PRE) { return std::nullopt; }
        codepoint = (codepoint << UTF8_CONT_SHIFT) | (current & UTF8_CONT_MASK);
    }

    const auto result = static_cast<char32_t>(codepoint);

    // Reject overlong encodings
    if (bytecount == 2 && codepoint <= UTF8_1B_MAX) { return std::nullopt; }
    if (bytecount == 3 && codepoint <= UTF8_2B_MAX) { return std::nullopt; }
    if (bytecount == 4 && codepoint <= UTF8_3B_MAX) { return std::nullopt; }
    if (!isValidCodePoint(result)) { return std::nullopt; }
    return DecodedUTF8{.codepoint = codepoint, .bytecount = bytecount};
}

std::optional<std::string> Lexer::resolveEscapeCharacters(std::string_view escapeString) {
    std::string result;
    result.reserve(escapeString.size());

    std::size_t index = 0;
    while (index < escapeString.size()) {
        if (escapeString[index] != '\\') {
            result += escapeString[index++];
            continue;
        }
        const std::optional<char32_t> codepoint = parseEscapeSequence(escapeString, index, getLine(), getCol());

        if (!codepoint) { return std::nullopt; }

        const auto encoded = encodeUTF8(*codepoint);

        if (!encoded) { return std::nullopt; }
        result += *encoded;
    }
    return result;
}

Result Lexer::processCharEscapeSequence(std::string_view charLiteral) {
    std::size_t index = 0;

    const auto codepoint = parseEscapeSequence(charLiteral, index, getLine(), getCol());

    if (!codepoint) {
        emitToken(TokenType::CharLiteral, std::string(charLiteral), true);
        return Result::Failure;
    }
    if (index != charLiteral.size()) {
        logError("character literal must contain exactly one character");

        emitToken(TokenType::CharLiteral, std::string(charLiteral), true);
        return Result::Failure;
    }

    auto encoded = encodeUTF8(*codepoint);
    if (!encoded) {
        emitToken(TokenType::CharLiteral, std::string(charLiteral), true);
        return Result::Failure;
    }
    emitToken(TokenType::CharLiteral, std::move(*encoded), false);
    return Result::Success;
}

}  // namespace Manganese::lexer