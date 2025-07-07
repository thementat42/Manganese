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
#include <utility>
#include <vector>

#include "token.h"

#if DEBUG
#define __DUMP_OVERRIDE void dump(std::ostream& os, int indent = 0) const override;  // Makes overriding dump() less cumbersome to type
#else
#define __DUMP_OVERRIDE  // Don't dump in non-debug builds
#endif

#define __STRING_OVERRIDE std::string toString() const override;  // Makes overriding toString() less cumbersome to type

#define __NODE_OVERRIDES \
    __STRING_OVERRIDE    \
    __DUMP_OVERRIDE

#define NODE_OVERRIDES __NODE_OVERRIDES  // Makes overriding toString() and dump() less cumbersome to type

namespace Manganese {

namespace parser {
class Parser;
}

// TODO: Add line and column setting methods to the derived classes

namespace ast {

class Expression;
class Statement;
class Type;
using ExpressionPtr = std::unique_ptr<Expression>;
using StatementPtr = std::unique_ptr<Statement>;
using TypePtr = std::unique_ptr<Type>;
using Block = std::vector<StatementPtr>;

enum class Visibility : char {
    Public = 0,
    ReadOnly = 1,
    Private = 2,
};

//~ Base Nodes

class ASTNode {
   protected:
    size_t line = 0, column = 0;

   public:
    virtual ~ASTNode() noexcept = default;

    virtual std::string toString() const = 0;

#if DEBUG
    /**
     * @brief Dump the AST node to an output stream
     * @param os The output stream to dump to
     * @param indent The indentation level for pretty-printing
     */
    virtual void dump(std::ostream& os, int indent = 0) const = 0;
#endif  // DEBUG

    size_t getLine() const { return line; }
    size_t getColumn() const { return column; }

    friend parser::Parser;
};

class Type : public ASTNode {
   public:
    virtual ~Type() noexcept = default;
};

class Expression : public ASTNode {
   public:
    virtual ~Expression() noexcept = default;
    virtual TypePtr getType() const = 0;
};

class Statement : public ASTNode {
   public:
    virtual ~Statement() noexcept = default;
};

//~ Types
class SymbolType : public Type {  // e.g. T, int, etc.
   protected:
    std::string name;

   public:
    /**
     * @param name_ The name of the type
     */
    explicit SymbolType(std::string name_) : name(std::move(name_)) {}

    const std::string& getName() const { return name; }
    NODE_OVERRIDES;
};

class ArrayType : public Type {  // e.g. int[]
   protected:
    std::unique_ptr<Type> elementType;
    ExpressionPtr lengthExpression;  // Optional length specification (otherwise, should be inferred based on the number of elements)

   public:
    /**
     * @param elementType_ The type of the elements in the array
     */
    explicit ArrayType(std::unique_ptr<Type> elementType_, ExpressionPtr lengthExpr_ = nullptr)
        : elementType(std::move(elementType_)), lengthExpression(std::move(lengthExpr_)) {}
    const Expression* getLengthExpression() const { return lengthExpression.get(); }
    bool hasFixedLength() const {
        return lengthExpression != nullptr;
    }
    NODE_OVERRIDES;
};

//~ Expressions
class TypeCastExpression : public Expression {
   protected:
    ExpressionPtr expression;
    TypePtr type;

   public:
    /**
     * @param expression_ The expression to cast
     * @param type_ The type to cast to
     */
    TypeCastExpression(ExpressionPtr expression_, TypePtr type_)
        : expression(std::move(expression_)), type(std::move(type_)) {}
    TypePtr getType() const override;
    NODE_OVERRIDES;
};

//* Literal Expressions

/**
 * @brief Represents a numeric literal in the AST
 */
class NumberLiteralExpression : public Expression {
   protected:
    number_t value;

   public:
    /**
     * @param value_ The numeric value of the expression (can be any numeric type)
     */
    explicit NumberLiteralExpression(number_t value_) : value(value_) {};
    const number_t& getValue() const { return value; }
    NODE_OVERRIDES;

    TypePtr getType() const override;
};

class CharLiteralExpression : public Expression {
   protected:
    char32_t value;

   public:
    /**
     * @param value_ The character value of the expression (char32_t)
     */
    explicit CharLiteralExpression(char32_t value_) : value(value_) {};
    explicit CharLiteralExpression(char value_) : value(static_cast<char32_t>(value_)) {};

    char32_t getValue() const { return value; }
    NODE_OVERRIDES;
    TypePtr getType() const override {
        return TypePtr(new SymbolType("char"));
    }
};

/**
 * @brief Represents a string literal in the AST
 */
class StringLiteralExpression : public Expression {
   protected:
    std::string value;

