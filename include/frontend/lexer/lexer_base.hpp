

/**
 * @file lexer_base.hpp
 * @brief Defines the base Lexer class and related utilities for tokenizing source code in the Manganese frontend.
 *
 * The lexer is responsible for converting source code (from a string or file)
 * into a stream of tokens that can be consumed by the parser. It includes helper functions for handling escape
 * sequences, Unicode encoding, and number literal parsing.
 * The lexer contains an io::Reader object to read the source code
 */
#ifndef MANGANESE_INCLUDE_FRONTEND_LEXER_LEXER_BASE_HPP
#define MANGANESE_INCLUDE_FRONTEND_LEXER_LEXER_BASE_HPP

#include <deque>
#include <frontend/lexer/token.hpp>
#include <functional>
#include <global_macros.hpp>
#include <io/filereader.hpp>
#include <io/reader.hpp>
#include <io/stringreader.hpp>
#include <memory>
#include <optional>
#include <string>
#include <utils/number_utils.hpp>

namespace Manganese {
namespace lexer {
constexpr auto NONE = std::nullopt;

enum class Mode {
    String = 's',  // Source code passed in as a string
    File = 'f'  // Filename passed in
};

struct NumberPrefixResult {
    Base base;
    std::function<bool(char)> isValidBaseChar;
    std::string prefix;
};

enum class TokenizationResult {
    Success,
    Failure,
};

//~ Static helper functions

/**
 * @brief Map a character to its corresponding escape sequence (e.g. 'n' -> '\n')
 * @param escapeChar The character to map
 * @return An optional character representing the escape sequence, or NONE if the character is not a valid escape
 * sequence
 */
std::optional<char> getEscapeCharacter(const char escapeChar, size_t line, size_t col);

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
std::optional<char32_t> resolveUnicodeCharacters(const std::string& escDigits, size_t line, size_t col,
                                                 bool isLongUnicode = false);

/**
 * @brief The lexer is responsible for turning the source code into a non-textual representation that the parser can
 * understand.
 */
class Lexer {
   public:  // public variables
   private:  // private variables
    std::unique_ptr<io::Reader> reader;
    size_t tokenStartLine, tokenStartCol;  // Keep track of where the token started for error reporting
    constexpr static const size_t QUEUE_LOOKAHEAD_AMOUNT = 8;  // how many tokens to look ahead
    bool hasCriticalError_ = false;
    bool hasError_ = false;
    std::deque<Token> tokenStream;

   public:  // public methods
    explicit Lexer(const std::string& source, const Mode mode = Mode::File);
    ~Lexer() noexcept = default;

    // Avoid file ownership issues
    Lexer(const Lexer&) = delete;
    Lexer(Lexer&&) = delete;
    Lexer& operator=(const Lexer&) = delete;
    Lexer& operator=(Lexer&&) = delete;

    /**
     * @brief See the next token in the input stream without consuming it
     * @return The peeked token
     * @details This function will not advance the reader position
     */
    Token peekToken() noexcept;

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

    constexpr bool hasCriticalError() const noexcept { return hasCriticalError_; }
    constexpr bool hasError() const noexcept { return hasError_; }

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
    TokenizationResult tokenizeCharLiteral();

    /**
     * @brief Process any sequence of alphanumeric characters and underscores
     * @details If the sequence is a keyword (e.g. "if"), it will be tokenized as such
     */
    TokenizationResult tokenizeKeywordOrIdentifier();

    /**
     * @brief Process a number literal and generate a token
     */
    TokenizationResult tokenizeNumber();

    /**
     * @brief Skip over a block comment
     * @note Allows for nested block comments
     */
    TokenizationResult skipBlockComment();

    /**
     * @brief Process a string literal and generate a token. Triggered when a double quote (") is encountered
     */
    TokenizationResult tokenizeStringLiteral();

    /**
     * @brief Process any character that is neither alphanumeric, a number, an underscore, quotes or whitespace (e.g. +)
     */
    TokenizationResult tokenizeSymbol();

    //~ Helper functions

    /**
     * @brief Processes the prefix of a number literal to determine its base.
     * @details The valid prefixes are 0b for binary, 0o for octal and 0x for hexadecimal
     * @return The base of the number literal (decimal/hexadecimal/octal/binary),
     * a function that checks if a character is valid for the base and a string that contains the prefix
     */
    NumberPrefixResult processNumberPrefix();

    /**
     * @brief Process the suffix of a number literal (e.g., 'f' for float)
     * @param base The base of the number literal
     * @param numberLiteral The lexeme for the number literal (the base prefix will be appended if there is one)
     * @param isFloat Whether the number literal is a float (e.g., 1.23f)
     */
    TokenizationResult processNumberSuffix(Base base, std::string& numberLiteral, bool isFloat);

    /**
     * @brief Replaces raw escape sequences in a string with their corresponding characters (e.g. "\n" (literally)
     * becomes a newline)
     * @param escapeString The string containing escape sequences
     * @return An optional string with the escape sequences resolved, or NONE if the string is invalid
     */
    std::optional<std::string> resolveEscapeCharacters(const std::string& escapeString);

    /**
     * @brief Helper function specifically to handle escape sequences in char literals
     * @param charLiteral The char literal to process
     */
    TokenizationResult processCharEscapeSequence(const std::string& charLiteral);

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
    inline void advance(size_t n = 1) noexcept { reader->setPosition(reader->getPosition() + n); }
};
}  // namespace lexer
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_LEXER_LEXER_BASE_HPP
