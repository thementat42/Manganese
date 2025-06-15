/**
 * @file token.h
 * @brief This file contains the definition of token functionality for the Manganese compiler.
 */

#ifndef TOKEN_H
#define TOKEN_H

#include <global_macros.h>

#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

MANGANESE_BEGIN
namespace lexer {

/**
 * @brief Defines all supported token types in Manganese
 *
 * This enum represents all tokens recognized by the compiler,
 * categorized by their functional purpose (keywords, identifiers, etc.).
 * Each token is documented with its corresponding
 */
enum class TokenType : uint8_t;  // Forward declaration -- implementation is at the end of this file for readability

// TODO: Add debug check that these are in the right order

/**
 * @brief Representation of a token
 */
class Token {
   private:
    TokenType type;

    // specific data about the token
    // std::string is for tokens with no enum representation (identifiers, numbers, etc.)
    std::string lexeme;
    size_t line, column;

   public:
    Token() = default;
    Token(const TokenType type, const std::string lexeme, const size_t line, const size_t column);
    ~Token() = default;

    bool isKeyword() const noexcept;
    bool isOperator() const noexcept;
    bool isLiteral() const noexcept;
    bool isBracket() const noexcept;
    bool hasUnaryCounterpart() const noexcept;
    TokenType getUnaryCounterpart() const noexcept;

    TokenType getType() const noexcept;
    std::string getLexeme() const noexcept;
    size_t getLine() const noexcept;
    size_t getColumn() const noexcept;

    /**
     * @note Parser only: be careful
     */
    void overrideType(TokenType _type, std::string _lexeme = "");

    /**
     * @details This function is used for debugging purposes. (if the debug flag is not set, this function will be empty)
     */
    void log() const noexcept;
    static void log(const Token& token) noexcept;
};

//~ Helpers, not tied to the Token class
std::string tokenTypeToString(TokenType type);
extern std::unordered_map<std::string, const TokenType> keywordMap;
extern std::unordered_map<std::string, const TokenType> operatorMap;
std::optional<TokenType> keywordFromString(const std::string& keyword);
std::optional<TokenType> operatorFromString(const std::string& op);

// Implementation of TokenType
enum class TokenType : uint8_t {
    //~ Basic
    Identifier,      // variables, functions
    StrLiteral,      // "text"
    CharLiteral,     // 'a'
    IntegerLiteral,  // Whole Number
    FloatLiteral,    // Floating point number

    /*
    Keyword and Operator have generic values here for simplicity in other parts of the compiler
    The token constructor uses this to assign a specific enum value (see below)

    For example, the lexer can just say "this is a keyword" and trust the token will be correctly assigned to that keyword, without having to check what keyword it
    */
    Keyword,   // any keyword
    Operator,  // any operator

    //~ Brackets
    LeftParen,    // (
    RightParen,   // )
    LeftBrace,    // {
    RightBrace,   // }
    LeftSquare,   // [
    RightSquare,  // ]
    LeftAngle,    // <
    RightAngle,   // >

    //~ Punctuation
    Semicolon,  // ;
    Colon,      // :
    Comma,      // ,

    //~ Misc
    EndOfFile,
    Invalid,

    //~ Keywords
    __KeywordStart,  // Marker for the start of keyword token types -- not to be used as an actual token type
    //* Type Qualifiers
    Let,    // mutable variable
    Const,  // constant variable
    Ptr,    // pointer variables

    //* Access Levels
    Private,
    ReadOnly,  // readable, but can't be modified, outside the module
    Public,    // can be read and modified outside the module

    //* Primitive Types
    Int8,     // 8 bit int
    Int16,    // 16 bit int
    Int32,    // 32 bit int
    Int64,    // 64 bit int
    UInt8,    // unsigned 8 bit int
    UInt16,   // unsigned 16 bit int
    UInt32,   // unsigned 32 bit int
    UInt64,   // unsigned 64 bit int
    Float32,  // 32-bit float (single precision)
    Float64,  // 64-bit float (double precision)
    Char,     // single character
    Bool,     // true or false
    True,     // boolean true
    False,    // boolean false

    //* Conditionals
    If,    // check a condition
    Elif,  // if previous condition was false, check this
    Else,  // if all previous conditions were false, do this

    //* Pattern Matching
    Switch,   // value to compare against
    Case,     // specific value
    Default,  // if no values matched, do this

    //* Loops
    For,
    While,
    Repeat,    // execute a block of code `n` times -- don't care about index
    Do,        // run once before checking condition (in while loop)
    Break,     // stop loop
    Continue,  // skip this iteration

    //* Functions
    Func,    // function
    Lambda,  // anonymous function
    Return,  // function returns

    //* Modules & Scoping
    Import,  // bring in another module
    Module,  // declare as a module
    As,      // alias a module

    //* Encapsulation
    Bundle,     // like C's struct
    Enum,       // list of named constants
    Blueprint,  // like a class

    //* Misc Operators
    Alias,         // type aliasing (alias `a` as `b`)
    TypeOf,        // get the type of a variable
    Cast,          // cast to a new type (e.g., cast<new_type>)
    __KeywordEnd,  // Marker for the end of keyword token types -- not to be used as an actual token type

    //~ Operators
    //* Arithmetic Operators
    __OperatorStart,  // Marker for the start of operator token types -- not to be used as an actual token type
    Plus,             // `+`
    Minus,            // `-`
    Mul,              // `*`
    Div,              // `/`
    FloorDiv,         // `//`
    Mod,              // `%`
    Exp,              // `^^`
    Inc,              // `++`
    Dec,              // `--`
    UnaryPlus,        // `+` (unary plus)
    UnaryMinus,       // `-` (unary minus)

    //* Arithmetic Assignment Operators
    // Overrides the value of the variable in place (e.g. x += 2 is the same as x = x + 2)
    PlusAssign,      // `+=`
    MinusAssign,     // `-=`
    MulAssign,       // `*=`
    DivAssign,       // `/=`
    FloorDivAssign,  // `//=`
    ModAssign,       // `%=`
    ExpAssign,       // `^^=`

    //* Comparison Operators
    GreaterThan,         // `>`
    GreaterThanOrEqual,  // `>=`
    LessThan,            // `<`
    LessThanOrEqual,     // `<=`,
    Equal,               // `==`
    NotEqual,            // `!=`

    //* Boolean Operators
    And,  // `&&`
    Or,   // `||`
    Not,  // `!`

    //* Bitwise Operators
    BitAnd,     // `&`
    BitOr,      // `|`
    BitNot,     // `~`
    BitXor,     // `^`
    BitLShift,  // `<<`
    BitRShift,  // `>>`

    //* Bitwise Assignment Operators
    // Overrides the value of the variable in place (e.g. x &= y is the same as x = x & y)
    BitAndAssign,     // `&=`
    BitOrAssign,      // `|=`
    BitNotAssign,     // `~=`
    BitXorAssign,     // `^=`
    BitLShiftAssign,  // `<<=`
    BitRShiftAssign,  // `>>=`

    //* Pointer Operators
    AddressOf,    // `&`
    Dereference,  // `*`

    //* Access Operators
    MemberAccess,     // `.`
    ScopeResolution,  // `::`

    //* Misc
    Assignment,     // `=`
    Arrow,          // `->`
    Ellipsis,       // `...`
    __OperatorEnd,  // Marker for the end of operator token types -- not to be used as an actual token type

};

}  // namespace lexer
MANGANESE_END

#endif  // TOKEN_H