# Frontend

This directory contains the frontend components of the Manganese compiler, responsible for the initial phases of the compilation process.

## Overview

The frontend is responsible for:

1. Converting source code into tokens (Lexer)
2. Building an Abstract Syntax Tree from tokens (Parser)
3. Performing semantic analysis on the AST (Semantic Analyzer)

## Components

### [Lexer](/lexer)

The lexer (tokenizer) takes in raw source code and produces a stream of tokens. These tokens represent the smallest meaningful units in the language, such as keywords, identifiers, operators, and literals.

The lexer handles:

- Tokenization of source code
- Recognition of keywords, operators, and literals
- Handling of comments and whitespace
- Basic error reporting for invalid tokens

Key files:

- `lexer.cpp/h`: Main lexer implementation
- `token.cpp/h`: Token structure definition
- `keywords.cpp/h`: Keyword management
- `operators.cpp/h`: Operator management

### [Parser](/parser)

The parser takes the token stream from the lexer and constructs an Abstract Syntax Tree (AST) that represents the hierarchical structure of the program.

The parser is responsible for:

- Ensuring code follows Manganese grammar rules
- Building a structured representation of the program
- Reporting syntax errors
- Managing precedence of operations

### [Semantic Analyzer](/semantic_analyzer)

The semantic analyzer checks the AST for semantic errors that can't be caught during parsing.

It handles:

- Type checking
- Scope validation
- Symbol resolution
- Ensuring correct usage of language constructs

## Error Handling

The frontend components provide detailed error messages to help identify and fix issues code. Errors detected during the frontend phase include:

- Lexical errors (invalid tokens, unterminated strings)
- Syntax errors (invalid grammar)
- Semantic errors (type mismatches, undefined variables)

## Interaction with Other Components

The frontend passes the processed AST to the middle-end of the compiler, which handles IR generation and optimization before code generation occurs in the backend.
