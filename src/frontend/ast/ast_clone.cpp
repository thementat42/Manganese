#include <frontend/ast.hpp>
#include <frontend/semantic/clone_context.hpp>
#include <frontend/semantic/type_context.hpp>

namespace Manganese::ast {

// Helpers

namespace {

template <class T, class... Args>
    requires(std::is_convertible_v<T*, ast::ASTNode*> && std::is_constructible_v<T, Args...>)
T* makeClonedNode(T* t, semantic::CloneContext* context, Args&&... args) {
    T* const node = context->arena.emplace<T>(std::forward<Args>(args)...);
    node->line = t->line;
    node->column = t->column;
    return node;
}

}  // namespace

// Statements
AggregateDeclarationStatement* AggregateDeclarationStatement::clone(
    [[maybe_unused]] semantic::CloneContext* context) const {
    return nullptr;
}

AliasStatement* AliasStatement::clone([[maybe_unused]] semantic::CloneContext* context) const { return nullptr; }

BreakStatement* BreakStatement::clone([[maybe_unused]] semantic::CloneContext* context) const { return nullptr; }

ContinueStatement* ContinueStatement::clone([[maybe_unused]] semantic::CloneContext* context) const { return nullptr; }

EmptyStatement* EmptyStatement::clone([[maybe_unused]] semantic::CloneContext* context) const { return nullptr; }

EnumDeclarationStatement* EnumDeclarationStatement::clone([[maybe_unused]] semantic::CloneContext* context) const {
    return nullptr;
}

ExpressionStatement* ExpressionStatement::clone([[maybe_unused]] semantic::CloneContext* context) const {
    return nullptr;
}

ForLoopStatement* ForLoopStatement::clone([[maybe_unused]] semantic::CloneContext* context) const { return nullptr; }

FunctionDeclarationStatement* FunctionDeclarationStatement::clone(
    [[maybe_unused]] semantic::CloneContext* context) const {
    return nullptr;
}

IfStatement* IfStatement::clone([[maybe_unused]] semantic::CloneContext* context) const { return nullptr; }

ImportStatement* ImportStatement::clone([[maybe_unused]] semantic::CloneContext* context) const { return nullptr; }

ModuleDeclarationStatement* ModuleDeclarationStatement::clone([[maybe_unused]] semantic::CloneContext* context) const {
    return nullptr;
}

NamespaceStatement* NamespaceStatement::clone([[maybe_unused]] semantic::CloneContext* context) const {
    return nullptr;
}

NestedBlockStatement* NestedBlockStatement::clone([[maybe_unused]] semantic::CloneContext* context) const {
    return nullptr;
}

ReturnStatement* ReturnStatement::clone([[maybe_unused]] semantic::CloneContext* context) const { return nullptr; }

SwitchStatement* SwitchStatement::clone([[maybe_unused]] semantic::CloneContext* context) const { return nullptr; }

VariableDeclarationStatement* VariableDeclarationStatement::clone(
    [[maybe_unused]] semantic::CloneContext* context) const {
    return nullptr;
}

WhileLoopStatement* WhileLoopStatement::clone([[maybe_unused]] semantic::CloneContext* context) const {
    return nullptr;
}

AggregateInstantiationExpression* AggregateInstantiationExpression::clone(
    [[maybe_unused]] semantic::CloneContext* context) const {
    return nullptr;
}

AggregateLiteralExpression* AggregateLiteralExpression::clone([[maybe_unused]] semantic::CloneContext* context) const {
    return nullptr;
}

AlignofExpression* AlignofExpression::clone([[maybe_unused]] semantic::CloneContext* context) const { return nullptr; }

ArrayLiteralExpression* ArrayLiteralExpression::clone([[maybe_unused]] semantic::CloneContext* context) const {
    return nullptr;
}

AssignmentExpression* AssignmentExpression::clone([[maybe_unused]] semantic::CloneContext* context) const {
    return nullptr;
}

BinaryExpression* BinaryExpression::clone([[maybe_unused]] semantic::CloneContext* context) const { return nullptr; }

BoolLiteralExpression* BoolLiteralExpression::clone([[maybe_unused]] semantic::CloneContext* context) const {
    return nullptr;
}

CharLiteralExpression* CharLiteralExpression::clone([[maybe_unused]] semantic::CloneContext* context) const {
    return nullptr;
}

FunctionCallExpression* FunctionCallExpression::clone([[maybe_unused]] semantic::CloneContext* context) const {
    return nullptr;
}

GenericInstantiationExpression* GenericInstantiationExpression::clone(
    [[maybe_unused]] semantic::CloneContext* context) const {
    return nullptr;
}

IdentifierExpression* IdentifierExpression::clone([[maybe_unused]] semantic::CloneContext* context) const {
    return nullptr;
}

IndexExpression* IndexExpression::clone([[maybe_unused]] semantic::CloneContext* context) const { return nullptr; }

MemberAccessExpression* MemberAccessExpression::clone([[maybe_unused]] semantic::CloneContext* context) const {
    return nullptr;
}

NumberLiteralExpression* NumberLiteralExpression::clone([[maybe_unused]] semantic::CloneContext* context) const {
    return nullptr;
}

PostfixExpression* PostfixExpression::clone([[maybe_unused]] semantic::CloneContext* context) const { return nullptr; }

PrefixExpression* PrefixExpression::clone([[maybe_unused]] semantic::CloneContext* context) const { return nullptr; }

ScopeResolutionExpression* ScopeResolutionExpression::clone([[maybe_unused]] semantic::CloneContext* context) const {
    return nullptr;
}

SizeofExpression* SizeofExpression::clone([[maybe_unused]] semantic::CloneContext* context) const { return nullptr; }

StringLiteralExpression* StringLiteralExpression::clone([[maybe_unused]] semantic::CloneContext* context) const {
    return nullptr;
}

TypeCastExpression* TypeCastExpression::clone([[maybe_unused]] semantic::CloneContext* context) const {
    return nullptr;
}

UninitializedExpression* UninitializedExpression::clone([[maybe_unused]] semantic::CloneContext* context) const {
    return nullptr;
}

// Types

AggregateType* AggregateType::clone([[maybe_unused]] semantic::CloneContext* context) const { return nullptr; }

ArrayType* ArrayType::clone([[maybe_unused]] semantic::CloneContext* context) const { return nullptr; }

FunctionType* FunctionType::clone([[maybe_unused]] semantic::CloneContext* context) const { return nullptr; }

GenericInstantiationType* GenericInstantiationType::clone([[maybe_unused]] semantic::CloneContext* context) const {
    return nullptr;
}

PointerType* PointerType::clone([[maybe_unused]] semantic::CloneContext* context) const { return nullptr; }

ScopedType* ScopedType::clone([[maybe_unused]] semantic::CloneContext* context) const { return nullptr; }

IdentifierType* IdentifierType::clone([[maybe_unused]] semantic::CloneContext* context) const { return nullptr; }

TypeofType* TypeofType::clone([[maybe_unused]] semantic::CloneContext* context) const { return nullptr; }

// Poison

PoisonedExpression* PoisonedExpression::clone([[maybe_unused]] semantic::CloneContext* context) const {
    return nullptr;
}

PoisonedStatement* PoisonedStatement::clone([[maybe_unused]] semantic::CloneContext* context) const { return nullptr; }

PoisonedType* PoisonedType::clone([[maybe_unused]] semantic::CloneContext* context) const { return nullptr; }

}  // namespace Manganese::ast