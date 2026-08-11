#include <cstddef>
#include <format>
#include <frontend/lexer.hpp>
#include <io/filereader.hpp>
#include <io/logging.hpp>
#include <io/stringreader.hpp>
#include <memory>
#include <mnstl/number.hxx>
#include <optional>
#include <string>
#include <utility>
#include <utils/result.hpp>

#include "frontend/lexer/lexer_base.hpp"


namespace Manganese::lexer {

//~ Core Lexer Functions

Lexer::Lexer(const std::string& source, Mode mode) : tokenStartLine(1), tokenStartCol(1) {
    switch (mode) {
        case Mode::String: reader = std::make_unique<io::StringReader>(source); break;
        case Mode::File: reader = std::make_unique<io::FileReader>(source); break;
    }
}

void Lexer::lex(std::size_t numTokens) {
    if (done()) { return; }
    std::size_t numTokensMade = 0;
    char currentChar = peekChar();
    while (!done() && numTokensMade < numTokens) {
        Result result = Result::Success;
        if (currentChar == '#') {
            // Single line comment
            do {
                advance();
                currentChar = peekChar();
            } while (!done() && currentChar != '\n');
            advance();  // Skip the newline
        } else if (currentChar == '/' && peekChar(1) == '*') {
            result = skipBlockComment();
        } else if (isspace(currentChar)) {
            advance();  // Skip whitespace
        } else if (isalpha(currentChar) || currentChar == '_') {
            result = tokenizeKeywordOrIdentifier();
            ++numTokensMade;
        } else if (currentChar == '\'') {
            result = tokenizeCharLiteral();
            ++numTokensMade;
        } else if (currentChar == '"') {
            // TODO: Add raw string literals (r"stuff" -- maybe r""text"")
            result = tokenizeStringLiteral();
            ++numTokensMade;
        } else if (isdigit(currentChar)) {
            result = tokenizeNumber();
            ++numTokensMade;
        } else {
            result = tokenizeSymbol();
            ++numTokensMade;
        }
        currentChar = peekChar();
        tokenStartLine = getLine();
        tokenStartCol = getCol();
        _hasError = _hasError || (result == Result::Failure);
    }
    if (done()) {
        // Just finished tokenizing
        tokenStream.emplace_back(TokenType::EndOfFile, "EOF", getLine(), getCol());
    }
}

Token Lexer::peekToken() {
    if (done() && tokenStream.empty()) { return Token(TokenType::EndOfFile, "EOF", getLine(), getCol()); }
    if (tokenStream.empty()) { lex(QUEUE_LOOKAHEAD_AMOUNT); }
    return tokenStream[0];
}

Token Lexer::consumeToken() {
    if (tokenStream.empty()) { lex(QUEUE_LOOKAHEAD_AMOUNT); }
    // check if queue is still empty if it is, we are done tokenizing
    if (tokenStream.empty()) { return Token(TokenType::EndOfFile, "EOF", getLine(), getCol()); }
    Token token = tokenStream.front();
    tokenStream.pop_front();  // get rid of the token
    return token;
}

//~ Main State Machine Functions

Result Lexer::tokenizeCharLiteral() {
    advance();  // Move past the opening quote
    std::string charLiteral;
    // For simplicity, just extract a chunk of text, handle it later
    // Look for a closing quote
    while (true) {
        if (done()) {
            logError("Unclosed character literal");
            emitToken(TokenType::CharLiteral, std::move(charLiteral), true);
            return Result::Failure;
        }
        if (peekChar() == '\'') { break; }
        if (peekChar() == '\n') {
            logError("Unclosed character literal");

            emitToken(TokenType::CharLiteral, std::move(charLiteral), true);
            return Result::Failure;
        }
        if (peekChar() == '\\') {
            // Skip past a \ so that in '\'' the ' preceded by a \ doesn't get misinterpreted as a closing quote
            charLiteral += consumeChar();  // Add the backslash to the string
        }
        charLiteral += consumeChar();  // Add the character to the string
    }
    advance();
    Result result = Result::Success;
    if (charLiteral.empty()) {
        logError("Empty character literal");
        result = Result::Failure;
    } else if (charLiteral[0] == '\\') {
        return processCharEscapeSequence(charLiteral);
    } else {
        const std::optional<DecodedUTF8> decoded = decodeUTF8(charLiteral, 0);
        if (!decoded) {
            logError("Invalid UTF-8 character literal");
            result = Result::Failure;
        } else if (decoded->bytecount != charLiteral.size()) {
            logError("Character literal exceeds 1 character limit");
            result = Result::Failure;
        }
    }
    emitToken(TokenType::CharLiteral, std::move(charLiteral), result == Result::Failure);
    return result;
}

Result Lexer::tokenizeKeywordOrIdentifier() {
    std::string lexeme;
    while (!done() && (isalnum(peekChar()) || peekChar() == '_')) { lexeme += consumeChar(); }
    const TokenType t = keywordLookup(lexeme);

    // if type is unknown, assume it's an identifier, otherwise use the given keyword type
    emitToken(t == TokenType::Unknown ? TokenType::Identifier : t, std::move(lexeme), false);
    return Result::Success;
}

Result Lexer::tokenizeNumber() {
    std::string numberLiteral;
    Result result = Result::Success;

    const auto [base, isValidBaseChar, prefix] = processNumberPrefix();
    numberLiteral += prefix;

    bool isFloat = false;

    while (!done()) {
        const char currentChar = peekChar();
        if (currentChar == '_') {
            advance();
            continue;
        }

        if (currentChar == '.') {
            if (isFloat) {
                logError("Invalid number literal: multiple decimal points");
                advance();
                result = Result::Failure;
                continue;
            }

            if (base != mnstl::Base::Decimal) {
                logError("Invalid number literal : floating point values are only allowed for decimal literals");
                advance();
                result = Result::Failure;
                continue;
            }

            isFloat = true;
            numberLiteral += consumeChar();
            continue;
        }

        if (!isalnum(currentChar)) { break; }

        if (!isValidBaseChar(currentChar)) {
            const char lowerChar = tolower(currentChar);

            if (lowerChar == 'i' || lowerChar == 'f' || lowerChar == 'u' || lowerChar == 'e') { break; }
            logError("Invalid digit '{}' in numeric constant", currentChar);
            advance();
            result = Result::Failure;
            continue;
        }
        numberLiteral += consumeChar();
    }
    if (base == mnstl::Base::Decimal && tolower(peekChar()) == 'e') {
        if (processScientificNotation(numberLiteral) == Result::Failure) { result = Result::Failure; }

        isFloat = true;
    }

    if (processNumberSuffix(numberLiteral, isFloat) == Result::Failure) { result = Result::Failure; }

    emitToken(isFloat ? TokenType::FloatLiteral : TokenType::IntegerLiteral, std::move(numberLiteral),
              result != Result::Success);

    return result;
}

Result Lexer::skipBlockComment() {
    advance(2);  // Skip the /*
    std::uint64_t commentDepth = 1;  // Allow nested block comments
    const std::size_t startLine = getLine();
    const std::size_t startCol = getCol();
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
        logError("Unclosed block comment at end of file (comment started at line {}, column {})", startLine, startCol);
        return Result::Failure;
    }
    return Result::Success;
}

