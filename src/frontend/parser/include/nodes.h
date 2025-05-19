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
            - Garbage
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

#include "../../../global_macros.h"
#include "../../lexer/include/keywords.h"
#include "../../lexer/include/operators.h"
#include "../../lexer/include/token.h"

// todo? std::pmr::vector instead?

MANG_BEGIN
namespace parser {
// Forward declarations
class Parser;
class Visitor;
class ASTNode;
class ExpressionNode;
class StatementNode;

using NodePtr = std::unique_ptr<ASTNode>;
using ExpressionPtr = std::unique_ptr<ExpressionNode>;
using StatementPtr = std::unique_ptr<StatementNode>;
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

/**
 * @brief For variable nodes -- gives what data type the variable is storing
 */
enum class DataType {
    Int8,       // 8-bit signed integers
    UInt8,      // 8-bit unsigned integers
    Int16,      // 16-bit signed integers
    UInt16,     // 16-bit unsigned integers
    Int32,      // 32-bit signed integers
    UInt32,     // 32-bit unsigned integers
    Int64,      // 64-bit signed integers
    UInt64,     // 64-bit unsigned integers
    Float32,    // 32-bit floating-point number
    Float64,    // 64-bit floating point number (`double` in C/C++)
    Char,       // A single character
    Bool,       // A boolean
    Bundle,     // A bundle (C-style `struct`)
    Blueprint,  // A blueprint (`class`)
    Enum,       // Named constants
};

class ASTNode {
   protected:
    size_t line, column;
    /* We don't explicitly declare a children vector here
    instead, any node which has sub-components will explicitly declare them
    (e.g. a binary operator node will have an LHS, op and RHS node as children)
    */

   public:
    virtual ~ASTNode() = default;
    virtual void accept(Visitor& visitor) = 0;

    friend Parser;
    friend Visitor;
};

class ExpressionNode : public ASTNode {
   public:
    virtual ~ExpressionNode() = default;
};

class StatementNode : public ASTNode {
   public:
    virtual ~StatementNode() = default;
};

//~ Expressions
class NumericLiteralNode : public ExpressionNode {
   protected:
    const number_t value;
    const DataType type;

   public:
    NumericLiteralNode(number_t _value, DataType _type);
};

class CharLiteralNode : public ExpressionNode {
   protected:
    const char value;

   public:
    explicit CharLiteralNode(char _value);
};

class BoolLiteralNode : public ExpressionNode {
   protected:
    const bool value;

   public:
    explicit BoolLiteralNode(bool _value);
};

class StringLiteralNode : public ExpressionNode {
   protected:
    const str value;

   public:
    explicit StringLiteralNode(str _value);
};

class IdentifierNode : public ExpressionNode {
   protected:
    str name;

   public:
    explicit IdentifierNode(str name);
};

class FunctionCallNode : public ExpressionNode {
   protected:
    ExpressionPtr callee;
    std::vector<ExpressionPtr> arguments;

   public:
    FunctionCallNode(ExpressionPtr callee, ExpressionList args);
};

class LambdaExpressionNode : public ExpressionNode {
   protected:
    std::vector<std::pair<str, DataType>> parameters;
    DataType returnType;
    ExpressionPtr body;

   public:
    LambdaExpressionNode(
        std::vector<std::pair<str, DataType>> params,
        DataType returnType,
        ExpressionPtr body);
};

class BinaryOperatorNode : public ExpressionNode {
   protected:
    lexer::OperatorType op;
    ExpressionPtr LHS, RHS;

   public:
    BinaryOperatorNode(lexer::OperatorType op, ExpressionPtr leftOperand, ExpressionPtr rightOperand);
};

class UnaryOperatorNode : public ExpressionNode {
   protected:
    lexer::OperatorType op;
    ExpressionPtr operand;

   public:
    UnaryOperatorNode(lexer::OperatorType op, ExpressionPtr operand);
};

class MemberAccessNode : public ExpressionNode {
    ExpressionPtr object;
    str member;

public:
    MemberAccessNode(ExpressionPtr object, str member);
};

class ScopeResolutionNode : public ExpressionNode {
    ExpressionPtr scope;
    str symbol;

public:
    ScopeResolutionNode(ExpressionPtr scope, str symbol);
};

//~ Statements

class VariableDeclarationNode : public StatementNode {
   protected:
    str name;
    DataType type;
    std::optional<ExpressionPtr> initializer;  // optional initializer expression
    bool isPublic, isReadOnly, isConst;

   public:
    explicit VariableDeclarationNode(const str name,
                                     DataType type, std::optional<ExpressionPtr> initializer,
                                     bool isPublic = false, bool isReadOnly = false, bool isConst = false);
};

class FunctionDeclarationNode : public StatementNode {
   protected:
    str name;
    std::vector<std::pair<str, DataType>> parameters;
    // std::optional<std::vector<str>> genericParameters;
    Block body;
    DataType returnType;

   public:
    FunctionDeclarationNode(str name, Block body, DataType returnType);
};

class AssignmentNode : public StatementNode {
   protected:
    str targetName;          // where result is stored
    lexer::OperatorType op;  // e.g. +=, &=, =
    ExpressionPtr value;

   public:
    AssignmentNode(str name, lexer::OperatorType op, ExpressionPtr value);
};

class IfStatementNode : public StatementNode {
   protected:
    ExpressionPtr condition;
    Block trueBlock;
    Block falseBlock;

   public:
    IfStatementNode(ExpressionPtr condition, Block trueBlock, Block falseBlock);
};

class ElifStatementNode : public StatementNode {
   protected:
    ExpressionPtr condition;
    Block trueBlock;

   public:
    ElifStatementNode(ExpressionPtr condition, Block trueBlock);
};

class ElseStatementNode : public StatementNode {
   protected:
    Block block;

   public:
    explicit ElseStatementNode(Block block);
};

class ForLoopNode : public StatementNode {
   protected:
    ExpressionPtr iterator;
    ExpressionPtr start;
    ExpressionPtr end;
    Block body;

   public:
    ForLoopNode(ExpressionPtr iterator, ExpressionPtr start, ExpressionPtr end, Block body);
};

class WhileLoopNode : public StatementNode {
   protected:
    ExpressionPtr condition;
    Block body;

   public:
    WhileLoopNode(ExpressionPtr condition, Block body);
};

class BreakStatementNode : public StatementNode {
   protected:
    // No additional members needed for break statement
   public:
    BreakStatementNode();
};

class ContinueStatementNode : public StatementNode {
   protected:
    // No additional members needed for continue statement
   public:
    ContinueStatementNode();
};

class ReturnStatementNode : public StatementNode {
   protected:
    ExpressionPtr returnValue;

   public:
    explicit ReturnStatementNode(ExpressionPtr returnValue);
};

class SwitchStatementNode : public StatementNode {
    ExpressionPtr condition;
    std::vector<std::pair<ExpressionPtr, Block>> cases;
    Block defaultBlock;

   public:
    SwitchStatementNode(
        ExpressionPtr condition,
        std::vector<std::pair<ExpressionPtr, Block>> cases,
        Block defaultBlock);
};

}  // namespace parser
MANG_END

#endif  // NODES_H