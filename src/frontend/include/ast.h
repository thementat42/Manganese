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

#include "token.h"
#include "../../global_macros.h"

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

using number_t = std::variant<
    int8_t,
    uint8_t,
    int16_t,
    uint16_t,
    int32_t,
    uint32_t,
    int64_t,
    uint64_t,
    float,
    double>;

//~ Base Nodes
class ASTNode {
   protected:
    size_t line, column;

   public:
    virtual ~ASTNode() noexcept = default;
    virtual std::string toString() const = 0;
    
    size_t getLine() const { return line; }
    size_t getColumn() const { return column; }

    friend parser::Parser;
};

class Statement : public ASTNode {
   public:
    virtual ~Statement() noexcept = default;
};

class Expression : public ASTNode {
   public:
    virtual ~Expression() noexcept = default;
};

//~ Expressions

//* Literal Expressions
class NumberExpression : public Expression {
   protected:
    number_t value;

   public:
    NumberExpression(number_t _value) : value(_value) {};
    
    const number_t& getValue() const { return value; }
    std::string toString() const override;
};

class StringExpression : public Expression {
   protected:
    std::string value;

   public:
    StringExpression(const std::string& _value) : value(std::move(_value)) {};
    
    const std::string& getValue() const { return value; }
    std::string toString() const override;
};

class SymbolExpression : public Expression {
   protected:
    std::string value;
   public:
    SymbolExpression(const std::string& _value) : value(std::move(_value)) {}
    
    const std::string& getValue() const { return value; }
    std::string toString() const override;
};

//* Complex Expressions
class BinaryExpression : public Expression {
   protected:
    ExpressionPtr left, right;
    lexer::TokenType op;

   public:
    BinaryExpression(ExpressionPtr _left, lexer::TokenType _op, ExpressionPtr _right)
        : left(std::move(_left)), op(_op), right(std::move(_right)) {};
    
    const Expression& getLeft() const { return *left; }
    const Expression& getRight() const { return *right; }
    lexer::TokenType getOperator() const { return op; }
    std::string toString() const override;
};

//~ Statements
class ExpressionStatement : public Statement {
   protected:
    ExpressionPtr expression;

   public:
    ExpressionStatement(ExpressionPtr _expression) : expression(std::move(_expression)) {};
    
    const Expression& getExpression() const { return *expression; }
    std::string toString() const override;
};

}  // namespace ast
}  // namespace manganese

#endif  // AST_H
