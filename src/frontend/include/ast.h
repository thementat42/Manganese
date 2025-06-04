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

A block is a vector of statements
*/

#ifndef AST_H
#define AST_H

#include <memory>
#include <string>
#include <vector>

#include "../../core/include/keywords.h"
#include "../../core/include/operators.h"
#include "../../core/include/token.h"
#include "../../global_macros.h"
using float32_t = float;
using float64_t = double;

namespace manganese {

namespace parser {
class Parser;
}

namespace ast {
class Expression;
class Statement;
using ExpressionPtr = std::unique_ptr<Expression>;
using StatementPtr = std::unique_ptr<Statement>;
using Block = std::vector<StatementPtr>;

//~ Base Nodes
class ASTNode {
   protected:
    size_t line, column;

   public:
    virtual ~ASTNode() = default;

    friend parser::Parser;
};

class Statement : public ASTNode {
   public:
    virtual ~Statement() = default;
};

class Expression : public ASTNode {
   public:
    virtual ~Expression() = default;
};

//~ Expressions

//* Literal Expressions
class NumberExpression : public Expression {
   protected:
    float64_t value;

   public:
    NumberExpression(float64_t _value) : value(_value) {};
};

class StringExpression : public Expression {
   protected:
    std::string value;
};

class SymbolExpression : public Expression {
   protected:
    std::string value;
};

//* Complex Expressions
class BinaryExpression : public Expression {
   protected:
    ExpressionPtr left, right;
    core::OperatorType op;
};

//~ Statements
class ExpressionStatement {
   protected:
    ExpressionPtr expression;
};

}  // namespace ast
}  // namespace manganese

#endif  // AST_H