   public:
    /**
     * @param value_ The string value of the expression (str)
     */
    explicit StringLiteralExpression(const std::string& value_) : value(std::move(value_)) {};

    /**
     * @brief Initialize a StringExpression node
     * @param value_ The string value of the expression (const char*)
     */
    explicit StringLiteralExpression(const char* value_) : value(value_) {};

    const std::string& getValue() const { return value; }
    NODE_OVERRIDES;
    TypePtr getType() const override {
        return TypePtr(new SymbolType("string"));
    }
};

/**
 * @brief Represents a symbol (identifier) in the AST
 */
class IdentifierExpression : public Expression {
   protected:
    std::string value;

   public:
    /**
     * @param value_ The symbol value of the expression (str)
     */
    explicit IdentifierExpression(const std::string& value_) : value(std::move(value_)) {}

    const std::string& getValue() const { return value; }
    NODE_OVERRIDES;
    TypePtr getType() const override {
        return TypePtr(new SymbolType("auto"));
    }
};

class BoolLiteralExpression : public Expression {
   protected:
    bool value;

   public:
    explicit BoolLiteralExpression(const bool value_) : value(value_) {};
    bool getValue() const { return value; };
    NODE_OVERRIDES;
    TypePtr getType() const override {
        return TypePtr(new SymbolType("bool"));
    }
};

class ArrayLiteralExpression : public Expression {
   protected:
    std::vector<ExpressionPtr> elements;
    TypePtr elementType;  // Optional, can be inferred from the elements

   public:
    /**
     * @param elements_ The elements of the array
     * @param elementType_ The type of the elements (optional)
     */
    ArrayLiteralExpression(std::vector<ExpressionPtr> elements_, TypePtr elementType_ = nullptr)
        : elements(std::move(elements_)), elementType(std::move(elementType_)) {}
    const std::vector<ExpressionPtr>& getElements() const { return elements; }
    TypePtr getType() const override {
        return elementType ? TypePtr(elementType.get()) : TypePtr(new SymbolType("auto"));
    }
    NODE_OVERRIDES;
};
//* Complex Expressions

class BinaryExpression : public Expression {
   protected:
    ExpressionPtr left, right;
    lexer::TokenType op;

   public:
    /**
     * @param left_ The left operand of the expression
     * @param op_ The operator token type (e.g., +, -, *, /)
     * @param right_ The right operand of the expression
     */
    BinaryExpression(ExpressionPtr left_, lexer::TokenType op_, ExpressionPtr right_)
        : left(std::move(left_)), right(std::move(right_)), op(op_) {};

    const Expression& getLeft() const { return *left; }
    const Expression& getRight() const { return *right; }
    lexer::TokenType getOperator() const { return op; }
    NODE_OVERRIDES;
    TypePtr getType() const override {
        //? Resolution is in the semantic analysis phase
        return TypePtr(new SymbolType("auto"));
    }
};

class PrefixExpression : public Expression {
   protected:
    lexer::TokenType op;
    ExpressionPtr right;

   public:
    /**
     * @param op_ The operator token type (e.g., ++, --, !)
     * @param right_ The operand of the expression
     */
    PrefixExpression(lexer::TokenType op_, ExpressionPtr right_)
        : op(op_), right(std::move(right_)) {}

    const Expression& getRight() const { return *right; }
    lexer::TokenType getOperator() const { return op; }

    NODE_OVERRIDES;
    TypePtr getType() const override {
        // Don't infer the type, leave that to the semantic analysis phase
        return TypePtr(new SymbolType("auto"));
    }
};

class PostfixExpression : public Expression {
   protected:
    ExpressionPtr left;
    lexer::TokenType op;

   public:
    /**
     * @param left_ The operand of the expression
     * @param op_ The operator token type (e.g., ++, --)
     */
    PostfixExpression(ExpressionPtr left_, lexer::TokenType op_)
        : left(std::move(left_)), op(op_) {}

    const Expression& getLeft() const { return *left; }
    lexer::TokenType getOperator() const { return op; }

    NODE_OVERRIDES;
    TypePtr getType() const override {
        return TypePtr(new SymbolType("auto"));
    }
};

class AssignmentExpression : public Expression {
   protected:
    ExpressionPtr assignee, value;
    lexer::TokenType op;

   public:
    /**
     * @param assignee_ The expression being assigned to (left-hand side)
     * @param op_ The assignment operator token type (e.g., =, +=, -=)
     * @param value_ The value being assigned (right-hand side)
     */
    AssignmentExpression(ExpressionPtr assignee_, lexer::TokenType op_, ExpressionPtr value_)
        : assignee(std::move(assignee_)), value(std::move(value_)), op(op_) {}

