/**
 * @file ast.h
 * @brief This file contains the definition of the different AST nodes for the Manganese compiler.
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

#ifndef AST_H
#define AST_H

#include <global_macros.h>
#include <utils/stox.h>

#include <memory>
#include <string>
#include <vector>

#include "token.h"

namespace Manganese {

namespace parser {
class Parser;
}

// TODO: Add line and column setting methods to the derived classes

namespace ast {
class Expression;
class Statement;
using ExpressionPtr = std::unique_ptr<Expression>;
using StatementPtr = std::unique_ptr<Statement>;
using Block = std::vector<StatementPtr>;

enum class Visibility : char {
    Public = 0,
    ReadOnly = 1,
    Private = 2,
};

//~ Base Nodes

/**
 * @brief Base class for all AST nodes
 */
class ASTNode {
   protected:
    size_t line, column;

   public:
    virtual ~ASTNode() noexcept = default;

    /**
     * @brief Convert the AST node to a string representation
     * @return A string representation of the AST node
     */
    virtual std::string toString() const = 0;

    /**
     * @brief Dump the AST node to an output stream
     * @param os The output stream to dump to
     * @param indent The indentation level for pretty-printing
     */
    virtual void dump(std::ostream& os, int indent = 0) const = 0;

    size_t getLine() const { return line; }
    size_t getColumn() const { return column; }

    friend parser::Parser;
};

/**
 * @brief The base class for all statements in the AST
 */
class Statement : public ASTNode {
   public:
    virtual ~Statement() noexcept = default;
};

/**
 * @brief The base class for all expressions in the AST
 */
class Expression : public ASTNode {
   public:
    virtual ~Expression() noexcept = default;
};

//~ Expressions

//* Literal Expressions

/**
 * @brief Represents a numeric literal in the AST
 */
class NumberExpression : public Expression {
   protected:
    number_t value;

   public:
    /**
     * @brief Initialize a NumberExpression node
     * @param _value The numeric value of the expression (can be any numeric type)
     */
    explicit NumberExpression(number_t _value) : value(_value) {};

    const number_t& getValue() const { return value; }
    std::string toString() const override { return ""; }
    void dump(std::ostream& os, int indent = 0) const override {}
};

class CharExpression : public Expression {
   protected:
    wchar_t value;

   public:
    /**
     * @brief Initialize a CharExpression node
     * @param _value The character value of the expression (wchar_t)
     */
    explicit CharExpression(wchar_t _value) : value(_value) {};
    explicit CharExpression(char _value) : value(static_cast<wchar_t>(_value)) {};

    wchar_t getValue() const { return value; }
    std::string toString() const override { return ""; }
    void dump(std::ostream& os, int indent = 0) const override {}
};

/**
 * @brief Represents a string literal in the AST
 */
class StringExpression : public Expression {
   protected:
    std::string value;

   public:
    /**
     * @brief Initialize a StringExpression node
     * @param _value The string value of the expression (std::string)
     */
    explicit StringExpression(const std::string& _value) : value(std::move(_value)) {};

    /**
     * @brief Initialize a StringExpression node
     * @param _value The string value of the expression (const char*)
     */
    explicit StringExpression(const char* _value) : value(_value) {};

    const std::string& getValue() const { return value; }
    std::string toString() const override { return ""; }
    void dump(std::ostream& os, int indent = 0) const override {}
};

/**
 * @brief Represents a symbol (identifier) in the AST
 */
class SymbolExpression : public Expression {
   protected:
    std::string value;

   public:
    /**
     * @brief Initialize a SymbolExpression node
     * @param _value The symbol value of the expression (std::string)
     */
    explicit SymbolExpression(const std::string& _value) : value(std::move(_value)) {}

    const std::string& getValue() const { return value; }
    std::string toString() const override { return ""; }
    void dump(std::ostream& os, int indent = 0) const override {}
};

//* Complex Expressions

/**
 * @brief Represents a binary expression in the AST
 */
