#ifndef MANGANESE_INCLUDE_FRONTEND_LEXER_LEXER_BASE_HPP
#define MANGANESE_INCLUDE_FRONTEND_LEXER_LEXER_BASE_HPP

#include <core.hpp>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <format>
#include <frontend/lexer/token.hpp>
#include <io/logging.hpp>
#include <io/reader.hpp>
#include <memory>
#include <mnstl/number.hxx>
#include <optional>
#include <string>
#include <utility>
#include <utils/result.hpp>

namespace Manganese::lexer {

enum class Mode : std::uint8_t {
    String = 's',  // Source code passed in as a string
    File = 'f'  // Filename passed in
};

struct NumberPrefixResult {
    mnstl::Base base;
    bool (*isValidBaseChar)(char);
    std::string prefix;
};

struct DecodedUTF8 {
    char32_t codepoint;
    std::size_t bytecount;
};

/**
 * @brief The lexer is responsible for turning the source code into a non-textual representation that the parser can
 * understand.
 */
class Lexer {
   private:
    std::unique_ptr<io::Reader> reader;
    std::size_t tokenStartLine, tokenStartCol;  // Keep track of where the token started for error reporting
    constexpr static std::size_t QUEUE_LOOKAHEAD_AMOUNT = 8;  // how many tokens to look ahead
    bool _hasError = false;
    std::deque<Token> tokenStream;

   public:
    explicit Lexer(const std::string& source, Mode mode = Mode::File);
    ~Lexer() noexcept = default;

    // Avoid file ownership issues
    Lexer(const Lexer&) = delete;
    Lexer(Lexer&&) = delete;
    Lexer& operator=(const Lexer&) = delete;
    Lexer& operator=(Lexer&&) = delete;

    Token& peekToken();
    Token consumeToken();
    inline bool done() noexcept { return reader->done(); }
    bool hasError() const noexcept { return _hasError; }

   private:
    //~ Main tokenization functions

    void lex(std::size_t numTokens = 1);
    Result tokenizeCharLiteral();
    Result tokenizeKeywordOrIdentifier();
    Result tokenizeNumber();
    Result skipBlockComment();
    Result tokenizeStringLiteral();
    Result tokenizeSymbol();

    //~ Helper functions
    void emitToken(TokenType type, std::string&& lexeme, bool invalid);
    NumberPrefixResult processNumberPrefix();
    Result processScientificNotation(std::string& numberLiteral);
    Result processNumberSuffix(std::string& numberLiteral, bool isFloat);
    std::optional<std::string> resolveEscapeCharacters(std::string_view escapeString);
    static std::optional<DecodedUTF8> decodeUTF8(std::string_view input, std::size_t offset);
    Result processCharEscapeSequence(std::string_view charLiteral);

    template <class... Args>
    void logError(std::format_string<Args...> message, Args&&... args) const {
        logging::logError(getLine(), getCol(), message, std::forward<Args>(args)...);
    }
    //~ Reader wrapper functions
    inline char peekChar(std::size_t offset = 0) noexcept { return reader->peekChar(offset); }
    [[nodiscard]] inline char consumeChar() const noexcept { return reader->consumeChar(); }
    inline std::size_t getLine() const noexcept { return reader->getLine(); }
    inline std::size_t getCol() const noexcept { return reader->getColumn(); }
    inline void advance(std::size_t n = 1) noexcept { reader->setPosition(reader->getPosition() + n); }
};

//~ Static helper functions

std::optional<std::string> encodeUTF8(char32_t);

inline std::string codepointToUTF8(char32_t codepoint) {
    if (codepoint <= 0x7F) { return std::string(1, static_cast<char>(codepoint)); }
    auto encoded = lexer::encodeUTF8(codepoint);
    return encoded ? *encoded : "";
}

constexpr bool is_bdigit(char c) noexcept { return c == '0' || c == '1'; }
constexpr bool is_digit(char c) noexcept { return BETWEEN(c, '0', '9'); }
constexpr bool is_odigit(char c) { return BETWEEN(c, '0', '7'); }
constexpr bool is_xdigit(char c) noexcept { return is_digit(c) || BETWEEN(c, 'a', 'f') || BETWEEN(c, 'A', 'F'); }
constexpr bool is_alpha(char c) noexcept { return BETWEEN(c, 'a', 'z') || BETWEEN(c, 'A', 'Z'); }
constexpr bool is_alphanumeric(char c) noexcept { return is_alpha(c) || is_digit(c); }
constexpr bool is_whitespace(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}
constexpr char to_lowercase(char c) noexcept { return BETWEEN(c, 'A', 'Z') ? (c + 32) : c; }

}  // namespace Manganese::lexer

#endif  // MANGANESE_INCLUDE_FRONTEND_LEXER_LEXER_BASE_HPP
