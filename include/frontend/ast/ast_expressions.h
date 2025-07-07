#ifndef MANGANESE_INCLUDE_FRONTEND_AST_AST_EXPRESSIONS_H
#define MANGANESE_INCLUDE_FRONTEND_AST_AST_EXPRESSIONS_H

#include <frontend/ast/ast_base.h>
#include <frontend/ast/ast_types.h>

namespace Manganese {

namespace ast {

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

}  // namespace ast

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_AST_EXPRESSIONS_H