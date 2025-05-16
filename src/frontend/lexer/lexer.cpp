/**
 * @file lexer.cpp
 * @brief This file contains the implementation of the lexer for the Manganese compiler.
 *
 * The lexer takes the source code and splits it into tokens (see token.h)
 * These tokens are then passed on to the parser to build the AST.
 *
 * Note: all angle brackets are tokenized as angle brackets, even if they are actually comparison or bitwise shift operators.
 * This is because determining which operator it is would require looking ahead in the source code, which is not possible in a single-pass lexer.
 * The parser will handle this ambiguity and determine the correct operator.
 */
#include "include/lexer.h"

#include <algorithm>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../../global_macros.h"
#include "../../io/include/filereader.h"
#include "../../io/include/reader.h"
#include "../../io/include/stringreader.h"
#include "include/keywords.h"
#include "include/operators.h"
#include "include/token.h"

/*
~ Some ambiguous cases to consider  -- cases where a character could map to more than one operator
~ If any of these occur while inside a string literal or in a char, obviously don't do anything
* Ambiguous case 1: angle brackets (`<` and `>`)
`<` and `>` use cases:
- comparisons (<, >)
- comparisons with equality (<=, >=)
- bitwise shifts (<<, >>)
- generics (func<int>)

when < or > is seen, check next char:
if it's an =, push that as one operator
otherwise, push it as an angle bracket (the the comparison vs shift vs generic is handled in the parser since it requires more context, like "is this a function identifier")


* Ambiguous case 2: Any logical operator (&, |, !, ^)
The bitwise operators are just this + a tilde (~)
Another easy thing to handle:
If the current char is a logical operator, look at the next char
- if it's a ~, push that as one bitwise operator
- otherwise push it as a regular binary operator
- for !, if it's an equals sign, push that as an assignment operator

* Ambiguous case 3: `+`
`+` use cases:
- unary plus
- addition (or whatever that's overloaded to for the type)
- increment (++)

- if the next token is a `+`, it's an increment, push that as one operator
- if the previous token is an identifier or a literal, it's addition
// - if the previous token is an operator or an opening parenthesis, it's unary plus
^ Handled during AST parsing, not tokenizing

* Ambiguous case 4: `-`
`-` use cases:
- unary minus
- subtraction (or whatever that's overloaded to for the type)
- decrement
- arrow (->)

- if the next token is a >, it's ->, push that as one operator
- if the next token is a -, it's a decrement, push that as one operator
- if the previous token is an identifier or a literal, it's subtraction
// - if the previous token is an operator or an opening parenthesis, it's unary minus
^ Handled during AST parsing, not tokenizing

* Ambiguous case 5: `*`
`*` use cases:
- multiplication (or whatever that's overloaded to for the type)
- exponentiation

When a * is seen, look at the next char. If it's also a *, it's exponentiation, push that as one operator.
Otherwise, it's multiplication, push that as one operator.

* Ambiguous case 6: `/`
`/` use cases
- division operator
- multiline/block comments
    ? Nested comments? (have a counter start at 0 -- set to 1 once the first block comment opener is seen. increment for every opener and decrement for every closer -- if it reaches 0, end of the block, go to normal parsing).
    ? Is this slower than not having nested comments?
- floor division operator //

pretty easy to resolve -- if the current char is an initial /, look at the next char
- if it's another /, it's the floor division operator -- push it as one thing
- if it's a *, it's a multiline comment -– keep going until * / is found (no space)
- otherwise, it's a division operator -- push that

* Ambiguous case 7: `=`
`=` use cases:
- assignment
- equality (or inequality)
(note, cases like <= and >= are handled in the angle brackets parsing since in both cases, the angle bracket appears first so it's the first thing the lexer sees)

again, just look at next char
- if it's an equals, push that as one comparison operator
- otherwise, push it as an assignment operator

* Ambiguous case 8: any arithmetic/logical/bitwise operator
if followed by an equals sign, it's an assignment operator

if current char is an operator (after doing the above checks), look at the next char
- if it's an equals sign, it's an assignment operator, push that as one operator
- otherwise, push it as a regular operator

*/

