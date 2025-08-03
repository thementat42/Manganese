/**
 * @file ast.h
 * @brief Central header for the AST components
 */

/*
We want one object for each language construct
In Manganese, there are:
    - Expressions (produce a value) -- Have to handle the angle bracket ambiguity here once generics get implemented
        - Binary Operator Expressions (x + y, x && y)
        - Unary Operator Expressions (++x, *y)
        - Cast expressions (x as int)
        - Variable Expressions
        - Member Access
        - Scope Resolution
        - Literals
            - Int, Float, Char, Bool, String literals
    - Statements (perform an action)
        - Variable declaration (+ qualifiers sometimes)
        - Function Declarations
        - Module Declarations
        - Bundle Declarations
        - Blueprint Declarations (not yet)
        - Enum Declarations
        - Control flow
            - If/elif/else
            - Switch/case
            - For loops (repeat is syntactic sugar, see if it can be expanded somehow)
            - While loops
            - Jump statements (break, continue)
            - Return Statements
        - Assignment statements
        - Function calls
        - Lambdas (eventually)

A block is a vector of statements
*/

#ifndef MANGANESE_INCLUDE_FRONTEND_AST_H
#define MANGANESE_INCLUDE_FRONTEND_AST_H

#include <frontend/ast/ast_base.hpp>
#include <frontend/ast/ast_expressions.hpp>
#include <frontend/ast/ast_statements.hpp>
#include <frontend/ast/ast_types.hpp>

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_H
