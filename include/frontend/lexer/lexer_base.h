#ifndef MANGANESE_INCLUDE_FRONTEND_LEXER_LEXER_BASE_H
#define MANGANESE_INCLUDE_FRONTEND_LEXER_LEXER_BASE_H

#include <global_macros.h>
#include <io/filereader.h>
#include <io/reader.h>
#include <io/stringreader.h>

#include <functional>
#include <memory>
#include <optional>
#include <deque>
#include <string>

#include "token.h"

namespace Manganese {
namespace lexer {
constexpr auto NONE = std::nullopt;

enum class Mode {
    String = 's',  // Source code passed in as a string
    File = 'f'     // Filename passed in
};

enum class Base {
    Binary = 2,       // 0b prefix
    Octal = 8,        // 0o prefix
    Decimal = 10,     // Default base, no prefix
    Hexadecimal = 16  // 0x prefix
};

//~ Static helper functions

/**
 * @brief Map a character to its corresponding escape sequence (e.g. 'n' -> '\n')
 * @param escapeChar The character to map
 * @return An optional character representing the escape sequence, or NONE if the character is not a valid escape sequence
 */
std::optional<char> getEscapeCharacter(const char escapeChar);

/**
 * @brief Convert a wide character to a UTF-8 encoded string
 * @param wideChar The wide character to convert
 * @return A string containing the UTF-8 encoded representation of the wide character
 */
std::string encodeUTF8String(char32_t wideChar);

/**
 * @brief Resolves escape sequences of the form \xXX
 * @param escDigits The hexadecimal digits following the escape sequence (e.g. "1F")
 * @return An optional character representing the resolved escape sequence, or NONE if the sequence is invalid
 */
std::optional<char32_t> resolveHexCharacters(const std::string& escDigits);

/**
 * @brief Resolves escape sequences of the form \uXXXX or \UXXXXXXXX
 * @param escDigits The hexadecimal digits following the escape sequence (e.g. "1F4A9" for \u1F4A9)
 * @param isLongUnicode Whether the escape sequence is a long Unicode escape sequence (\UXXXXXXXX)
 * @return An optional character representing the resolved escape sequence, or NONE if the sequence is invalid
 */
std::optional<char32_t> resolveUnicodeCharacters(const std::string& escDigits, bool isLongUnicode = false);

/**
 * @brief The lexer is responsible for turning the source code into a non-textual representation that the parser can understand.
 */
class Lexer {
   private:  // private variables
    std::unique_ptr<io::Reader> reader;
    size_t tokenStartLine, tokenStartCol;                      // Keep track of where the token started for error reporting
    constexpr static const size_t QUEUE_LOOKAHEAD_AMOUNT = 8;  // how many tokens to look ahead
    bool hasCriticalError_ = false;

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
     * @brief Process any character that is neither alphanumeric, a number, an underscore, quotes or whitespace (e.g. +)
     */
    void tokenizeSymbol();

    //~ Helper functions

    /**
     * @brief Processes the prefix of a number literal to determine its base.
     * @details The valid prefixes are 0b for binary, 0o for octal and 0x for hexadecimal
     * @param isValidBaseChar Output parameter: A reference to a function or lambda that takes a character and returns true
     *                        if the character is valid for the intended base, false otherwise.
     * @param numberLiteral Output parameter: The lexeme for the number literal (the base prefix will be appended if there is one)
     * @return Base The base of the number literal (decimal/hexadecimal/octal/binary).
     */
    Base processNumberPrefix(std::function<bool(char)>& isValidBaseChar, std::string& numberLiteral);

    /**
     * @brief Process the suffix of a number literal (e.g., 'f' for float)
     * @param base The base of the number literal
     * @param numberLiteral The lexeme for the number literal (the base prefix will be appended if there is one)
     * @param isFloat Whether the number literal is a float (e.g., 1.23f)
     * @return True if the suffix was processed successfully, false otherwise
     */
    bool processNumberSuffix(Base base, std::string& numberLiteral, bool isFloat);

    /**
     * @brief Replaces raw escape sequences in a string with their corresponding characters (e.g. "\n" (literally) becomes a newline)
     * @param escapeString The string containing escape sequences
     * @return An optional string with the escape sequences resolved, or NONE if the string is invalid
     */
    std::optional<std::string> resolveEscapeCharacters(const std::string& escapeString);

    /**
     * @brief Helper function specifically to handle escape sequences in char literals
     * @param charLiteral The char literal to process
     */
    void processCharEscapeSequence(const std::string& charLiteral);

    //~ Reader wrapper functions

    /**
     * @brief See the next character in the input stream without consuming it
     * @param offset How many characters to look ahead (default is 0 -- the current character)
     * @return The peeked character
     */
    inline char peekChar(size_t offset = 0) const noexcept { return reader->peekChar(offset); }

    /**
     * @brief Consume the next character in the input stream
     * @details This will advance the reader position by 1
     * @return The consumed character
     */
    [[nodiscard]] inline char consumeChar() const noexcept { return reader->consumeChar(); }

    /**
     * @brief Get the current line in the input stream
     * @return The current line number
     */
    inline size_t getLine() const noexcept { return reader->getLine(); }

    /**
     * @brief Get the current column in the input stream
     * @return The current column number
     */
    inline size_t getCol() const noexcept { return reader->getColumn(); }

    /**
     * @brief Move forward in the input stream by a certain number of characters
     * @param n The number of characters to move forward (default is 1)
     */
    inline void advance(size_t n = 1) noexcept {
        reader->setPosition(reader->getPosition() + n);
    }

   public:  // public methods
    explicit Lexer(const std::string& source, const Mode mode = Mode::File);
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

    /**
     * @brief Check if the end of the input stream has been reached
     * @return True if the end of the stream has been reached, false otherwise
     */
    bool done() const noexcept { return reader->done(); }

    bool hasCriticalError() const noexcept { return hasCriticalError_; }
};
}  // namespace lexer
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_LEXER_LEXER_BASE_H
