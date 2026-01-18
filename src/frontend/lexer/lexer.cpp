/**
 * @file lexer.cpp
 * @brief This file contains the implementation of the lexer for the Manganese compiler.
 *
 * The lexer takes the source code and splits it into tokens (see token.h)
 * These tokens are then passed on to the parser to build the AST.
 *
 *
 * @note: The lexer strips out comments and whitespace; the parser never sees this
 *
 * @note: The main loop does not advance the reader position, it just peeks the current character.
 * Each specific tokenization function should advance the reader position once its token has been generated.
 * E.g. the string tokenizing function will advance the reader past the quotes
 */

/*
~ Some ambiguous cases to consider  -- cases where a character could map to more than one operator
~ If any of these occur while inside a string literal or in a char, obviously don't do anything
* Ambiguous case 1: angle brackets (`<` and `>`)
`<` and `>` use cases:
- comparisons (<, >)
- comparisons with equality (<=, >=)
- bitwise shifts (<<, >>)

when < or > is seen, check next char:
if it's an =, push that as one operator
otherwise, push it as a comparison op


* Ambiguous case 2: Logical/bitwise and and or (&&/&, ||/|)
If the current char is a bitwise operator, look at the next char
- if it's a the same character, push that as a logical operator
- if it's an equals sign, push it as a bitwise assignment operator
- otherwise push it as a regular bitwise operator

* Ambiguous case 3: `+`
`+` use cases:
- unary plus
- addition (or whatever that's overloaded to for the type)
- increment (++)

- if the next token is a `+`, it's an increment, push that as one operator

* Ambiguous case 4: `-`
`-` use cases:
- unary minus
- subtraction (or whatever that's overloaded to for the type)
- decrement
- arrow (->)

- if the next token is a >, it's ->, push that as one operator
- if the next token is a -, it's a decrement, push that as one operator

When a * is seen, look at the next char. If it's also a *, it's exponentiation, push that as one operator.
Otherwise, it's multiplication, push that as one operator.

* Ambiguous case 5: `/`
`/` use cases
- division operator
- multiline/block comments
- floor division operator //

if the current char is an initial /, look at the next char
- if it's another /, it's the floor division operator -- push it as one thing
- if it's a *, it's a multiline comment -– keep going until * / is found (no space)
- otherwise, it's a division operator -- push that

* Ambiguous case 6: `=`
`=` use cases:
- assignment
- equality (or inequality)
(note, cases like <= and >= are handled in the angle brackets parsing since in both cases, the angle bracket appears
first so it's the first thing the lexer sees)

look at next char
- if it's an equals, push that as one comparison operator
- otherwise, push it as an assignment operator

* Ambiguous case 7: any arithmetic/bitwise operator
if followed by an equals sign, it's an assignment operator

if current char is an operator (after doing the above checks), look at the next char
- if it's an equals sign, it's an assignment operator, push that as one operator
- otherwise, push it as a regular operator
*/

#include <cctype>
#include <format>
#include <frontend/lexer.hpp>
#include <functional>
#include <global_macros.hpp>
#include <io/filereader.hpp>
#include <io/logging.hpp>
#include <io/reader.hpp>
#include <io/stringreader.hpp>
#include <memory>
#include <string>
#include <utility>

#include "frontend/lexer/token_base.hpp"
#include "frontend/lexer/token_type.hpp"