class BinaryExpression : public Expression {
   protected:
    ExpressionPtr left, right;
    lexer::TokenType op;

   public:
    /**
     * @brief Initialize a BinaryExpression node
     * @param _left The left operand of the expression
     * @param _op The operator token type (e.g., +, -, *, /)
     * @param _right The right operand of the expression
     */
    BinaryExpression(ExpressionPtr _left, lexer::TokenType _op, ExpressionPtr _right)
        : left(std::move(_left)), right(std::move(_right)), op(_op) {};

    const Expression& getLeft() const { return *left; }
    const Expression& getRight() const { return *right; }
    lexer::TokenType getOperator() const { return op; }
    std::string toString() const override { return ""; }
    void dump(std::ostream& os, int indent = 0) const override {}
};

/**
 * @brief Represents a prefixed unary expression in the AST
 */
class PrefixExpression : public Expression {
   protected:
    lexer::TokenType op;
    ExpressionPtr right;

   public:
    /**
     * @brief Initialize a PrefixExpression node
     * @param _op The operator token type (e.g., ++, --, !)
     * @param _right The operand of the expression
     */
    PrefixExpression(lexer::TokenType _op, ExpressionPtr _right)
        : op(_op), right(std::move(_right)) {}

    const Expression& getRight() const { return *right; }
    lexer::TokenType getOperator() const { return op; }

    std::string toString() const override { return ""; }
    void dump(std::ostream& os, int indent = 0) const override {}
};

class AssignmentExpression : public Expression {
   protected:
    ExpressionPtr assignee, value;
    lexer::TokenType op;

   public:
    /**
     * @brief Initialize an AssignmentExpression node
     * @param _assignee The expression being assigned to (left-hand side)
     * @param _op The assignment operator token type (e.g., =, +=, -=)
     * @param _value The value being assigned (right-hand side)
     */
    AssignmentExpression(ExpressionPtr _assignee, lexer::TokenType _op, ExpressionPtr _value)
        : assignee(std::move(_assignee)), value(std::move(_value)), op(_op) {}

    const Expression& getAssignee() const { return *assignee; }
    const Expression& getValue() const { return *value; }
    lexer::TokenType getOperator() const { return op; }
    std::string toString() const override { return ""; }
    void dump(std::ostream& os, int indent = 0) const override {}
};

//~ Statements

/**
 * @brief Wrapper class to convert an expression into a statement
 */
class ExpressionStatement : public Statement {
   protected:
    ExpressionPtr expression;

   public:
    /**
     * @brief Initialize an ExpressionStatement node
     * @param _expression The expression to wrap in the statement
     */
    explicit ExpressionStatement(ExpressionPtr _expression) : expression(std::move(_expression)) {};

    const Expression& getExpression() const { return *expression; }
    std::string toString() const override { return ""; }
    void dump(std::ostream& os, int indent = 0) const override {}
};

/**
 * @brief Represents a variable declaration statement in the AST
 */
class VariableDeclarationStatement : public Statement {
   protected:
    bool isConst;
    std::string name;
    Visibility visibility;
    ExpressionPtr value;
    // primitiveType type;

   public:
    /**
     * @brief Initialize a VariableDeclarationStatement node
     * @param _name The name of the variable
     * @param _isConst Whether the variable is a constant (immutable)
     * @param _visibility The visibility of the variable (public, read-only, private)
     * @param _value The initial value of the variable
     */
    VariableDeclarationStatement(bool _isConst, std::string _name, Visibility _visibility, ExpressionPtr _value)
        : isConst(_isConst), name(std::move(_name)), visibility(_visibility), value(std::move(_value)) {};

    bool isConstant() const { return isConst; }
    const std::string& getName() const { return name; }
    Visibility getVisibility() const { return visibility; }
    const Expression& getValue() const { return *value; }
    // const primitiveType& getType() const { return type; }

    std::string toString() const override { return ""; }
    void dump(std::ostream& os, int indent = 0) const override {}
};

}  // namespace ast
}  // namespace Manganese

#endif  // AST_H
