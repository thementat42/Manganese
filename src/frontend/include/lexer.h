#ifndef LEXER_H
#define LEXER_H

#include <functional>
#include <memory>
#include <optional>
#include <queue>

#include "../../global_macros.h"
#include "../../io/include/filereader.h"
#include "../../io/include/reader.h"
#include "../../io/include/stringreader.h"
#include "token.h"

MANGANESE_BEGIN
namespace lexer {
using str = std::string;
using std::optional;
constexpr auto NONE = std::nullopt;

// ~ Constants for UTF-8 encoding
constexpr uint32_t UTF8_1B_MAX = 0x7F;
constexpr uint32_t UTF8_2B_MAX = 0x7FF;
constexpr uint32_t UTF8_3B_MAX = 0xFFFF;
constexpr uint32_t UTF8_4B_MAX = 0x10FFFF;

constexpr uint8_t UTF8_2B_PRE = 0xC0;
constexpr uint8_t UTF8_3B_PRE = 0xE0;
constexpr uint8_t UTF8_4B_PRE = 0xF0;
constexpr uint8_t UTF8_CONT_PRE = 0x80;

constexpr uint8_t UTF8_1B_MASK = 0x7F;
constexpr uint8_t UTF8_2B_MASK = 0x1F;
constexpr uint8_t UTF8_3B_MASK = 0x0F;
constexpr uint8_t UTF8_4B_MASK = 0x07;
constexpr uint8_t UTF8_CONT_MASK = 0x3F;

constexpr uint8_t UTF8_CONT_SHIFT = 6;
constexpr uint8_t UTF8_2B_SHIFT = 12;
constexpr uint8_t UTF8_3B_SHIFT = 18;

enum class Mode {
    String,  // Source code passed in as a string
    File     // Filename passed in
};

enum NumberLiteralBase {
    Binary = 2,
    Octal = 8,
    Decimal = 10,
    Hexadecimal = 16
};

//~ Static helper functions

/**
 * @brief Map a character to its corresponding escape sequence (e.g. 'n' -> '\n')
 * @param escapeChar The character to map
 * @return An optional wide character representing the escape sequence, or NONE if the character is not a valid escape sequence
 */
optional<wchar_t> getEscapeCharacter(const char& escapeChar);

/**
 * @brief Convert a wide character to a UTF-8 encoded string
 * @param wideChar The wide character to convert
 * @return A string containing the UTF-8 encoded representation of the wide character
 */
std::string convertWideCharToUTF8(wchar_t wideChar);

class Lexer {
   private:  // private variables
    std::unique_ptr<io::Reader> reader;
    size_t tokenStartLine, tokenStartCol;                      // Keep track of where the token started for error reporting
    constexpr static const size_t QUEUE_LOOKAHEAD_AMOUNT = 8;  // how many tokens to look ahead

   public:  // public variables
    std::deque<Token> tokenStream;

   private:  // private methods
    //~ Main tokenization functions

    /**
     * @brief Generates a certain number of tokens. Holds the main tokenization loop
     * @param numTokens The number of tokens to generate (default is 1)
     */
    void lex(size_t numTokens = 1);

    /**
     * @brief Process a character literal and generate a token. Triggered when a single quote (') is encountered
     */
    void tokenizeCharLiteral();

    /**
     * @brief Process a string literal and generate a token. Triggered when a double quote (") is encountered
     */
    void tokenizeStringLiteral();

    /**
     * @brief Process a number literal and generate a token
     */
    void tokenizeNumber();

    /**
     * @brief Process any sequence of alphanumeric characters and underscores
     * @details If the sequence is a keyword (e.g. "if"), it will be tokenized as such
     */
    void tokenizeKeywordOrIdentifier();

    /**
     * @brief Process any character that is neither alphanumeric, a number, an underscore, quotes or whitespace
     */
    void tokenizeSymbol();

    //~ Helper functions

    /**
     * @brief Processes the prefix of a number literal to determine its base.
     * @details The valid prefixes are 0b for binary, 0o for octal and 0x for hexadecimal
     * @param isValidBaseChar Output parameter: A reference to a function or lambda that takes a character and returns true
     *                        if the character is valid for the intended base, false otherwise.
     * @param numberLiteral Output parameter: The lexeme for the number literal (the base prefix will be appended if there is one)
     * @return NumberLiteralBase The detected base of the number literal (e.g., decimal, hexadecimal, binary).
     */
    NumberLiteralBase processNumberPrefix(std::function<bool(char)>& isValidBaseChar, str& numberLiteral);

    /**
     * @brief Process the suffix of a number literal (e.g., 'f' for float)
     * @param base The base of the number literal
     * @param numberLiteral The lexeme for the number literal (the base prefix will be appended if there is one)
     * @param isFloat Whether the number literal is a float (e.g., 1.23f)
     * @return True if the suffix was processed successfully, false otherwise
     */
    bool processNumberSuffix(NumberLiteralBase base, str& numberLiteral, bool isFloat);

    optional<wchar_t> resolveHexAndUnicodeCharacters(const str& esc, const bool& isUnicode, size_t& skipLength);

    std::optional<str> resolveEscapeCharacters(const str& escapeString);

    /**
     * @brief Helper function specifically to handle escape sequences in char literals
     * @param charLiteral The char literal to process
     */
    void processCharEscapeSequence(const str& charLiteral);

    //~ Reader wrapper functions

    /**
     * @brief See the next character in the input stream without consuming it
     * @param offset How many characters to look ahead (default is 0 -- the current character)
     * @return The peeked character
     */
    inline char peekChar(size_t offset = 0) noexcept { return reader->peekChar(offset); }

    /**
     * @brief Consume the next character in the input stream
     * @details This will advance the reader position by 1
     * @return The consumed character
     */
    [[nodiscard]] inline char consumeChar() noexcept { return reader->consumeChar(); }

    /**
     * @brief Get the current line in the input stream
     * @return The current line number
     */
    inline size_t getLine() noexcept { return reader->getLine(); }

    /**
     * @brief Get the current column in the input stream
     * @return The current column number
     */
    inline size_t getCol() noexcept { return reader->getColumn(); }

    /**
     * @brief Move forward in the input stream by a certain number of characters
     * @param n The number of characters to move forward (default is 1)
     */
    inline void advance(size_t n = 1) noexcept {
        reader->setPosition(reader->getPosition() + n);
    }
    /**
     * @brief Check if the end of the input stream has been reached
     * @return True if the end of the stream has been reached, false otherwise
     */
    inline bool done() noexcept { return reader->done(); }

   public:  // public methods
    Lexer(const str& source, const Mode mode = Mode::File);
    ~Lexer() noexcept = default;

    /**
     * @brief See the next token in the input stream without consuming it
     * @param offset How many tokens to look ahead (default is 0 -- the current token)
     * @return The peeked token
     * @details This function will not advance the reader position
     */
    Token peekToken(size_t offset = 0) noexcept;

    /**
     * @brief Consume the next token in the input stream
     * @return The consumed token
     * @details This function will advance the reader position by 1
     */
    Token consumeToken() noexcept;
};
}  // namespace lexer
MANGANESE_END

#endif  // LEXER_H