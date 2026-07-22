#ifndef MANGANESE_INCLUDE_FRONTEND_LEXER_LEXER_BASE_HPP
#define MANGANESE_INCLUDE_FRONTEND_LEXER_LEXER_BASE_HPP

#include <core.hpp>
#include <deque>
#include <frontend/lexer/token.hpp>
#include <io/filereader.hpp>
#include <io/reader.hpp>
#include <io/stringreader.hpp>
#include <memory>
#include <mnstl/number.hxx>
#include <optional>
#include <string>
#include <utils/result.hpp>

namespace Manganese {
namespace lexer {

enum class Mode : uint8_t {
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
    size_t tokenStartLine, tokenStartCol;  // Keep track of where the token started for error reporting
    constexpr static const size_t QUEUE_LOOKAHEAD_AMOUNT = 8;  // how many tokens to look ahead
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

    Token peekToken() noexcept;
    Token consumeToken() noexcept;
    inline bool done() const noexcept { return reader->done(); }
    constexpr bool hasError() const noexcept { return _hasError; }

   private:
    //~ Main tokenization functions

    void lex(size_t numTokens = 1);
    Result tokenizeCharLiteral();
    Result tokenizeKeywordOrIdentifier();
    Result tokenizeNumber();
    Result skipBlockComment();
    Result tokenizeStringLiteral();
    Result tokenizeSymbol();

    //~ Helper functions
    NumberPrefixResult processNumberPrefix();
    Result processNumberSuffix(mnstl::Base base, std::string& numberLiteral, bool isFloat);
    std::optional<std::string> resolveEscapeCharacters(const std::string& escapeString);
    Result processCharEscapeSequence(const std::string& charLiteral);

    //~ Reader wrapper functions
    inline char peekChar(size_t offset = 0) const noexcept { return reader->peekChar(offset); }
    [[nodiscard]] inline char consumeChar() const noexcept { return reader->consumeChar(); }
    inline size_t getLine() const noexcept { return reader->getLine(); }
    inline size_t getCol() const noexcept { return reader->getColumn(); }
    inline void advance(size_t n = 1) noexcept { reader->setPosition(reader->getPosition() + n); }
};

//~ Static helper functions

constexpr bool isbdigit(char c) noexcept { return c == '0' || c == '1'; }
constexpr bool isdigit(char c) noexcept { return (c >= '0' && c <= '9'); }
constexpr bool isodigit(char c) { return (c >= '0' && c <= '7'); }
constexpr bool isxdigit(char c) noexcept { return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
constexpr bool isalpha(char c) noexcept { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
constexpr bool isspace(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}
constexpr char tolower(char c) noexcept {
    if (c >= 'A' && c <= 'Z') { return c - 'A' + 'a'; }
    return c;
}

std::optional<char> getEscapeCharacter(const char escapeChar, size_t line, size_t col);
std::string encodeUTF8String(char32_t wideChar);
std::optional<char32_t> resolveHexCharacters(const std::string& escDigits);
std::optional<char32_t> resolveUnicodeCharacters(const std::string& escDigits, size_t line, size_t col,
                                                 bool isLongUnicode = false);

}  // namespace lexer
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_LEXER_LEXER_BASE_HPP