/*
TODO: Make this lazy
- Large files could have a lot of tokens -- this is not memory efficient
- Instead, generate tokens on demand.

- Create a TokenStream class, with a queue containing tokens
- When the parser asks for the next token, pop it from the queue
- If the queue is empty, generate more tokens
- Like a filereader, this should be abstracted away -- from the point of view of the lexer, it's just a stream of tokens

- Have to still be able to implement lookahead and look behind so the parser can use context
    - Especially important for angle brackets (eventually, once generics start to be implemented)
    - Maybe: inside parser, have an std::vector of tokens for the current parsing context
    - Once the context is done, flush the vector
    - That way, the number of tokens that actually need to be stored in memory is smaller

*/

MANG_BEGIN
namespace lexer {

// Type aliases for convenience
using TokenList = std::vector<Token>;
using str = std::string;
using std::optional;
using std::unique_ptr;
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

/**
 * @brief Holds the state of the lexer
 */
struct LexerState {
    unique_ptr<io::Reader> reader;
    TokenList& tokens;
    /**
     * @brief Initializes the lexer state
     * @param _reader The reader to use for reading the source code
     * @param _tokens The list of tokens to fill with the results of the tokenization
     */
    LexerState(unique_ptr<io::Reader>&& _reader, TokenList& _tokens) : reader(std::move(_reader)), tokens(_tokens) {}

