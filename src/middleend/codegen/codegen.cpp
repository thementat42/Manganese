#include <global_macros.hpp>
#include <middleend/codegen/codegen_base.hpp>
#include <string>


namespace Manganese {

namespace codegen {

llvm::Value* generateExpression(const ast::Expression* expression) {
    switch (expression->kind()) {
        case ast::ExpressionKind::AggregateInstantiationExpression:
            return generateAggregateInstantiationExpression(
                static_cast<const ast::AggregateInstantiationExpression*>(expression));
        case ast::ExpressionKind::ArrayLiteralExpression:
            return generateArrayLiteralExpression(static_cast<const ast::ArrayLiteralExpression*>(expression));
        case ast::ExpressionKind::AssignmentExpression:
            return generateAssignmentExpression(static_cast<const ast::AssignmentExpression*>(expression));
        case ast::ExpressionKind::BinaryExpression:
            return generateBinaryExpression(static_cast<const ast::BinaryExpression*>(expression));
        case ast::ExpressionKind::BoolLiteralExpression:
            return generateBoolLiteralExpression(static_cast<const ast::BoolLiteralExpression*>(expression));
        case ast::ExpressionKind::CharLiteralExpression:
            return generateCharLiteralExpression(static_cast<const ast::CharLiteralExpression*>(expression));
        case ast::ExpressionKind::FunctionCallExpression:
            return generateFunctionCallExpression(static_cast<const ast::FunctionCallExpression*>(expression));
        case ast::ExpressionKind::GenericExpression:
            return generateGenericExpression(static_cast<const ast::GenericExpression*>(expression));
        case ast::ExpressionKind::IdentifierExpression:
            return generateIdentifierExpression(static_cast<const ast::IdentifierExpression*>(expression));
        case ast::ExpressionKind::IndexExpression:
            return generateIndexExpression(static_cast<const ast::IndexExpression*>(expression));
        case ast::ExpressionKind::MemberAccessExpression:
            return generateMemberAccessExpression(static_cast<const ast::MemberAccessExpression*>(expression));
        case ast::ExpressionKind::NumberLiteralExpression:
            return generateNumberLiteralExpression(static_cast<const ast::NumberLiteralExpression*>(expression));
        case ast::ExpressionKind::PostfixExpression:
            return generatePostfixExpression(static_cast<const ast::PostfixExpression*>(expression));
        case ast::ExpressionKind::PrefixExpression:
            return generatePrefixExpression(static_cast<const ast::PrefixExpression*>(expression));
        case ast::ExpressionKind::ScopeResolutionExpression:
            return generateScopeResolutionExpression(static_cast<const ast::ScopeResolutionExpression*>(expression));
        case ast::ExpressionKind::StringLiteralExpression:
            return generateStringLiteralExpression(static_cast<const ast::StringLiteralExpression*>(expression));
        case ast::ExpressionKind::TypeCastExpression:
            return generateTypeCastExpression(static_cast<const ast::TypeCastExpression*>(expression));
        default:
            ASSERT_UNREACHABLE("No codegen method for expression kind "
                               + std::to_string(static_cast<int>(expression->kind())));
            return nullptr;
    }
}

llvm::Value* generateStatement(const ast::Statement* statement) {
    switch (statement->kind()) {
        case ast::StatementKind::AggregateDeclarationStatement:
            return generateAggregateDeclarationStatement(
                static_cast<const ast::AggregateDeclarationStatement*>(statement));
        case ast::StatementKind::AliasStatement:
            return generateAliasStatement(static_cast<const ast::AliasStatement*>(statement));
        case ast::StatementKind::BreakStatement:
            return generateBreakStatement(static_cast<const ast::BreakStatement*>(statement));
        case ast::StatementKind::ContinueStatement:
            return generateContinueStatement(static_cast<const ast::ContinueStatement*>(statement));
        case ast::StatementKind::EmptyStatement:
            return generateEmptyStatement(static_cast<const ast::EmptyStatement*>(statement));
        case ast::StatementKind::EnumDeclarationStatement:
            return generateEnumDeclarationStatement(static_cast<const ast::EnumDeclarationStatement*>(statement));
        case ast::StatementKind::ExpressionStatement:
            return generateExpressionStatement(static_cast<const ast::ExpressionStatement*>(statement));
        case ast::StatementKind::FunctionDeclarationStatement:
            return generateFunctionDeclarationStatement(
                static_cast<const ast::FunctionDeclarationStatement*>(statement));
        case ast::StatementKind::IfStatement:
            return generateIfStatement(static_cast<const ast::IfStatement*>(statement));
        case ast::StatementKind::RepeatLoopStatement:
            return generateRepeatLoopStatement(static_cast<const ast::RepeatLoopStatement*>(statement));
        case ast::StatementKind::ReturnStatement:
            return generateReturnStatement(static_cast<const ast::ReturnStatement*>(statement));
        case ast::StatementKind::SwitchStatement:
            return generateSwitchStatement(static_cast<const ast::SwitchStatement*>(statement));
        case ast::StatementKind::VariableDeclarationStatement:
            return generateVariableDeclarationStatement(
                static_cast<const ast::VariableDeclarationStatement*>(statement));
        case ast::StatementKind::WhileLoopStatement:
            return generateWhileLoopStatement(static_cast<const ast::WhileLoopStatement*>(statement));
        default:
            ASSERT_UNREACHABLE("No codegen method for statement kind "
                               + std::to_string(static_cast<int>(statement->kind())));
            return nullptr;
    }
}

llvm::Value* generateType(const ast::Type* type) {
    switch (type->kind()) {
        case ast::TypeKind::AggregateType: return generateAggregateType(static_cast<const ast::AggregateType*>(type));
        case ast::TypeKind::ArrayType: return generateArrayType(static_cast<const ast::ArrayType*>(type));
        case ast::TypeKind::FunctionType: return generateFunctionType(static_cast<const ast::FunctionType*>(type));
        case ast::TypeKind::GenericType: return generateGenericType(static_cast<const ast::GenericType*>(type));
        case ast::TypeKind::PointerType: return generatePointerType(static_cast<const ast::PointerType*>(type));
        case ast::TypeKind::SymbolType: return generateSymbolType(static_cast<const ast::SymbolType*>(type));
        default:
            ASSERT_UNREACHABLE("No codegen method for type kind " + std::to_string(static_cast<int>(type->kind())));
            return nullptr;
    }
}

}  // namespace codegen

}  // namespace Manganese