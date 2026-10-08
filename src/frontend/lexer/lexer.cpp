#include <core.hpp>
#include <cstddef>
#include <cstdint>
#include <format>
#include <frontend/lexer.hpp>
#include <io/filereader.hpp>
#include <io/logging.hpp>
#include <io/stringreader.hpp>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <utils/result.hpp>
#include <utils/str_to_num.hpp>

namespace Manganese::lexer {

//~ Core Lexer Functions

Lexer::Lexer(const std::string& source, utils::StringInterner& interner, Mode mode)
    : tokenStartLine(1), tokenStartCol(1), interner(interner) {
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
        } else if (is_whitespace(currentChar)) {
            advance();  // Skip whitespace
        } else if (currentChar == 'r' && peekChar(1) == '`') {
            result = tokenizeRawStringLiteral();
        } else if (is_alpha(currentChar) || currentChar == '_') {
            result = tokenizeKeywordOrIdentifier();
            ++numTokensMade;
        } else if (currentChar == '\'') {
            result = tokenizeCharLiteral();
            ++numTokensMade;
        } else if (currentChar == '"') {
            result = tokenizeStringLiteral();
            ++numTokensMade;
        } else if (is_digit(currentChar)) {
            result = tokenizeNumber();
            ++numTokensMade;
        } else {
            result = tokenizeSymbol();
            ++numTokensMade;
        }
        currentChar = peekChar();
        tokenStartLine = getLine();
        tokenStartCol = getCol();
        if (result == Result::Failure) { flags.hasError = true; }
    }
    if (done()) {
        // Just finished tokenizing
        emitToken(TokenType::EndOfFile, "EOF", false);
    }
}

Token& Lexer::peekToken() {
    if (tokenStream.empty()) {
        if (done()) {
            emitToken(TokenType::EndOfFile, "EOF", false);
        } else {
            lex(QUEUE_LOOKAHEAD_AMOUNT);
        }
    }

    return tokenStream[0];
}

Token Lexer::consumeToken() {
    if (tokenStream.empty()) { lex(QUEUE_LOOKAHEAD_AMOUNT); }
    // check if queue is still empty if it is, we are done tokenizing
    if (tokenStream.empty()) { return {TokenType::EndOfFile, interner.intern(std::string_view("EOF")), getLine(), getCol()}; }
    const Token token = tokenStream.front();
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
    while (!done() && (is_alphanumeric(peekChar()) || peekChar() == '_')) { lexeme += consumeChar(); }
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

            if (base != utils::Base::Decimal) {
                logError("Invalid number literal : floating point values are only allowed for decimal literals");
                advance();
                result = Result::Failure;
                continue;
            }

            isFloat = true;
            numberLiteral += consumeChar();
            continue;
        }

        if (!is_alphanumeric(currentChar)) { break; }

        if (!isValidBaseChar(currentChar)) {
            const char lowerChar = to_lowercase(currentChar);

            if (lowerChar == 'i' || lowerChar == 'f' || lowerChar == 'u' || lowerChar == 'e') { break; }
            logError("Invalid digit '{}' in numeric constant", currentChar);
            advance();
            result = Result::Failure;
            continue;
        }
        numberLiteral += consumeChar();
    }
    if (base == utils::Base::Decimal && to_lowercase(peekChar()) == 'e') {
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

Result Lexer::tokenizeRawStringLiteral() {
    DISCARD(consumeChar());  // skip the 'r'
    std::size_t backtickCount = 0;
    while (peekChar() == '`') {
        ++backtickCount;
        DISCARD(consumeChar());
    }
    //* we've passed the opening backticks now; start reading text
    std::string rawStringLiteral;
    while (!done()) {
        if (peekChar() != '`') {
            // regular character
            rawStringLiteral += consumeChar();
            continue;
        }
        // seen a '`', this could be the end of the string
        std::size_t closingBacktickCount = 0;

        while (peekChar() == '`') {
            ++closingBacktickCount;
            DISCARD(consumeChar());
            if (closingBacktickCount == backtickCount) {
                // end of string literal
                emitToken(TokenType::StrLiteral, std::move(rawStringLiteral), false);
                return Result::Success;
            }
        }
        // we didn't see the same number of closing backticks, so these are just backticks inside the string
        rawStringLiteral.append(closingBacktickCount, '`');
    }
    logError("Unterminated raw string literal {} (expected {} backticks to close the string)", rawStringLiteral,
             backtickCount);
    emitToken(TokenType::StrLiteral, std::move(rawStringLiteral), true);
    return Result::Failure;
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
                lexeme += '=';
                type = TokenType::BitXorAssign;
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
        case '?': {
            type = TokenType::Ternary;
            break;
        }
        case '~': {
            type = TokenType::BitNot;
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
                lexeme += '=';
                type = TokenType::LessThanOrEqual;
            } else if (next == current) {
                lexeme += next;
                lexeme += (nextnext == '=') ? "=" : "";
                type = (nextnext == '=') ? TokenType::BitLShiftAssign : TokenType::BitLShift;
            } else {
                type = TokenType::LessThan;
            }
            break;
        }
        case '>': {
            if (next == '=') {
                lexeme += '=';
                type = TokenType::GreaterThanOrEqual;
            } else if (next == current) {
                lexeme += next;
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
    tokenStream.emplace_back(type, interner.intern(lexeme), tokenStartLine, tokenStartCol, invalid);
}

NumberPrefixResult Lexer::processNumberPrefix() {
    const char currentChar = peekChar();
    if (currentChar != '0') {
        // Decimal number
        return NumberPrefixResult{.base = utils::Base::Decimal, .isValidBaseChar = is_digit, .prefix = ""};
    }
    // Could be a base indicator (0x, 0b, 0o) -- check next char
    switch (peekChar(1)) {
        case 'x':
        case 'X':
            advance(2);
            return NumberPrefixResult{.base = utils::Base::Hexadecimal, .isValidBaseChar = is_xdigit, .prefix = "0x"};
        case 'b':
        case 'B':
            advance(2);
            return NumberPrefixResult{.base = utils::Base::Binary, .isValidBaseChar = is_bdigit, .prefix = "0b"};
        case 'o':
        case 'O':
            advance(2);
            return NumberPrefixResult{.base = utils::Base::Octal, .isValidBaseChar = is_odigit, .prefix = "0o"};
        case 'd':
        case 'D':
            advance(2);
            return NumberPrefixResult{.base = utils::Base::Octal, .isValidBaseChar = is_digit, .prefix = "0d"};
        default:
            if (is_digit(peekChar(1))) {
                logWarning("Leading zeros in numeric literals are treated as decimal numbers."
                           "Use a 0o prefix for octal numbers.");
            }
            return NumberPrefixResult{.base = utils::Base::Decimal, .isValidBaseChar = is_digit, .prefix = ""};
    }
}

Result Lexer::processScientificNotation(std::string& numberLiteral) {
    numberLiteral += to_lowercase(consumeChar());
    if (peekChar() == '+' || peekChar() == '-') { numberLiteral += consumeChar(); }

    if (!is_digit(peekChar())) {
        logError("Invalid exponent: must be a number");
        return Result::Failure;
    }

    while (!done() && is_digit(peekChar())) { numberLiteral += consumeChar(); }

    return Result::Success;
}

Result Lexer::processNumberSuffix(std::string& numberLiteral, bool isFloat) {
    const char suffix = to_lowercase(peekChar());

    if (suffix != 'i' && suffix != 'u' && suffix != 'f') { return Result::Success; }

    DISCARD(consumeChar());

    std::string width;

    while (!done() && is_digit(peekChar())) { width += consumeChar(); }

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