    // Wrapper functions for the reader (to do state.X instead of state.reader->X)
    inline char peekChar(size_t offset = 0) { return reader->peekChar(offset); }
    [[nodiscard]] inline char consumeChar() { return reader->consumeChar(); }
    inline size_t getLine() { return reader->getLine(); }
    inline size_t getCol() { return reader->getColumn(); }
    inline size_t getPosition() { return reader->getPosition(); }
    inline void advance(size_t n = 1) {
        reader->setPosition(reader->getPosition() + n);
    }
    inline bool done() { return reader->done(); }
};

//~ Character and String Literal Lexing

/**
 * @brief Converts hex and unicode escape sequences to their corresponding characters
 * @param esc The escape sequence to convert (e.g., \xFF or \u1234)
 * @param isUnicode true if the escape sequence is unicode, false if it's hex
 * @param skipLength An output parameter that will be set to the length of the escape sequence (e.g., 4 for unicode, 2 for hex)
 * @return The converted character, or NONE if the escape sequence is invalid
 */
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

/**
 * @brief Converts a character to its corresponding escape sequence
 * @param escapeChar The character to convert (e.g., n, t, \, etc.) -- the thing after the \
 * @return The converted character, or NONE if the escape sequence is invalid
 */
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

/**
 * @brief Converts a wide character to its corresponding UTF-8 representation
 * @param wideChar The wide character to convert (e.g., \u1234)
 * @return The UTF-8 encoded string representing the wide character
 */
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

/**
 * @brief Resolves escape characters in a string literal (e.g., \\n, \t, \u1234, etc.)
 * @param escapeString The string to process (e.g., "Hello\\nWorld")
 * @return The processed string with escape sequences resolved
 */
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

/**
 * @brief Processes a character escape sequence (e.g., '\n', '\u1234', etc.)
 * @param charLiteral The character literal to process (e.g., '\n')
 * @param state The lexer state
 */
inline void processCharEscapeSequence(str& charLiteral, LexerState& state) {
    // inline since this is only used in one place
    // extracted for readability only
    auto processed = resolveEscapeCharacters(charLiteral);
    if (processed == "INVALID ESCAPE SEQUENCE") {
        state.tokens.emplace_back(TokenType::Invalid, "INVALID ESCAPE SEQUENCE", state.getLine(), state.getCol());
        return;
    }
    // For escaped characters, we need to check if it represents a single code point
    // not necessarily the same as the length of the resolved string being 1
    size_t byteCount = processed.length();
    bool isValidSingleCodePoint = true;
    if (byteCount > 1) {
        unsigned char firstByte = static_cast<unsigned char>(processed[0]);
        isValidSingleCodePoint = (byteCount == 2 && (firstByte & 0xE0) == 0xC0) ||  // 2-byte UTF-8 character
                                 (byteCount == 3 && (firstByte & 0xF0) == 0xE0) ||  // 3-byte UTF-8 character
                                 (byteCount == 4 && (firstByte & 0xF8) == 0xF0);    // 4-byte UTF-8 character
    }
    if (!isValidSingleCodePoint) {
        fprintf(stderr, "Error: Invalid character literal at line %zu column %zu\n", state.getLine(), state.getCol());
        state.tokens.emplace_back(TokenType::Invalid, "INVALID CharLiteral LITERAL", state.getLine(), state.getCol());
        return;
    }
    state.tokens.emplace_back(TokenType::CharLiteral, processed, state.getLine(), state.getCol());
}

/**
 * @brief Processes a character literal (e.g., 'a', '\n', '\u1234', etc.)
 * @param state The lexer state
 */
void tokenizeCharLiteral(LexerState& state) {
    state.advance();  // Move past the opening quote
    str charLiteral;
    // For simplicity, just extract a chunk of text, handle it later
    // Look for a closing quote
    while (!state.done() && state.peekChar() != '\'') {
        if (state.peekChar() == '\\') {
            // skip past a \ so that in '\'' the ' preceded by a \ doesn't get misinterpreted as a closing quote
            charLiteral += state.consumeChar();
        }
        charLiteral += state.consumeChar();
    }
    if (state.done()) {
        fprintf(stderr, "Unclosed character literal at line %zu column %zu\n", state.getLine(), state.getCol());
        state.tokens.emplace_back(TokenType::Invalid, "INVALID", state.getLine(), state.getCol());
        return;
    }
    // go past closing quote so it doesn't get interpreted as an opening quote in the main tokenizing function
    state.advance();
    if (charLiteral[0] == '\\') {
        return processCharEscapeSequence(charLiteral, state);
    } else if (charLiteral.length() > 1) {
        fprintf(stderr, "Error: Character literal at line %zu column %zu exceeds 1 character limit.\n", state.getLine(), state.getCol());
        state.tokens.emplace_back(TokenType::Invalid, "INVALID CharLiteral LITERAL", state.getLine(), state.getCol());
        return;
    }
    state.tokens.emplace_back(TokenType::CharLiteral, charLiteral, state.getLine(), state.getCol());
}

/**
 * @brief Processes a string literal
 * @param state The lexer state
 */
void tokenizeStringLiteral(LexerState& state) {
    state.advance();  // Move past the opening quote
    bool containsEscapeSequence = false;
    str stringLiteral;

    // for simplicity, just extract a chunk of text until the closing quote -- check it afterwards
    while (!state.done() && state.peekChar() != '"') {
        if (state.peekChar() == '\\') {        // Escape sequence -- skip past the next character (e.g., don't consider a \" as a closing quote)
            stringLiteral += state.consumeChar();  // Add the backslash to the string
            containsEscapeSequence = true;
        }
        stringLiteral += state.consumeChar();  // Add the character to the string
    }
    if (state.done()) {
        // No closing quote found
        fprintf(stderr, "Error: Unterminated string literal at line %zu column %zu\n", state.getLine(), state.getCol());
        state.tokens.emplace_back(TokenType::Invalid, "INVALID", state.getLine(), state.getCol());
        return;
    }
    // Move past the closing quote so it doesn't get interpreted as an opening quote in the main tokenizing function
    state.advance();
    if (containsEscapeSequence) {
        stringLiteral = resolveEscapeCharacters(stringLiteral);
        if (stringLiteral == "INVALID ESCAPE SEQUENCE") {
            state.tokens.emplace_back(TokenType::Invalid, "INVALID", state.getLine(), state.getCol());
            return;
        }
    }
    state.tokens.emplace_back(TokenType::StrLiteral, stringLiteral, state.getLine(), state.getCol());
}

/**
 * @brief Processes a keyword or identifier (e.g., if, while, myVar, etc.)
 * @param state The lexer state
 */
void tokenizeKeywordOrIdentifier(LexerState& state) {
    str lexeme = "";
    while (!state.done() && (isalnum(state.peekChar()) || state.peekChar() == '_')) {
        lexeme += state.consumeChar();
    }
    auto it = keyword_map.find(lexeme);

    state.tokens.emplace_back(
        it != keyword_map.end() ? TokenType::Keyword : TokenType::Identifier,
        lexeme,
        state.getLine(), state.getCol());
}

/**
 * @brief Processes a number literal (e.g., 123, 45.67, etc.)
 * @param state The lexer state
 */
void tokenizeNumber(LexerState& state) {
    str numberLiteral;
    char currentChar = state.peekChar();
    bool isFloat = false;
    std::function<bool(char)> isValidBaseChar;
    // TODO: Add floating point support for hex numbers (but not octal or binary)
    if (currentChar == '0') {
        // Could be a base indicator (0x, 0b, 0o) -- check next char
        char baseChar = state.peekChar(1);
        switch (baseChar) {
            case 'x':
            case 'X':
                // Hexadecimal number
                isValidBaseChar = [](char c) { return isxdigit(static_cast<unsigned char>(c)); };
                state.advance(2);       // Skip the 0x
                numberLiteral += "0x";  // Add the base indicator so the parser can handle it
                break;
            case 'b':
            case 'B':
                isValidBaseChar = [](char c) { return c == '0' || c == '1'; };
                state.advance(2);       // Skip the 0b
                numberLiteral += "0b";  // Add the base indicator so the parser can handle it
                break;
            case 'o':
            case 'O':
                isValidBaseChar = [](char c) { return c >= '0' && c <= '7'; };
                state.advance(2);       // Skip the 0o
                numberLiteral += "0o";  // Add the base indicator so the parser can handle it
                break;
            default:
                // Not a valid base indicator -- just treat it as a decimal number
                isValidBaseChar = [](char c) { return isdigit(static_cast<unsigned char>(c)); };
                break;
        }
        currentChar = state.peekChar();  // if there was a base indicator, update the current char
    } else {
        // Decimal number
        isValidBaseChar = [](char c) { return isdigit(c); };
    }

    while (!state.done() && (isValidBaseChar(currentChar) || currentChar == '.')) {
        numberLiteral += state.consumeChar();
        if (currentChar == '.') {
            if (isFloat) {
                // Invalid number -- two decimal points
                fprintf(stderr, "Error: Invalid number at line %zu column %zu\n", state.getLine(), state.getCol());
                return;
            }
            isFloat = true;
        }
        currentChar = state.peekChar();
    }
    // TODO: Add support for scientific notation (e.g., 1.23e4)
    state.tokens.emplace_back(
        isFloat ? TokenType::Float : TokenType::Integer,
        numberLiteral,
        state.getLine(), state.getCol());
}

void skipMultilineComment(LexerState& state) {
    // Skip the comment until the closing comment is found
    state.advance(2);  // Skip the /*
    while (!state.done() && !(state.peekChar() == '*' && state.peekChar(1) == '/')) {
        state.advance();  // Skip the comment
    }
    if (state.done()) {
        fprintf(stderr, "Error: Unclosed comment at line %zu column %zu\n", state.getLine(), state.getCol());
        return;
    }
    state.advance(2);  // Skip the */
    return;            // Successfully skipped the comment
}

void tokenizeSymbol(LexerState& state) {
    TokenType type;
    char current = state.peekChar();
    char next = state.peekChar(1);
    char nextnext = state.peekChar(2);
    str lexeme = std::string(1, current);
    switch (current) {
        //~ Brackets
        case '(':
            type = TokenType::LeftParen;
            break;
        case '{':
            type = TokenType::LeftBrace;
            break;
        case '[':
            type = TokenType::LeftSquare;
            break;
        case '<':
            // Don't handle all the possible cases here -- let parser handle it
            type = TokenType::LeftAngle;
            break;
        case ')':
            type = TokenType::RightParen;
            break;
        case '}':
            type = TokenType::RightBrace;
            break;
        case ']':
            type = TokenType::RightSquare;
            break;
        case '>':
            // Don't handle all the possible cases here -- let parser handle it
            type = TokenType::RightAngle;
            break;

        // ~ Boolean / Bitwise operators
        case '&':
        case '|':
            type = TokenType::Operator;
            if (next == current) {
                // Logical operator (&& or ||)
                lexeme += current;
            } else if (next == '=') {
                // Bitwise assignment operator (&= or |=)
                lexeme += '=';
            } else {
                // Bitwise operator (& or |)
                // Do nothing
            }
            break;
        case '^':
        case '!':
        case '~':
        case '=':
            type = TokenType::Operator;
            if (next == '=') {
                // Assignment operator (^=, != or ~=)
                // or equality check (==)
                lexeme += '=';
            }
            break;

        // ~ Other punctuation
        case ';':
            type = TokenType::Semicolon;
            break;
        case ',':
            type = TokenType::Comma;
            break;
        case '.':
            lexeme = (next == '.' && nextnext == '.') ? "..." : ".";
            [[fallthrough]];  // Intentionally fall through to set the type
        case '?':
        case '@':
            type = TokenType::Operator;
            break;
        case ':':
            type = (next == ':') ? TokenType::Operator : TokenType::Colon;
            lexeme = (next == ':') ? "::" : ":";
            break;

        //~ Arithmetic operators
        case '+':
            type = TokenType::Operator;
            if (next == '+' || next == '=') {
                //* ++ (increment) or += (in-place addition)
                lexeme += next;
            }
            break;
        case '-':
            type = TokenType::Operator;
            if (next == '-' || next == '=' || next == '>') {
                //* -- (decrement) or -= (in-place subtraction) or -> (arrow operator)
                lexeme += next;
            }
            break;
        case '%':
            type = TokenType::Operator;
            if (next == '=') {
                // %= (in-place modulus)
                lexeme += '=';
            }
            break;
        case '*':
            type = TokenType::Operator;
            if (next == '=') {
                //* *= (in-place multiplication)
                lexeme += '=';
            } else if (next == '*') {
                // ** (exponentiation) or **= (in-place exponentiation)
                lexeme += next;
                lexeme += (nextnext == '=') ? "=" : "";
            }
            break;
        case '/':
            type = TokenType::Operator;
            if (next == '=') {
                //* /= (in-place division)
                lexeme += '=';
            } else if (next == '/') {
                //* // (floor division) or //= (in-place floor division)
                lexeme += next;
                lexeme += (nextnext == '=') ? "=" : "";
            } else if (next == '*') {
                skipMultilineComment(state);
                return;  // Don't add a token for the comment
            }
            break;
        default:
            type = TokenType::Invalid;
            break;
    }
    state.advance(lexeme.length());
    state.tokens.emplace_back(type, lexeme, state.getLine(), state.getCol());
}

TokenList tokenize(const str& source, Mode mode) {
    unique_ptr<io::Reader> reader;
    switch (mode) {
        case Mode::String:
            reader = std::make_unique<io::StringReader>(source);
            break;
        case Mode::File:
            reader = std::make_unique<io::FileReader>(source);
            break;
            // Don't need a default case since any other mode is invalid
    }

    TokenList tokens;
    LexerState state(std::move(reader), tokens);
    char currentChar = state.peekChar();
    while (!state.done()) {
        if (currentChar == '#') {
            // Inline comment
            do {
                state.advance();
                currentChar = state.peekChar();
            } while (!state.done() && currentChar != '\n');
            state.advance();  // Skip the newline (if EOF, this will just stay at EOF)
        } else if (std::isspace(currentChar)) {
            state.advance();
        } else if (isalpha(currentChar) || currentChar == '_') {
            tokenizeKeywordOrIdentifier(state);
        } else if (currentChar == '\'') {
            tokenizeCharLiteral(state);
        } else if (currentChar == '\"') {
            tokenizeStringLiteral(state);
        } else if (isdigit(currentChar)) {
            tokenizeNumber(state);
        } else {
            tokenizeSymbol(state);
        }
        currentChar = state.peekChar();
    }
    tokens.emplace_back(TokenType::EndOfFile, "EOF", state.getLine(), state.getCol());
    return tokens;
}
}  // namespace lexer
MANG_END