    const Expression& getAssignee() const { return *assignee; }
    const Expression& getValue() const { return *value; }
    lexer::TokenType getOperator() const { return op; }
    NODE_OVERRIDES;
    TypePtr getType() const override {
        return assignee->getType();  // Assume the assignee's type is the result type
    }
};

class FunctionCallExpression : public Expression {
   protected:
    ExpressionPtr callee;
    std::vector<ExpressionPtr> arguments;

   public:
    /**
     * @param callee_ The function being called
     * @param arguments_ The arguments passed to the function
     */
    FunctionCallExpression(ExpressionPtr callee_, std::vector<ExpressionPtr> arguments_)
        : callee(std::move(callee_)), arguments(std::move(arguments_)) {}
    NODE_OVERRIDES;
    TypePtr getType() const override {
        return TypePtr(new SymbolType("auto"));
    }
};

class GenericExpression : public Expression {
    protected:
    ExpressionPtr identifier;  // The thing to which the generic types are passed
    std::vector<TypePtr> types;  // The types passed to the generic expression
    public:
     GenericExpression(ExpressionPtr identifier_, std::vector<TypePtr> types_)
         : identifier(std::move(identifier_)), types(std::move(types_)) {}
     TypePtr getType() const override {
         // The type of a generic expression is the type of the identifier expression
         return identifier->getType();
     }
    NODE_OVERRIDES;
};

struct BundleInstantiationField {
    std::string name;
    ExpressionPtr value;

    BundleInstantiationField(std::string name_, ExpressionPtr value_)
        : name(std::move(name_)), value(std::move(value_)) {}
};

class BundleInstantiationExpression : public Expression {
   protected:
    std::string name;
    std::vector<BundleInstantiationField> fields;

   public:
    /**
     * @param name_ The name of the bundle being instantiated
     * @param fields_ The fields to initialize in the bundle
     */
    BundleInstantiationExpression(std::string name_, std::vector<BundleInstantiationField> fields_)
        : name(std::move(name_)), fields(std::move(fields_)) {}

    const std::string& getName() const { return name; }
    const std::vector<BundleInstantiationField>& getFields() const { return fields; }

    NODE_OVERRIDES;
    TypePtr getType() const override {
        return TypePtr(new SymbolType(name));
    }
};

class IndexExpression : public Expression {
   protected:
    ExpressionPtr variable;
    ExpressionPtr index;

   public:
    IndexExpression(ExpressionPtr variable_, ExpressionPtr index_)
        : variable(std::move(variable_)), index(std::move(index_)) {}
    TypePtr getType() const override {
        return TypePtr(new SymbolType("auto"));
    }
    NODE_OVERRIDES;
};

class ScopeResolutionExpression : public Expression {
   protected:
    ExpressionPtr scope;
    std::string element;

   public:
    ScopeResolutionExpression(ExpressionPtr scope_, std::string element_)
        : scope(std::move(scope_)), element(std::move(element_)) {}
    TypePtr getType() const override {
        return TypePtr(new SymbolType("auto"));
    }
    NODE_OVERRIDES;
};

class MemberAccessExpression : public Expression {
   protected:
    ExpressionPtr object;
    std::string property;

   public:
    MemberAccessExpression(ExpressionPtr object_, std::string property_)
        : object(std::move(object_)), property(std::move(property_)) {}
    TypePtr getType() const override {
        return TypePtr(new SymbolType("auto"));
    }
    NODE_OVERRIDES;
};

//~ Statements

/**
 * @brief Wrapper class to convert an expression into a statement
 */
class ExpressionStatement : public Statement {
   protected:
    ExpressionPtr expression;

   public:
    explicit ExpressionStatement(ExpressionPtr expression_) : expression(std::move(expression_)) {};

    const Expression& getExpression() const { return *expression; }
    NODE_OVERRIDES;
};

class VariableDeclarationStatement : public Statement {
   protected:
    bool isConst;
    std::string name;
    Visibility visibility;
    ExpressionPtr value;
    TypePtr type;

   public:
    /**
     * @param isConst_ Whether the variable is constant
     * @param name_ The name of the variable
     * @param visibility_ The visibility of the variable (public, read-only, private)
     * @param value_ The initial value of the variable
     */
    VariableDeclarationStatement(bool isConst_, std::string name_, Visibility visibility_,
                                 ExpressionPtr value_, TypePtr type_ = nullptr);

    bool isConstant() const { return isConst; }
    const std::string& getName() const { return name; }
    Visibility getVisibility() const { return visibility; }
    const Expression& getValue() const { return *value; }

