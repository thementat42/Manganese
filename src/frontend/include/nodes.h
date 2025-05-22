/**
 * @file nodes.h
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

#ifndef NODES_H
#define NODES_H

#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "../../global_macros.h"
#include "../../core/include/keywords.h"
#include "../../core/include/operators.h"
#include "../../core/include/token.h"

// todo? std::pmr::vector instead?

MANG_BEGIN
namespace parser {
// Forward declarations
class Parser;
class Visitor;
class ASTNode;
class Expression;
class Statement;

using NodePtr = std::unique_ptr<ASTNode>;
using ExpressionPtr = std::unique_ptr<Expression>;
using StatementPtr = std::unique_ptr<Statement>;
using ExpressionList = std::vector<ExpressionPtr>;
using Block = std::vector<StatementPtr>;
using str = std::string;

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

//~ High level AST nodes
class ASTNode {
    // TODO: Add llvm codegen method
   public:
    virtual ~ASTNode() = default;
    virtual std::string toString() const = 0;
    virtual void accept(Visitor& visitor) = 0;
    friend Parser;
    friend Visitor;
};

class Expression : public ASTNode {
   public:
    virtual ~Expression() = default;
    virtual void expr();
};

class Statement : public ASTNode {
   public:
    virtual ~Statement() = default;
    virtual void stmt();
};

//~ Expressions

//* Literals

class NumberExpression : public Expression {
   protected:
    number_t value;

   public:
    NumberExpression(number_t value);
};

class StringExpression : public Expression {
   protected:
    str value;

   public:
    StringExpression(str value);
};

class SymbolExpression : public Expression {
   protected:
    str value;

   public:
    SymbolExpression(str value);
};

//* Complex Expressions

class BinaryExpression : public Expression {
   protected:
    ExpressionPtr left, right;
    lexer::OperatorType op;

   public:
    BinaryExpression(ExpressionPtr left, lexer::OperatorType op, ExpressionPtr right);
};

class UnaryExpression : public Expression {
    protected:
    ExpressionPtr operand;
    lexer::OperatorType op;
    public:
    UnaryExpression(lexer::OperatorType op, ExpressionPtr operand);
};

// ~ Statements

/**
 * @brief Wrapper around an expression so that the output of the parser is a Block (a vector of statements)
 */
class ExpressionStatement : public Statement {
   protected:
    ExpressionPtr expression;
};

}  // namespace parser
MANG_END

#endif  // NODES_H