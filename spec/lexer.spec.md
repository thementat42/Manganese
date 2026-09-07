# Lexical Specification

- The lexer uses a "maximum munch" principle, consuming the longest possible sequence of characters that can form a token
  - For example, `++` will always be lexed as the increment operator, never as two consecutive `+` signs

## Whitespace and Comments

- Whitespace is ignored, except where it temrinates a token
  - Whitespace refers to spaces (` `), newlines (`\n`), tabs (`\t` and `\v`), form feeds (`\f`) and carriage returns (`\r`)
- A `#` begins a single-line comment, extending to the end of the line (i.e. until a `\n`, 0xA)
- A `/*` begins a block comment, which extends until the next `*/`
  - Nested block comments are supported; each `/*` must be matched by a correspoinding `*/`

## String Literals

- Character literals begin and end with `'` and can contain at most one character
- String literals begin and end with `"`
- Raw string literals begin with `r` followed immediately (i.e. no whitespace) by one or more backticks
  - The number of backticks after the `r` determines the delimiter length
  - The literal ends at the next occurence of the same number of consecutive backticks
  - The contents are interpreted literally; escape sequences are not processed
  - Raw strings must contain at least one character

Character and string literals support the following escape sequences:

- `\\` to type a backslash inside a string (to avoid interpreting something as an escape sequence)
- `\'` to type a single quote (unescaped `'` are permitted in string literals)
- `\"` to type a double quote (a single, unescaped `"` is permitted as a character literal)
- `\a` for alarm (produces a beep from the system speaker; effects vary)
- `\b` for a backspace (moves the cursor back one character)
- `\f` for form feed (moves the cursor to the start of the next logical page)
- `\n` for a new line (moves the cursor to the next line)
  - Depending on the operating system, this may also move the cursor to the start of the line rather than staying at the same position on a new line
- `\r` for carriage return (moves the cursor to the start of the line)
- `\t` for a horizontal tab
- `\v` for a vertical tab
- `\0` for a null terminator

## Numeric Literals

- Numeric literals begin with a digit (0-9)
- Numeric literals can begin with one of the following (case-insensitive) prefixes:
  - 0b for binary (the digits following this must be either `0` or `1`)
  - 0o for octal (the digits following this must be between `0` and `7`, inclusive)
  - 0x for hexadecimal (the digits must be between `0` and `9` or `a` and `f` or `A` and `F`)
  - A leading zero does not imply an octal literal
- If no prefix is specified, the literal is treated as an ordinary decimal literal
- Numeric literals can contain underscores (`_`) as digit separators; these are ignored

## Keywords and Identifiers

- An identifier may contain alphanumeric characters or underscores
- Identifiers may not begin with a digit (i.e. they must begin with alphabetic characters or `_`)
- If an identifier exactly matches a keyword, it is treated as such and thus cannot be used as an identifier (e.g. as the name of a variable)
  - per the maximum munch rule, if an identifier begins with the same sequence of characters as a keyword but contains additional characters after that, it will not be interpreted as a keyword
  - for the complete list of keywords, see [](/spec/keywords.spec)

## Symbols

- Characters that are not whitespace, comments, identifiers or literals are interpreted as symbols by the lexer
- Symbols must be a valid operator (see [](/spec/operators.spec))
