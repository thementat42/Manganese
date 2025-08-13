#ifndef MANGANESE_INCLUDE_MIDDLEEND_CODEGEN_CODEGEN_BASE_HPP
#define MANGANESE_INCLUDE_MIDDLEEND_CODEGEN_CODEGEN_BASE_HPP

#include <llvm/IR/Value.h>

#include <frontend/ast.hpp>


namespace Manganese {
namespace codegen {

llvm::Value* generateExpression(const ast::Expression* expression);
llvm::Value* generateStatement(const ast::Statement* statement);
llvm::Value* generateType(const ast::Type* type);

// ===== Specific Expression Code Generation =====
llvm::Value* generateAggregateInstantiationExpression(const ast::AggregateInstantiationExpression* expression);
llvm::Value* generateArrayLiteralExpression(const ast::ArrayLiteralExpression* expression);
llvm::Value* generateAssignmentExpression(const ast::AssignmentExpression* expression);
llvm::Value* generateBinaryExpression(const ast::BinaryExpression* expression);
llvm::Value* generateBoolLiteralExpression(const ast::BoolLiteralExpression* expression);
llvm::Value* generateCharLiteralExpression(const ast::CharLiteralExpression* expression);
llvm::Value* generateFunctionCallExpression(const ast::FunctionCallExpression* expression);
llvm::Value* generateGenericExpression(const ast::GenericExpression* expression);
llvm::Value* generateIdentifierExpression(const ast::IdentifierExpression* expression);
llvm::Value* generateIndexExpression(const ast::IndexExpression* expression);
llvm::Value* generateMemberAccessExpression(const ast::MemberAccessExpression* expression);
llvm::Value* generateNumberLiteralExpression(const ast::NumberLiteralExpression* expression);
llvm::Value* generatePostfixExpression(const ast::PostfixExpression* expression);
llvm::Value* generatePrefixExpression(const ast::PrefixExpression* expression);
llvm::Value* generateScopeResolutionExpression(const ast::ScopeResolutionExpression* expression);
llvm::Value* generateStringLiteralExpression(const ast::StringLiteralExpression* expression);
llvm::Value* generateTypeCastExpression(const ast::TypeCastExpression* expression);

// ===== Specific Statement Code Generation =====
llvm::Value* generateAggregateDeclarationStatement(const ast::AggregateDeclarationStatement* statement);
llvm::Value* generateAliasStatement(const ast::AliasStatement* statement);
llvm::Value* generateBreakStatement(const ast::BreakStatement* statement);
llvm::Value* generateContinueStatement(const ast::ContinueStatement* statement);
llvm::Value* generateEmptyStatement(const ast::EmptyStatement* statement);
llvm::Value* generateEnumDeclarationStatement(const ast::EnumDeclarationStatement* statement);
llvm::Value* generateExpressionStatement(const ast::ExpressionStatement* statement);
llvm::Value* generateFunctionDeclarationStatement(const ast::FunctionDeclarationStatement* statement);
llvm::Value* generateIfStatement(const ast::IfStatement* statement);
llvm::Value* generateRepeatLoopStatement(const ast::RepeatLoopStatement* statement);
llvm::Value* generateReturnStatement(const ast::ReturnStatement* statement);
llvm::Value* generateSwitchStatement(const ast::SwitchStatement* statement);
llvm::Value* generateVariableDeclarationStatement(const ast::VariableDeclarationStatement* statement);
llvm::Value* generateWhileLoopStatement(const ast::WhileLoopStatement* statement);

// ===== Specific Type Code Generation =====
llvm::Value* generateAggregateType(const ast::AggregateType* type);
llvm::Value* generateArrayType(const ast::ArrayType* type);
llvm::Value* generateFunctionType(const ast::FunctionType* type);
llvm::Value* generateGenericType(const ast::GenericType* type);
llvm::Value* generatePointerType(const ast::PointerType* type);
llvm::Value* generateSymbolType(const ast::SymbolType* type);

}  // namespace codegen
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_MIDDLEEND_CODEGEN_CODEGEN_BASE_H