#ifndef MANGANESE_INCLUDE_FRONTEND_AST_AST_STATEMENTS_H
#define MANGANESE_INCLUDE_FRONTEND_AST_AST_STATEMENTS_H

#include <utility>

#include <frontend/ast/ast_base.h>
#include <frontend/ast/ast_expressions.h>
#include <frontend/ast/ast_types.h>


namespace Manganese {

namespace ast {

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
    std::vector<std::string> genericTypes;
    std::vector<BundleField> fields;

   public:
    BundleDeclarationStatement(std::string name_, std::vector<std::string> genericTypes_ ,std::vector<BundleField> fields_)
        : name(std::move(name_)), genericTypes(std::move(genericTypes_)), fields(std::move(fields_)) {}
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

} // namespace ast

} // namespace Manganese


#endif // MANGANESE_INCLUDE_FRONTEND_AST_AST_STATEMENTS_H