Result Lexer::tokenizeStringLiteral() {
    advance();  // Move past the opening quote
    bool containsEscapeSequence = false;
    std::string stringLiteral;

    // for simplicity, just extract a chunk of text until the closing quote -- check it afterwards
    while (true) {
        if (done()) {
            logError("Unclosed string literal");
            emitToken(TokenType::StrLiteral, std::move(stringLiteral), true);
            return Result::Failure;
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

            emitToken(TokenType::StrLiteral, std::move(stringLiteral), true);
            return Result::Failure;
        }
        stringLiteral += consumeChar();  // Add the character to the string
    }

    Result result = Result::Success;
    advance();
    if (containsEscapeSequence) {
        std::optional<std::string> processedString = resolveEscapeCharacters(stringLiteral);
        if (!processedString) {
            result = Result::Failure;
        } else {
            stringLiteral = std::move(*processedString);
        }
    }
    emitToken(TokenType::StrLiteral, std::move(stringLiteral), result == Result::Failure);
    return result;
}

Result Lexer::tokenizeSymbol() {
    Result result = Result::Success;
    TokenType type;
    const char current = peekChar();
    const char next = peekChar(1);
    const char nextnext = peekChar(2);
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
            }
            // else if (next == '^') {
            //     // Exponentiation operator (^^)
            //     lexeme += '^';
            //     lexeme += (nextnext == '=') ? "=" : "";  // ^^=, in place exponentiation
            //     type = (nextnext == '=') ? TokenType::ExpAssign : TokenType::Exp;
            // }
            else {
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
            logError("Invalid character: '{}'", current);
            result = Result::Failure;
            break;
    }
    advance(lexeme.length());
    emitToken(type, std::move(lexeme), result == Result::Failure);
    return result;
}

