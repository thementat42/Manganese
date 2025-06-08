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

#if DEBUG
#define AST_DEBUG_OVERRIDES                \
    std::string toString() const override; \
    void dump(std::ostream& os, int indent = 0) const override;
#else                        // ^^ DEBUG vv !DEBUG
#define AST_DEBUG_OVERRIDES  // Don't have these methods in release builds
#endif                       // DEBUG

#include <memory>
#include <string>
#include <vector>

#include "../../global_macros.h"
#include "token.h"

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

enum class Visibility : char {
    Public = 0,
    ReadOnly = 1,
    Private = 2,
};

//~ Base Nodes
class ASTNode {
   protected:
    size_t line, column;

   public:
    virtual ~ASTNode() noexcept = default;

    #if DEBUG
    virtual std::string toString() const = 0;
    virtual void dump(std::ostream& os, int indent = 0) const = 0;
    #endif // DEBUG

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
    AST_DEBUG_OVERRIDES
};

class StringExpression : public Expression {
   protected:
    std::string value;

   public:
    StringExpression(const std::string& _value) : value(std::move(_value)) {};

    const std::string& getValue() const { return value; }
    AST_DEBUG_OVERRIDES
};

class SymbolExpression : public Expression {
   protected:
    std::string value;

   public:
    SymbolExpression(const std::string& _value) : value(std::move(_value)) {}

    const std::string& getValue() const { return value; }
    AST_DEBUG_OVERRIDES
};

//* Complex Expressions
class BinaryExpression : public Expression {
   protected:
    ExpressionPtr left, right;
    lexer::TokenType op;

   public:
    BinaryExpression(ExpressionPtr _left, lexer::TokenType _op, ExpressionPtr _right)
        : left(std::move(_left)), right(std::move(_right)), op(_op) {};

    const Expression& getLeft() const { return *left; }
    const Expression& getRight() const { return *right; }
    lexer::TokenType getOperator() const { return op; }
    AST_DEBUG_OVERRIDES
};

//~ Statements
class ExpressionStatement : public Statement {
   protected:
    ExpressionPtr expression;

   public:
    ExpressionStatement(ExpressionPtr _expression) : expression(std::move(_expression)) {};

    const Expression& getExpression() const { return *expression; }
    AST_DEBUG_OVERRIDES
};

class VariableDeclarationStatement : public Statement {
   protected:
    std::string name;
    bool isConst;
    Visibility visibility;
    ExpressionPtr value;
    // primitiveType type;

   public:
    VariableDeclarationStatement(std::string _name, bool _isConst, Visibility _visibility, ExpressionPtr _value)
        : name(std::move(_name)), isConst(_isConst), visibility(_visibility), value(std::move(_value)) {};
    VariableDeclarationStatement(std::string _name, bool _isConst, ExpressionPtr _value)
        : name(std::move(_name)), isConst(_isConst), visibility(Visibility::ReadOnly), value(std::move(_value)) {};
    AST_DEBUG_OVERRIDES
};

}  // namespace ast
}  // namespace manganese

#endif  // AST_H
