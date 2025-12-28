#ifndef MANGANESE_INCLUDE_FRONTEND_AST_LEXER_TOKEN_TYPE_HPP
#define MANGANESE_INCLUDE_FRONTEND_AST_LEXER_TOKEN_TYPE_HPP

#include <stdint.h>

#include <global_macros.hpp>

namespace Manganese {

namespace lexer {

/**
 * @brief Defines all supported token types in Manganese
 *
 * This enum represents all tokens recognized by the compiler,
 * categorized by their functional purpose (keywords, identifiers, etc.).
 */
enum class TokenType : uint8_t {
    //~ Basic
    Identifier,  // variables, functions
    StrLiteral,  // "text"
    CharLiteral,  // 'a'
    IntegerLiteral,  // Whole Number
    FloatLiteral,  // Floating point number

    /*
    Keyword and Operator have generic values here for simplicity in other parts of the compiler
    The token constructor uses this to assign a specific enum value (see below)

    For example, the lexer can just say "this is a keyword" and trust the token will be correctly assigned to that
    keyword, without having to check what keyword it
    */
    Keyword,  // any keyword
    Operator,  // any operator

    //~ Brackets
    LeftParen,  // (
    RightParen,  // )
    LeftBrace,  // {
    RightBrace,  // }
    LeftSquare,  // [
    RightSquare,  // ]

    //~ Punctuation
    Semicolon,  // ;
    Colon,  // :
    Comma,  // ,

    //~ Misc
    EndOfFile,

    //~ Keywords
    _keywordStart,  // Marker for the start of keyword token types -- not to be used as an actual token type
    //* Type Qualifiers
    Let,  // variable declaration
    Mut, // specify something as mutable
    Ptr,  // pointer variables

    //* Access Levels
    Private,
    ReadOnly,  // readable, but can't be modified, outside the module
    Public,  // can be read and modified outside the module

    //* Primitive Types
    Int8,  // 8 bit int
    Int16,  // 16 bit int
    Int32,  // 32 bit int
    Int64,  // 64 bit int
    UInt8,  // unsigned 8 bit int
    UInt16,  // unsigned 16 bit int
    UInt32,  // unsigned 32 bit int
    UInt64,  // unsigned 64 bit int
    Float32,  // 32-bit float (single precision)
    Float64,  // 64-bit float (double precision)
    Char,  // single character
    Bool,  // true or false
    True,  // boolean true
    False,  // boolean false
    String,  // sequence of characters

    //* Conditionals
    If,  // check a condition
    Elif,  // Alternate condition
    Else,  // if all previous conditions were false, do this

    //* Pattern Matching
    Switch,  // value to compare against
    Case,  // specific value
    Default,  // if no values matched, do this

    //* Loops
    For,
    While,
    Repeat,  // execute a block of code `n` times -- don't care about index
    Do,  // run once before checking condition (in while loop)
    Break,  // stop loop
    Continue,  // skip this iteration

    //* Functions
    Func,  // function
    Lambda,  // anonymous function
    Return,  // function returns

    //* Modules & Scoping
    Import,  // bring in another module
    Module,  // declare as a module
    As,  // type casting, or module aliasing in an import statement

    //* Encapsulation
    Aggregate,  // like C's struct
    Enum,  // list of named constants
    Blueprint,  // like a class

    //* Misc Operators
    Alias,  // type aliasing (alias `a` as `b`)
    // TypeOf,  // get the type of a variable
    _keywordEnd,  // Marker for the end of keyword token types -- not to be used as an actual token type

    //~ Operators
    //* Arithmetic Operators
    _operatorStart,  // Marker for the start of operator token types -- not to be used as an actual token type
    Plus,  // `+`
    Minus,  // `-`
    Mul,  // `*`
    Div,  // `/`
    FloorDiv,  // `//`
    Mod,  // `%`
    Exp,  // `^^`
    Inc,  // `++`
    Dec,  // `--`
    UnaryPlus,  // `+` (unary plus)
    UnaryMinus,  // `-` (unary minus)

    //* Arithmetic Assignment Operators
    // Overrides the value of the variable in place (e.g. x += 2 is the same as x = x + 2)
    PlusAssign,  // `+=`
    MinusAssign,  // `-=`
    MulAssign,  // `*=`
    DivAssign,  // `/=`
    FloorDivAssign,  // `//=`
    ModAssign,  // `%=`
    ExpAssign,  // `^^=`

    //* Comparison Operators
    GreaterThan,  // `>`
    GreaterThanOrEqual,  // `>=`
    LessThan,  // `<`
    LessThanOrEqual,  // `<=`,
    Equal,  // `==`
    NotEqual,  // `!=`

    //* Boolean Operators
    And,  // `&&`
    Or,  // `||`
    Not,  // `!`

    //* Bitwise Operators
    BitAnd,  // `&`
    BitOr,  // `|`
    BitNot,  // `~`
    BitXor,  // `^`
    BitLShift,  // `<<`
    BitRShift,  // `>>`

    //* Bitwise Assignment Operators
    // Overrides the value of the variable in place (e.g. x &= y is the same as x = x & y)
    BitAndAssign,  // `&=`
    BitOrAssign,  // `|=`
    BitNotAssign,  // `~=`
    BitXorAssign,  // `^=`
    BitLShiftAssign,  // `<<=`
    BitRShiftAssign,  // `>>=`

    //* Pointer Operators
    AddressOf,  // `&`
    Dereference,  // `*`

    //* Access Operators
    MemberAccess,  // `.`
    ScopeResolution,  // `::`

    //* Misc
    Assignment,  // `=`
    Arrow,  // `->`
    Ellipsis,  // `...`
    At,  // `@`
    _operatorEnd,  // Marker for the end of operator token types -- not to be used as an actual token type
    Unknown,  // For truly catastrophic failures (e.g, unrecognized character)
};

}  // namespace lexer

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_LEXER_TOKEN_TYPE_HPP