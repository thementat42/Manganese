#include <frontend/ast.hpp>
#include <frontend/semantic/clone_context.hpp>
#include <frontend/semantic/type_context.hpp>
#include <vector>
#include <utils/string_interner.hpp>
#include <optional>

#include "core.hpp"

namespace Manganese::ast {

// Helpers

namespace {

template <class T, class... Args>
    requires(std::is_convertible_v<T*, ast::ASTNode*> && std::is_constructible_v<T, Args...>)
T* makeClonedNode(const T* t, semantic::CloneContext* context, Args&&... args) {
    T* const node = context->arena.emplace<T>(std::forward<Args>(args)...);
    node->line = t->line;
    node->column = t->column;
    return node;
}

// Type alias for temporary statement collection during cloning
using StatementVector = std::vector<ast::Statement*>;

ast::Block cloneBlock(const ast::Block& block, semantic::CloneContext* context) {
    StatementVector result;
    result.reserve(block.size);
    for (const auto* statement : block) { result.push_back(statement->clone(context)); }
    return context->arena.emplace_range(result);
}

}  // namespace

// Statements
AggregateDeclarationStatement* AggregateDeclarationStatement::clone(semantic::CloneContext* context) const {
    std::vector<AggregateField> fieldClones;
    fieldClones.reserve(fields.size);
    for (const auto& field : fields) {
        fieldClones.push_back({.name = field.name,
                               .type = field.type->clone(context),
                               .line = field.line,
                               .column = field.column,
                               .isMutable = field.isMutable});
    }
    auto* cloned = makeClonedNode(this, context, name, mnstl::Slice<utils::StringID>{}, context->arena.emplace_range(fieldClones));
    cloned->visibility = visibility;
    return cloned;
}

AliasStatement* AliasStatement::clone(semantic::CloneContext* context) const {
    Type* baseClone = baseType->clone(context);
    auto* clone = makeClonedNode(this, context, baseClone, name);
    clone->visibility = visibility;
    return clone;
}

BreakStatement* BreakStatement::clone(semantic::CloneContext* context) const { return makeClonedNode(this, context); }

ContinueStatement* ContinueStatement::clone(semantic::CloneContext* context) const {
    return makeClonedNode(this, context);
}

EmptyStatement* EmptyStatement::clone(semantic::CloneContext* context) const { return makeClonedNode(this, context); }

EnumDeclarationStatement* EnumDeclarationStatement::clone(semantic::CloneContext* context) const {
    Type* baseTypeClone = baseType->clone(context);
    std::vector<EnumValue> valueClones;
    valueClones.reserve(values.size);

    for (const auto& value : values) {
        valueClones.push_back(
            {.name = value.name, .value = value.value->clone(context), .line = value.line, .column = value.column});
    }
    auto* clone = makeClonedNode(this, context, name, baseTypeClone, context->arena.emplace_range(valueClones));
    clone->visibility = visibility;
    return clone;
}

ExpressionStatement* ExpressionStatement::clone(semantic::CloneContext* context) const {
    return makeClonedNode(this, context, expression->clone(context));
}

ForLoopStatement* ForLoopStatement::clone(semantic::CloneContext* context) const {
    Statement* initializationClone = nullptr;
    Expression* stopClone = nullptr;
    Expression* postClone = nullptr;

    if (initializationStep != nullptr) { initializationClone = initializationStep->clone(context); }
    if (stopCondition != nullptr) { stopClone = stopCondition->clone(context); }
    if (postExpression != nullptr) { postClone = postExpression->clone(context); }

    return makeClonedNode(this, context, initializationClone, stopClone, postClone, cloneBlock(body, context));
}

FunctionDeclarationStatement* FunctionDeclarationStatement::clone(semantic::CloneContext* context) const {
    std::vector<FunctionParameter> parameterClones;
    parameterClones.reserve(parameters.size);
    for (const auto& param : parameters) {
        parameterClones.push_back(
            {.name = param.name,
             .type = param.type->clone(context),
             .defaultValue = (param.defaultValue == nullptr) ? nullptr : param.defaultValue->clone(context),
             .line = param.line,
             .column = param.column,
             .isMutable = param.isMutable,
             .isVariadic = param.isVariadic});
    }
    Type* returnTypeClone = (returnType == nullptr) ? nullptr : returnType->clone(context);

    auto* clone = makeClonedNode(this, context, name, mnstl::Slice<utils::StringID>{},
                                 context->arena.emplace_range(parameterClones), returnTypeClone, cloneBlock(body, context));
    clone->visibility = visibility;
    return clone;
}

IfStatement* IfStatement::clone(semantic::CloneContext* context) const {
    Expression* conditionClone = condition->clone(context);
    std::vector<ElifClause> elifClones;
    elifClones.reserve(elifs.size);

    for (const auto& elif : elifs) {
        elifClones.push_back({.condition = elif.condition->clone(context), .body = cloneBlock(elif.body, context)});
    }

    std::optional<ast::Block> elseBodyClone = std::nullopt;
    if (elseBody.has_value()) {
        elseBodyClone = cloneBlock(*elseBody, context);
    }

    return makeClonedNode(this, context, conditionClone, cloneBlock(body, context), context->arena.emplace_range(elifClones),
                          elseBodyClone);
}

ImportStatement* ImportStatement::clone(semantic::CloneContext* /*context*/) const {
    ASSERT_UNREACHABLE(
        "Import statements should have been enforced as being at the top of a file, not inside a generic function");
}

ModuleDeclarationStatement* ModuleDeclarationStatement::clone(semantic::CloneContext* /*context*/) const {
    ASSERT_UNREACHABLE(
        "Module declaration statements should have been enforced as being at the top of a file, not inside a generic function");
}

NamespaceStatement* NamespaceStatement::clone(semantic::CloneContext* context) const {
    return makeClonedNode(this, context, name, cloneBlock(block, context));
}

NestedBlockStatement* NestedBlockStatement::clone(semantic::CloneContext* context) const {
    return makeClonedNode(this, context, cloneBlock(block, context));
}

ReturnStatement* ReturnStatement::clone(semantic::CloneContext* context) const {
    return makeClonedNode(this, context, value == nullptr ? nullptr : value->clone(context));
}

SwitchStatement* SwitchStatement::clone(semantic::CloneContext* context) const {
    Expression* targetClone = target->clone(context);
    std::vector<CaseClause> caseClones;
    caseClones.reserve(cases.size);

    for (const auto& caseClause : cases) {
        std::vector<Expression*> clonedCaseValues;
        clonedCaseValues.reserve(caseClause.values.size);

        for (const auto* value : caseClause.values) { clonedCaseValues.push_back(value->clone(context)); }
        caseClones.push_back({.values = context->arena.emplace_range(clonedCaseValues), .body = cloneBlock(caseClause.body, context)});
    }

    std::optional<ast::Block> defaultBodyClone = std::nullopt;
    if (defaultBody.has_value()) {
        defaultBodyClone = cloneBlock(*defaultBody, context);
    }

    return makeClonedNode(this, context, targetClone, context->arena.emplace_range(caseClones), defaultBodyClone);
}

VariableDeclarationStatement* VariableDeclarationStatement::clone(semantic::CloneContext* context) const {
    Type* typeClone = type == nullptr ? nullptr : type->clone(context);
    Expression* valueClone = value == nullptr ? nullptr : value->clone(context);

    auto* clone = makeClonedNode(this, context, isMutable, name, visibility, valueClone, typeClone);
    return clone;
}

WhileLoopStatement* WhileLoopStatement::clone(semantic::CloneContext* context) const {
    return makeClonedNode(this, context, cloneBlock(body, context), condition->clone(context), isDoWhile);
}

// Expressions

AggregateInstantiationExpression* AggregateInstantiationExpression::clone(semantic::CloneContext* context) const {
    Expression* baseClone = base->clone(context);

    std::vector<AggregateInstantiationField> fieldClones;
    fieldClones.reserve(fields.size);

    for (const auto& field : fields) {
        fieldClones.push_back(
            {.name = field.name, .value = field.value->clone(context), .line = field.line, .column = field.column});
    }

    return makeClonedNode(this, context, baseClone, context->arena.emplace_range(fieldClones));
}

AggregateLiteralExpression* AggregateLiteralExpression::clone(semantic::CloneContext* context) const {
    std::vector<Expression*> elementClones;
    elementClones.reserve(elements.size);

    for (const auto& element : elements) { elementClones.push_back(element->clone(context)); }

    return makeClonedNode(this, context, context->arena.emplace_range(elementClones));
}

AlignofExpression* AlignofExpression::clone(semantic::CloneContext* context) const {
    return makeClonedNode(this, context, type->clone(context));
}

ArrayLiteralExpression* ArrayLiteralExpression::clone(semantic::CloneContext* context) const {
    std::vector<Expression*> elementClones;
    elementClones.reserve(elements.size);

    for (const auto& element : elements) { elementClones.push_back(element->clone(context)); }

    return makeClonedNode(this, context, context->arena.emplace_range(elementClones));
}

AssignmentExpression* AssignmentExpression::clone(semantic::CloneContext* context) const {
    return makeClonedNode(this, context, assignee->clone(context), op, value->clone(context));
}

BinaryExpression* BinaryExpression::clone(semantic::CloneContext* context) const {
    return makeClonedNode(this, context, left->clone(context), op, right->clone(context));
}

BoolLiteralExpression* BoolLiteralExpression::clone(semantic::CloneContext* context) const {
    return makeClonedNode(this, context, value);
}

CharLiteralExpression* CharLiteralExpression::clone(semantic::CloneContext* context) const {
    return makeClonedNode(this, context, value);
}

FunctionCallExpression* FunctionCallExpression::clone(semantic::CloneContext* context) const {
    Expression* calleeClone = callee->clone(context);

    std::vector<Expression*> argumentClones;
    argumentClones.reserve(arguments.size);

    for (const auto& argument : arguments) { argumentClones.push_back(argument->clone(context)); }

    return makeClonedNode(this, context, calleeClone, context->arena.emplace_range(argumentClones));
}

GenericInstantiationExpression* GenericInstantiationExpression::clone(semantic::CloneContext* context) const {
    Expression* identifierClone = identifier->clone(context);
    std::vector<Type*> typeClones;
    typeClones.reserve(types.size);

    for (Type* t : types) { typeClones.push_back(t->clone(context)); }

    auto* clone = makeClonedNode(this, context, identifierClone, context->arena.emplace_range(typeClones));

    return clone;
}

IdentifierExpression* IdentifierExpression::clone(semantic::CloneContext* context) const {
    auto* clone = makeClonedNode(this, context, name);

    if (resolvedDeclaration != nullptr) {
        if (auto it = context->declarationSubstitutions.find(resolvedDeclaration);
            it != context->declarationSubstitutions.end()) {
            clone->resolvedDeclaration = it->second;
        } else {
            clone->resolvedDeclaration = resolvedDeclaration;
        }
    }
    return clone;
}

IndexExpression* IndexExpression::clone(semantic::CloneContext* context) const {
    return makeClonedNode(this, context, variable->clone(context), index->clone(context));
}

MemberAccessExpression* MemberAccessExpression::clone(semantic::CloneContext* context) const {
    auto* clone = makeClonedNode(this, context, object->clone(context), field);
    clone->fieldIndex = fieldIndex;
    return clone;
}

NumberLiteralExpression* NumberLiteralExpression::clone(semantic::CloneContext* context) const {
    return makeClonedNode(this, context, value, isFloat);
}

PostfixExpression* PostfixExpression::clone(semantic::CloneContext* context) const {
    return makeClonedNode(this, context, left->clone(context), op);
}

PrefixExpression* PrefixExpression::clone(semantic::CloneContext* context) const {
    return makeClonedNode(this, context, op, right->clone(context));
}

ScopeResolutionExpression* ScopeResolutionExpression::clone(semantic::CloneContext* context) const {
    auto* clone = makeClonedNode(this, context, scope->clone(context), element->clone(context));
    return clone;
}

SizeofExpression* SizeofExpression::clone(semantic::CloneContext* context) const {
    return makeClonedNode(this, context, type->clone(context));
}

StringLiteralExpression* StringLiteralExpression::clone(semantic::CloneContext* context) const {
    return makeClonedNode(this, context, value);
}

TypeCastExpression* TypeCastExpression::clone(semantic::CloneContext* context) const {
    return makeClonedNode(this, context, originalValue->clone(context), targetType->clone(context));
}

TernaryExpression* TernaryExpression::clone(semantic::CloneContext* context) const {
    return makeClonedNode(this, context, condition->clone(context), ifTrue->clone(context), ifFalse->clone(context));
}

UninitializedExpression* UninitializedExpression::clone(semantic::CloneContext* context) const {
    return makeClonedNode(this, context);
}

// Types

AggregateType* AggregateType::clone(semantic::CloneContext* context) const {
    std::vector<Type*> fieldTypeClones;
    fieldTypeClones.reserve(fieldTypes.size);

    for (const auto* type : fieldTypes) { fieldTypeClones.push_back(type->clone(context)); }

    return makeClonedNode(this, context, context->arena.emplace_range(fieldTypeClones));
}

ArrayType* ArrayType::clone(semantic::CloneContext* context) const {
    Expression* lengthExpressionClone = lengthExpression == nullptr ? nullptr : lengthExpression->clone(context);

    return makeClonedNode(this, context, elementType->clone(context), lengthExpressionClone);
}

FunctionType* FunctionType::clone(semantic::CloneContext* context) const {
    std::vector<FunctionParameterType> parameterTypeClones;
    parameterTypeClones.reserve(parameterTypes.size);

    for (const auto& param : parameterTypes) {
        parameterTypeClones.push_back(
            {.isMutable = param.isMutable, .isVariadic = param.isVariadic, .type = param.type->clone(context)});
    }

    Type* returnTypeClone = returnType == nullptr ? nullptr : returnType->clone(context);

    return makeClonedNode(this, context, context->arena.emplace_range(parameterTypeClones), returnTypeClone);
}

GenericInstantiationType* GenericInstantiationType::clone(semantic::CloneContext* context) const {
    Type* baseTypeClone = baseType->clone(context);

    std::vector<Type*> typeParameterClones;
    typeParameterClones.reserve(typeParameters.size);

    for (const auto* type : typeParameters) { typeParameterClones.push_back(type->clone(context)); }

    return makeClonedNode(this, context, baseTypeClone, context->arena.emplace_range(typeParameterClones));
}

IdentifierType* IdentifierType::clone(semantic::CloneContext* context) const {
    auto* clone = makeClonedNode(this, context, name);

    if (auto it = context->substitutions.find(name); it != context->substitutions.end()) {
        // This is a generic parameter: make its semantic type the substituted one from the instantiation
        // e.g for foo@[int32], map T in foo[T] to int32
        clone->semanticType = it->second;
    } else {
        // regular identifier (like int32)
        clone->semanticType = semanticType;
    }
    return clone;
}

PointerType* PointerType::clone(semantic::CloneContext* context) const {
    return makeClonedNode(this, context, baseType->clone(context), isMutable);
}

ScopedType* ScopedType::clone(semantic::CloneContext* context) const {
    return makeClonedNode(this, context, scope->clone(context), type->clone(context));
}

TypeofType* TypeofType::clone(semantic::CloneContext* context) const {
    return makeClonedNode(this, context, expression->clone(context));
}

// Poison

PoisonedExpression* PoisonedExpression::clone(semantic::CloneContext* context) const {
    return makeClonedNode(this, context);
}

PoisonedStatement* PoisonedStatement::clone(semantic::CloneContext* context) const {
    return makeClonedNode(this, context);
}

PoisonedType* PoisonedType::clone(semantic::CloneContext* context) const { return makeClonedNode(this, context); }

}  // namespace Manganese::ast