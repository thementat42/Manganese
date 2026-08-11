#ifndef MANGANESE_INCLUDE_FRONTEND_LEXER_LEXER_BASE_HPP
#define MANGANESE_INCLUDE_FRONTEND_LEXER_LEXER_BASE_HPP

#include <cstddef>
#include <cstdint>
#include <deque>
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

    Token peekToken();
    Token consumeToken();
    inline bool done() const noexcept { return reader->done(); }
    constexpr bool hasError() const noexcept { return _hasError; }

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
    template <class... Args>
    void logError(std::format_string<Args...> message, Args&&... args) const {
        logging::logError(getLine(), getCol(), message, std::forward<Args>(args)...);
    }
    void emitToken(TokenType type, std::string&& lexeme, bool invalid);
    NumberPrefixResult processNumberPrefix();
    Result processScientificNotation(std::string& numberLiteral);
    Result processNumberSuffix(std::string& numberLiteral, bool isFloat);
    std::optional<std::string> resolveEscapeCharacters(const std::string& escapeString);
    Result processCharEscapeSequence(const std::string& charLiteral);

    //~ Reader wrapper functions
    inline char peekChar(std::size_t offset = 0) const noexcept { return reader->peekChar(offset); }
    [[nodiscard]] inline char consumeChar() const noexcept { return reader->consumeChar(); }
    inline std::size_t getLine() const noexcept { return reader->getLine(); }
    inline std::size_t getCol() const noexcept { return reader->getColumn(); }
    inline void advance(std::size_t n = 1) noexcept { reader->setPosition(reader->getPosition() + n); }
};

//~ Static helper functions

constexpr bool isbdigit(char c) noexcept { return c == '0' || c == '1'; }
constexpr bool isdigit(char c) noexcept { return (c >= '0' && c <= '9'); }
constexpr bool isodigit(char c) { return (c >= '0' && c <= '7'); }
constexpr bool isxdigit(char c) noexcept { return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
constexpr bool isalpha(char c) noexcept { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
constexpr bool isalnum(char c) noexcept { return isalpha(c) || isdigit(c); }
constexpr bool isspace(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}
constexpr char tolower(char c) noexcept {
    if (c >= 'A' && c <= 'Z') { return c - 'A' + 'a'; }
    return c;
}

}  // namespace Manganese::lexer

#endif  // MANGANESE_INCLUDE_FRONTEND_LEXER_LEXER_BASE_HPP