    NODE_OVERRIDES;
};

struct FunctionParameter {
    std::string name;
    TypePtr type;
    bool isConst;

    FunctionParameter(std::string name_, TypePtr type_, bool isConst_) : name(std::move(name_)), type(std::move(type_)), isConst(isConst_) {}
};

class FunctionDeclarationStatement : public Statement {
   protected:
    std::string name;
    std::vector<std::string> genericTypes;
    std::vector<FunctionParameter> parameters;
    TypePtr returnType;
    Block body;

   public:
    FunctionDeclarationStatement(std::string name_, std::vector<std::string> genericTypes_, std::vector<FunctionParameter> parameters_, TypePtr returnType_, Block body_)
        : name(std::move(name_)), genericTypes(std::move(genericTypes_)), parameters(std::move(parameters_)), returnType(std::move(returnType_)), body(std::move(body_)) {}

    NODE_OVERRIDES;
};

class ReturnStatement : public Statement {
   protected:
    ExpressionPtr value;

   public:
    explicit ReturnStatement(ExpressionPtr value_ = nullptr) : value(std::move(value_)) {}

    const Expression* getValue() const { return value.get(); }
    NODE_OVERRIDES;
};

struct ElifClause {
    ExpressionPtr condition;
    Block body;

    ElifClause(ExpressionPtr condition_, Block body_) : condition(std::move(condition_)), body(std::move(body_)) {}
};

class IfStatement : public Statement {
   protected:
    ExpressionPtr condition;
    Block body, elseBody;  // elseBody might be empty
    std::vector<ElifClause> elifs;

   public:
    IfStatement(ExpressionPtr condition_, Block body_, std::vector<ElifClause> elifs_, Block elseBody_ = {})
        : condition(std::move(condition_)), body(std::move(body_)), elseBody(std::move(elseBody_)), elifs(std::move(elifs_)) {}

    NODE_OVERRIDES;
};

struct BundleField {
    std::string name;
    TypePtr type;
    bool isStatic;

    BundleField(std::string name_, TypePtr type_, bool isStatic_)
        : name(std::move(name_)), type(std::move(type_)), isStatic(isStatic_) {}
};

class BundleDeclarationStatement : public Statement {
   protected:
    std::string name;
    std::vector<BundleField> fields;

   public:
    BundleDeclarationStatement(std::string name_, std::vector<BundleField> fields_)
        : name(std::move(name_)), fields(std::move(fields_)) {}
    NODE_OVERRIDES;
};

class WhileLoopStatement : public Statement {
   protected:
    Block body;
    ExpressionPtr condition;
    bool isDoWhile;

   public:
    WhileLoopStatement(Block body_, ExpressionPtr condition_, bool isDoWhile_ = false)
        : body(std::move(body_)), condition(std::move(condition_)), isDoWhile(isDoWhile_) {}

    NODE_OVERRIDES;
};

class RepeatLoopStatement : public Statement {
   protected:
    ExpressionPtr numIterations;
    Block body;

   public:
    RepeatLoopStatement(ExpressionPtr numIterations_, Block body_)
        : numIterations(std::move(numIterations_)), body(std::move(body_)) {}

    NODE_OVERRIDES;
};

struct EnumValue {
    std::string name;
    ExpressionPtr value;
    explicit EnumValue(std::string name_, ExpressionPtr value_ = nullptr)
        : name(std::move(name_)), value(std::move(value_)) {}
};

class EnumDeclarationStatement : public Statement {
   protected:
    std::string name;
    TypePtr baseType;
    std::vector<EnumValue> values;

   public:
    EnumDeclarationStatement(std::string name_, TypePtr baseType_, std::vector<EnumValue> values_)
        : name(name_), baseType(std::move(baseType_)), values(std::move(values_)) {}

    NODE_OVERRIDES;
};

struct CaseClause {
    ExpressionPtr literalValue;
    Block body;

    CaseClause(ExpressionPtr literalValue_, Block body_) : literalValue(std::move(literalValue_)), body(std::move(body_)) {}
};

class SwitchStatement : public Statement {
   protected:
    ExpressionPtr variable;
    std::vector<CaseClause> cases;
    Block defaultBody;

   public:
    SwitchStatement(ExpressionPtr variable_, std::vector<CaseClause> cases_, Block defaultBody_ = {})
        : variable(std::move(variable_)), cases(std::move(cases_)), defaultBody(std::move(defaultBody_)) {}

    NODE_OVERRIDES;
};

}  // namespace ast
}  // namespace Manganese

#endif  // AST_H