//~ Helper Functions

void Lexer::emitToken(TokenType type, std::string&& lexeme, bool invalid) {
    tokenStream.emplace_back(type, std::move(lexeme), tokenStartLine, tokenStartCol, invalid);
}

NumberPrefixResult Lexer::processNumberPrefix() {
    char currentChar = peekChar();
    if (currentChar != '0') {
        // Decimal number
        return NumberPrefixResult{.base = mnstl::Base::Decimal, .isValidBaseChar = isdigit, .prefix = ""};
    }
    // Could be a base indicator (0x, 0b, 0o) -- check next char
    switch (peekChar(1)) {
        case 'x':
        case 'X':
            // Hexadecimal number
            advance(2);
            return NumberPrefixResult{.base = mnstl::Base::Hexadecimal, .isValidBaseChar = isxdigit, .prefix = "0x"};
        case 'b':
        case 'B':
            // Binary number
            advance(2);
            return NumberPrefixResult{.base = mnstl::Base::Binary, .isValidBaseChar = isbdigit, .prefix = "0b"};
        case 'o':
        case 'O':
            // Octal number
            advance(2);
            return NumberPrefixResult{.base = mnstl::Base::Octal, .isValidBaseChar = isodigit, .prefix = "0o"};
        default:
            // Not a valid base indicator -- just treat it as a decimal number
            if (isdigit(peekChar(1))) {  // if the literal is 0, that's fine
                logging::logWarning(getLine(), getCol(),
                                    "Leading zeros in numeric literals are treated as decimal numbers."
                                    "Use a 0o prefix for octal numbers.");
            }
            return NumberPrefixResult{.base = mnstl::Base::Decimal, .isValidBaseChar = isdigit, .prefix = ""};
    }
}

Result Lexer::processScientificNotation(std::string& numberLiteral) {
    // Caller guarantees next character is an 'e'
    numberLiteral += tolower(consumeChar());
    if (peekChar() == '+' || peekChar() == '-') { numberLiteral += consumeChar(); }

    if (!isdigit(peekChar())) {
        logError("Invalid exponent: must be a number");
        return Result::Failure;
    }

    while (!done() && isdigit(peekChar())) { numberLiteral += consumeChar(); }

    return Result::Success;
}

Result Lexer::processNumberSuffix(std::string& numberLiteral, bool isFloat) {
    /*
        The valid numeric suffixes are:
        - i8, i16, i32, i64, i128 (signed integers with the corresponding bit width)
        - u8, u16, u32, u64, u128 (unsigned integers with the corresponding bit width)
        - f32, f64 (floating-point numbers with the corresponding bit width)
        NOTE: These are case-insensitive, so 'I', 'U', and 'F' are also valid.
        */

    const char suffix = tolower(peekChar());

    if (suffix != 'i' && suffix != 'u' && suffix != 'f') { return Result::Success; }

    DISCARD(consumeChar());

    std::string width;

    while (!done() && isdigit(peekChar())) { width += consumeChar(); }

    if (width.empty()) { logError("Numeric suffix '{}' must specify a bit width", suffix); }

    const bool isIntegerSuffix = suffix == 'i' || suffix == 'u';
    const bool isFloatSuffix = suffix == 'f';

    Result result = Result::Success;

    if (isIntegerSuffix) {
        if (width != "8" && width != "16" && width != "32" && width != "64" && width != "128") {
            logError("Invalid integer suffix '{}': must be 8, 16, 32, 64 or 128", width);
            result = Result::Failure;
        }
        if (isFloat) {
            logError("Integer suffix '{}' cannot be used with floating-point literals", suffix);
            result = Result::Failure;
        }
    }
    if (isFloatSuffix) {
        if (width != "32" && width != "64") {
            logError("Invalid float suffix '{}' : must be 32 or 64", width);
            result = Result::Failure;
        }
        if (!isFloat) {
            logError("Float suffix '{}' can only be used with floating point literals", suffix);
            result = Result::Failure;
        }
    }
    numberLiteral += suffix;
    numberLiteral += width;
    return result;
}

}  // namespace Manganese::lexer

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