namespace Manganese {

namespace lexer {

//~ Core Lexer Functions

Lexer::Lexer(const std::string& source, const Mode mode) : tokenStartLine(1), tokenStartCol(1) {
    switch (mode) {
        case Mode::String: reader = std::make_unique<io::StringReader>(source); break;
        case Mode::File: reader = std::make_unique<io::FileReader>(source); break;
    }
    if (reader->hasCriticalError()) {
        this->hasCriticalError_ = true;
        return;
    }
}

void Lexer::lex(size_t numTokens) {
    if (done()) { return; }
    size_t numTokensMade = 0;
    char currentChar = peekChar();
    while (!done() && numTokensMade < numTokens) {
        // ? TODO: Output comments as tokens (or to a separate stream?)
        // ? Multiline probably, single line probably not
        // ? TODO: Update parser to skip past comment tokens
        if (currentChar == '#') {
            // Single line comment
            do {
                advance();
                currentChar = peekChar();
            } while (!done() && currentChar != '\n');
            advance();  // Skip the newline
        } else if (currentChar == '/' && peekChar(1) == '*') {
            skipBlockComment();
        } else if (std::isspace(currentChar)) [[likely]] {  // lots of whitespace
            advance();  // Skip whitespace
        } else if (isalpha(currentChar) || currentChar == '_') [[likely]] {  // Mostly identifiers and keywords
            tokenizeKeywordOrIdentifier();
            ++numTokensMade;
        } else if (currentChar == '\'') {
            tokenizeCharLiteral();
            ++numTokensMade;
        } else if (currentChar == '"') {
            // TODO: Add raw string literals (r"stuff" -- maybe r""text"")
            //? TODO: f-strings? (f"stuff {expression} more stuff")
            tokenizeStringLiteral();
            ++numTokensMade;
        } else if (std::isdigit(currentChar)) {
            tokenizeNumber();
            ++numTokensMade;
        } else {
            tokenizeSymbol();
            ++numTokensMade;
        }
        currentChar = peekChar();
        tokenStartLine = getLine();
        tokenStartCol = getCol();
    }
    if (done()) {
        // Just finished tokenizing
        tokenStream.emplace_back(TokenType::EndOfFile, "EOF", getLine(), getCol());
    }
}

Token Lexer::peekToken() noexcept {
    if (done() && tokenStream.empty()) { return Token(TokenType::EndOfFile, "EOF", getLine(), getCol()); }
    if (tokenStream.empty()) { lex(QUEUE_LOOKAHEAD_AMOUNT); }
    return tokenStream[0];
}

Token Lexer::consumeToken() noexcept {
    if (tokenStream.empty()) { lex(QUEUE_LOOKAHEAD_AMOUNT); }
    if (tokenStream.empty()) {
        // still empty -- we are done tokenizing
        return Token(TokenType::EndOfFile, "EOF", getLine(), getCol());
    }
    Token token = tokenStream.front();
    tokenStream.pop_front();  // get rid of the token
    return token;
}

//~ Main State Machine Functions

void Lexer::tokenizeCharLiteral() {
    advance();  // Move past the opening quote
    std::string charLiteral;
    // For simplicity, just extract a chunk of text, handle it later
    // Look for a closing quote
    while (true) {
        if (done()) {
            logging::logError(getLine(), getCol(), "Unclosed character literal");
            tokenStream.emplace_back(TokenType::CharLiteral, charLiteral, tokenStartLine, tokenStartCol, true);
            return;
        }
        if (peekChar() == '\'') { break; }
        if (peekChar() == '\n') {
            logging::logError(getLine(), getCol(), "Unclosed character literal");

            tokenStream.emplace_back(TokenType::CharLiteral, charLiteral, tokenStartLine, tokenStartCol, true);
            return;
        }
        if (peekChar() == '\\') {
            // Skip past a \ so that in '\'' the ' preceded by a \ doesn't get misinterpreted as a closing quote
            charLiteral += consumeChar();  // Add the backslash to the string
        }
        charLiteral += consumeChar();  // Add the character to the string
    }
    advance();
    if (charLiteral[0] == '\\') {
        processCharEscapeSequence(charLiteral);
        return;
    } else if (charLiteral.length() > 1) {
        logging::logError(getLine(), getCol(), "Character literal exceeds 1 character limit");
        tokenStream.emplace_back(TokenType::CharLiteral, charLiteral, tokenStartLine, tokenStartCol, true);
        return;
    }
    tokenStream.emplace_back(TokenType::CharLiteral, charLiteral, tokenStartLine, tokenStartCol);
}

void Lexer::tokenizeStringLiteral() {
    advance();  // Move past the opening quote
    bool containsEscapeSequence = false;
    std::string stringLiteral;

    // for simplicity, just extract a chunk of text until the closing quote -- check it afterwards
    while (true) {
        if (done()) {
            logging::logError(getLine(), getCol(), "Unclosed string literal");
            tokenStream.emplace_back(TokenType::StrLiteral, stringLiteral, tokenStartLine, tokenStartCol, true);
            return;
        }
        if (peekChar() == '"') { break; }
        if (peekChar() == '\\') {
            if (peekChar(1) == '\n') {
                // continuing a string literal across lines
                advance(2);  // Skip the backslash and the newline
                continue;
            }
            // Escape sequence -- skip past the next character (e.g., don't consider a \" as a closing quote)
            stringLiteral += consumeChar();  // Add the backslash to the string
            containsEscapeSequence = true;
        } else if (peekChar() == '\n') {
            logging::logError(
                getLine(), getCol(),
                "String literal cannot span multiple lines. If you wanted a string literal that spans lines, add a backslash ('\\') at the end of the line");

            tokenStream.emplace_back(TokenType::StrLiteral, stringLiteral, tokenStartLine, tokenStartCol, true);
            return;
        }
        stringLiteral += consumeChar();  // Add the character to the string
    }

    advance();
    if (containsEscapeSequence) {
        std::optional<std::string> result = resolveEscapeCharacters(stringLiteral);
        if (!result) {
            tokenStream.emplace_back(TokenType::StrLiteral, stringLiteral, tokenStartLine, tokenStartCol, true);
            return;
        }
        stringLiteral = std::move(result.value());
    }
    tokenStream.emplace_back(TokenType::StrLiteral, stringLiteral, tokenStartLine, tokenStartCol);
}

void Lexer::tokenizeKeywordOrIdentifier() {
    std::string lexeme = "";
    while (!done() && (isalnum(peekChar()) || peekChar() == '_')) { lexeme += consumeChar(); }
    TokenType t = keyword_lookup(lexeme);

    // if t is unknown, assume it's an identifier, otherwise use the given keyword type
    tokenStream.emplace_back(t == TokenType::Unknown ? TokenType::Identifier : t, lexeme, tokenStartLine,
                             tokenStartCol);
}

void Lexer::tokenizeNumber() {
    std::string numberLiteral;
    bool isFloat = false;
    auto [base, isValidBaseChar, prefix] = processNumberPrefix();
    numberLiteral += prefix;
    char currentChar = peekChar();  // If there was a number prefix, update the current char

    while (!done() && (isValidBaseChar(currentChar) || currentChar == '.' || currentChar == '_')) {
        if (currentChar == '_') {
            // Ignore underscores
            advance();
            currentChar = peekChar();
            continue;
        } else if (currentChar == '.') {
            if (isFloat) {
                // Invalid number -- two decimal points
                logging::logError(getLine(), getCol(), "Invalid number literal: multiple decimal points");
                tokenStream.emplace_back(TokenType::FloatLiteral, numberLiteral, tokenStartLine, tokenStartCol, true);
                return;
            }
            // Reject floating point for octal and binary numbers
            if (base == Base::Octal || base == Base::Binary) {
                logging::logError(getLine(), getCol(),
                                  "Invalid number literal: floating point not allowed for {} numbers",
                                  (base == Base::Octal ? "octal" : "binary"));
                tokenStream.emplace_back(TokenType::FloatLiteral, numberLiteral, tokenStartLine, tokenStartCol, true);
                return;
            }
            isFloat = true;
        }
        numberLiteral += consumeChar();
        currentChar = peekChar();
    }

    // Handle scientific notation (e.g., 1.23e4), size suffixes (e.g., 1.23f), etc.
    processNumberSuffix(base, numberLiteral, isFloat);
    tokenStream.emplace_back(isFloat ? TokenType::FloatLiteral : TokenType::IntegerLiteral, numberLiteral,
                             tokenStartLine, tokenStartCol);
}

// TODO: Rewrite to not require operator table lookup
void Lexer::tokenizeSymbol() {
    TokenType type;
    char current = peekChar();
    char next = peekChar(1);
    char nextnext = peekChar(2);
    std::string lexeme = std::string(1, current);

    // In here, use TokenType::Operator as a generic value (exact enum mapping determined at the end)
    switch (current) {
        //~ Brackets
        case '(': type = TokenType::LeftParen; break;
        case '{': type = TokenType::LeftBrace; break;
        case '[': type = TokenType::LeftSquare; break;
        case ')': type = TokenType::RightParen; break;
        case '}': type = TokenType::RightBrace; break;
        case ']': type = TokenType::RightSquare; break;

        // ~ Boolean / Bitwise operators
        case '&': {
            if (next == '&') {  // logical AND (&&)
                lexeme += next;
                type = TokenType::And;
            } else if (next == '=') {
                lexeme += next;
                type = TokenType::BitAndAssign;
            } else {
                type = TokenType::BitAnd;
            }
            break;
        }
            // Let the parser decide if '&' is a bitwise AND or an address-of operator, default to bitwise AND
        case '|': {
            if (next == '|') {  // logical OR (||)
                lexeme += next;
                type = TokenType::Or;
            } else if (next == '=') {
                lexeme += next;
                type = TokenType::BitOrAssign;
            } else {
                type = TokenType::BitOr;
            }
            break;
        }
        case '^': {  // Bitwise XOR
            if (next == '=') {
                // Bitwise assignment operator (^=)
                lexeme += '=';
                type = TokenType::BitXorAssign;
            } else if (next == '^') {
                // Exponentiation operator (^^)
                lexeme += '^';
                lexeme += (nextnext == '=') ? "=" : "";  // ^^=, in place exponentiation
                type = (nextnext == '=') ? TokenType::ExpAssign : TokenType::Exp;
            } else {
                type = TokenType::BitXor;
            }
            break;
        }
        case '!': {
            if (next == '=') {  // Inequality (!=)
                lexeme += next;
                type = TokenType::NotEqual;
            } else {
                type = TokenType::Not;
            }
            break;
        }
        case '~': {
            if (next == '=') {
                lexeme += next;
                type = TokenType::BitNotAssign;
            } else {
                type = TokenType::BitNot;
            }
            break;
        }
        case '=': {
            if (next == '=') {  // Equality (==)
                lexeme += next;
                type = TokenType::Equal;
            } else {
                type = TokenType::Assignment;
            }
            break;
        }
        case '<': {
            if (next == '=') {
                // Less than or Equal to (<=)
                lexeme += '=';
                type = TokenType::LessThanOrEqual;
            } else if (next == current) {
                // Bitwise left shift (<<)
                lexeme += next;
                // In place left shift (<<=)
                lexeme += (nextnext == '=') ? "=" : "";
                type = (nextnext == '=') ? TokenType::BitLShiftAssign : TokenType::BitLShift;
            } else {
                type = TokenType::LessThan;
            }
            break;
        }
        case '>': {
            if (next == '=') {
                // Greater than or Equal to (>=)
                lexeme += '=';
                type = TokenType::GreaterThanOrEqual;
            } else if (next == current) {
                // Bitwise right shift (>>)
                lexeme += next;
                // In place right shift (>>=)
                lexeme += (nextnext == '=') ? "=" : "";
                type = (nextnext == '=') ? TokenType::BitRShiftAssign : TokenType::BitRShift;
            } else {
                type = TokenType::GreaterThan;
            }
            break;
        }

        // ~ Other punctuation
        case ';': type = TokenType::Semicolon; break;
        case ',': type = TokenType::Comma; break;
        case '.': {
            if (next == '.' && nextnext == '.') {
                lexeme = "...";
                type = TokenType::Ellipsis;
            } else {
                type = TokenType::MemberAccess;
            }
            break;
        }
        case ':': {
            type = (next == ':') ? TokenType::ScopeResolution : TokenType::Colon;
            lexeme = (next == ':') ? "::" : ":";
            break;
        }
        case '@': type = TokenType::At; break;

        //~ Arithmetic operators
        case '+': {
            if (next == '+') {
                lexeme += next;
                type = TokenType::Inc;
            } else if (next == '=') {
                lexeme += next;
                type = TokenType::PlusAssign;
            } else {
                type = TokenType::Plus;
            }
            break;
        }
        case '-': {
            if (next == '-') {
                lexeme += next;
                type = TokenType::Dec;
            } else if (next == '=') {
                lexeme += next;
                type = TokenType::MinusAssign;
            } else if (next == '>') {
                lexeme += next;
                type = TokenType::Arrow;
            } else {
                type = TokenType::Minus;
            }
            break;
        }
        case '%': {
            if (next == '=') {
                lexeme += '=';
                type = TokenType::ModAssign;
            } else {
                type = TokenType::Mod;
            }
            break;
        }
        case '*': {
            if (next == '=') {
                lexeme += '=';
                type = TokenType::MulAssign;
            } else {
                type = TokenType::Mul;
            }
            break;
        }
        case '/': {
            if (next == '=') {
                lexeme += '=';
                type = TokenType::DivAssign;
            } else if (next == '/') {
                lexeme += next;
                lexeme += (nextnext == '=') ? "=" : "";
                type = (nextnext == '=') ? TokenType::FloorDivAssign : TokenType::FloorDiv;
            } else {
                type = TokenType::Div;
            }
            // Multiline comments handled in the main loop
            break;
        }
        default:
            type = TokenType::Unknown;
            logging::logError(getLine(), getCol(), "Invalid character: '{}'", current);
            tokenStream.emplace_back(type, lexeme, tokenStartLine, tokenStartCol, true);
            break;
    }
    advance(lexeme.length());
    tokenStream.emplace_back(type, lexeme, tokenStartLine, tokenStartCol);
}

void Lexer::skipBlockComment() {
    advance(2);  // Skip the /*
    int64_t commentDepth = 1;  // Allow nested comments
    size_t startLine = getLine(), startCol = getCol();
    while (!done() && commentDepth > 0) {
        if (peekChar() == '/' && peekChar(1) == '*') {
            ++commentDepth;
            advance(2);
        } else if (peekChar() == '*' && peekChar(1) == '/') {
            --commentDepth;
            advance(2);
            if (commentDepth == 0) { break; }
        } else {
            advance();
        }
    }
    if (commentDepth > 0) {
        logging::logError(getLine(), getCol(),
                          "Unclosed block comment at end of file (comment started at line {}, column {})", startLine,
                          startCol);
    }
}

//~ Helper Functions

NumberPrefixResult Lexer::processNumberPrefix() {
    char currentChar = peekChar();
    if (currentChar != '0') {
        // Decimal number
        return NumberPrefixResult{.base = Base::Decimal,
                                  .isValidBaseChar = [](char c) { return isdigit(static_cast<unsigned char>(c)); },
                                  .prefix = ""};
    }
    // Could be a base indicator (0x, 0b, 0o) -- check next char
    switch (peekChar(1)) {
        case 'x':
        case 'X':
            // Hexadecimal number
            advance(2);
            return NumberPrefixResult{.base = Base::Hexadecimal,
                                      .isValidBaseChar = [](char c) { return isxdigit(static_cast<unsigned char>(c)); },
                                      .prefix = "0x"};
        case 'b':
        case 'B':
            // Binary number
            advance(2);
            return NumberPrefixResult{
                .base = Base::Binary, .isValidBaseChar = [](char c) { return c == '0' || c == '1'; }, .prefix = "0b"};
        case 'o':
        case 'O':
            // Octal number
            advance(2);
            return NumberPrefixResult{
                .base = Base::Octal, .isValidBaseChar = [](char c) { return c >= '0' && c <= '7'; }, .prefix = "0o"};
        default:
            // Not a valid base indicator -- just treat it as a decimal number
            logging::logWarning(
                getLine(), getCol(),
                "Leading zeros in numeric literals are treated as decimal numbers. Use a 0o prefix for octal numbers.");
            return NumberPrefixResult{
                .base = Base::Decimal, .isValidBaseChar = [](char c) { return c >= '0' && c <= '9'; }, .prefix = ""};
    }
}

bool Lexer::processNumberSuffix(Base base, std::string& numberLiteral, bool isFloat) {
    /*
        The valid numeric suffixes are:
        - i8, i16, i32, i64 (signed integers with the corresponding bit width)
        - u8, u16, u32, u64 (unsigned integers with the corresponding bit width)
        - f32, f64 (floating-point numbers with the corresponding bit width)
        NOTE: These are case-insensitive, so 'I', 'U', and 'F' are also valid.
        */

    auto readUint = [this]() -> int {
        int value = 0;
        while (!done() && std::isdigit(peekChar())) { value = value * 10 + (consumeChar() - '0'); }
        return value;
    };

    // Suffix Handling

    char currentChar = (char)tolower(peekChar());
    if (currentChar == 'i' || currentChar == 'u' || currentChar == 'f') {
        char suffix = static_cast<char>(std::tolower(consumeChar()));
        int width = readUint();
        if (width == 0) {
            logging::logError(getLine(), getCol(), "Invalid Numeric Suffix {}", width);
            return false;
        }
        const bool validIntWidth = (width == 8 || width == 16 || width == 32 || width == 64);
        const bool validFloatWidth = (width == 32 || width == 64);

        if ((suffix == 'i' || suffix == 'u') && !validIntWidth) {
            logging::logError(getLine(), getCol(), "Invalid integer suffix: must be 8, 16, 32, or 64");
            return false;
        }
        if (suffix == 'f' && !validFloatWidth) {
            logging::logError(getLine(), getCol(), "Invalid float suffix: must be 32 or 64");
            return false;
        }
        if (suffix == 'f' && !isFloat) {
            logging::logError(getLine(), getCol(), "Float suffix can only be used with floating-point literals");
            return false;
        }

        if ((suffix == 'i' || suffix == 'u') && isFloat) {
            logging::logError(getLine(), getCol(), "Integer suffix cannot be used with floating-point literals");
            return false;
        }
        numberLiteral += suffix;
        numberLiteral += std::to_string(width);
    }

    // Scientific Notation

    currentChar = (char)tolower(peekChar());  // Update current character after processing the suffix
    auto parseExponent = [&](char expected) -> bool {
        if (currentChar != expected) return true;

        numberLiteral += static_cast<char>(std::tolower(consumeChar()));

        char next = peekChar();
        if (next == '+' || next == '-') { numberLiteral += consumeChar(); }

        if (!std::isdigit(peekChar())) {
            logging::logError(getLine(), getCol(), "Invalid exponent: must be a number");
            return false;
        }

        while (!done() && std::isdigit(peekChar())) { numberLiteral += consumeChar(); }

        return true;
    };

    if (base == Base::Decimal) {
        if (!parseExponent('e')) { return false; }
    } else if (base == Base::Hexadecimal && isFloat) {
        if (!parseExponent('p')) {
            logging::logError(getLine(), getCol(), "Invalid hexadecimal float: must have 'p' exponent");
            return false;
        }
    }
    return true;
}

}  // namespace lexer
}  // namespace Manganese
