/**
 * @file ast.h
 * @brief This file contains the definition of the different AST nodes for the Manganese compiler.
 */

/*
We want one object for each language construct
In Manganese, there are:
    - Expressions (produce a value) -- Have to handle the angle bracket ambiguity here once generics get implemented
        - Binary Operator Expressions (x + y, x && y)
        - Unary Operator Expressions (++x, @y)
        - Cast expressions (cast<int>(x))
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

A block is an std::vector of statements
*/

#ifndef AST_H
#define AST_H


#include "../../global_macros.h"

MANG_BEGIN
//* Forward declarations
namespace parser {
    class Parser;
}  // namespace parser

namespace visitor {
    class Visitor;
}  // namespace visitor

namespace ast {
//* High-Level constructs
    class ASTNode {
    public:
    virtual ~ASTNode() = default;
    friend parser::Parser;
    friend visitor::Visitor;
};

class Expression : public ASTNode {
};

class Statement : public ASTNode {
};

//* Expressions


//* Statements

}  // namespace ast
MANG_END

#endif // AST_H
