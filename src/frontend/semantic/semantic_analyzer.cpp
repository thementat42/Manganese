#include <core.hpp>
#include <frontend/lexer/token.hpp>
#include <frontend/semantic/semantic_analyzer.hpp>
#include <frontend/semantic/type_context.hpp>
#include <optional>
#include <utils/result.hpp>
#include <utils/str_to_num.hpp>
#include <utils/target_info.hpp>

namespace Manganese::semantic {

Result SemanticAnalyzer::analyze() {
    auto runPass = [this](auto passFunction) {
        Result passResult = Result::Success;

        for (parser::ParsedFile& file : parsedFiles) {
            if (std::invoke(passFunction, file) == Result::Failure) { passResult = Result::Failure; }
        }
        return passResult;
    };
    if (runPass([this](auto& f) { return buildScopeTree(f); }) == Result::Failure) { return Result::Failure; }

    symbolTable.switchToCheckingMode();

    if (runPass([this](auto& f) { return collectGlobals(f); }) == Result::Failure) { return Result::Failure; }
    if (runPass([this](auto& f) { return checkStatements(f); }) == Result::Failure) { return Result::Failure; }

    return Result::Success;
}

const Symbol* SemanticAnalyzer::resolveTypeSymbol(const ast::Type* typeNode) {
    if (typeNode->kind == ast::TypeKind::IdentifierType) {
        const auto* IdentifierType = static_cast<const ast::IdentifierType*>(typeNode);
        return symbolTable.lookup(IdentifierType->name);
    }

    if (typeNode->kind == ast::TypeKind::ScopedType) {
        const auto* scopedType = static_cast<const ast::ScopedType*>(typeNode);
        const Symbol* parentSymbol = resolveTypeSymbol(scopedType->scope);
        if ((parentSymbol == nullptr) || (parentSymbol->scopeDefined == nullptr)) {
            return nullptr;  // Parent symbol wasn't found or doesn't have a scope
        }

        if (scopedType->type->kind != ast::TypeKind::IdentifierType) {
            ASSERT_UNREACHABLE(
                "In resolveTypeSymbol: base case for recursion of a scoped type resolution should be an identifier");
        }
        const auto* memberIdentifierType = static_cast<const ast::IdentifierType*>(scopedType->type);
        return decltype(symbolTable)::scopedLookup(parentSymbol->scopeDefined, memberIdentifierType->name);
    }

    return nullptr;
}

const Symbol* SemanticAnalyzer::resolveScopeSymbol(const ast::Expression* expr) {
    if (expr->kind == ast::ExpressionKind::IdentifierExpression) {
        const auto* id = static_cast<const ast::IdentifierExpression*>(expr);
        return symbolTable.lookup(id->name);
    }
    if (expr->kind == ast::ExpressionKind::ScopeResolutionExpression) {
        const auto* scopeExpr = static_cast<const ast::ScopeResolutionExpression*>(expr);

        // recursively check the left hand side
        const Symbol* parentSymbol = resolveScopeSymbol(scopeExpr->scope);
        if ((parentSymbol == nullptr) || (parentSymbol->scopeDefined == nullptr)) { return nullptr; }

        if (scopeExpr->element->kind != ast::ExpressionKind::IdentifierExpression) { return nullptr; }
        const auto* memberId = static_cast<const ast::IdentifierExpression*>(scopeExpr->element);

        return decltype(symbolTable)::scopedLookup(parentSymbol->scopeDefined, memberId->name);
    }
    return nullptr;
}

bool SemanticAnalyzer::isMutableExpression(const ast::Expression* expr) {
    using enum ast::ExpressionKind;
    switch (expr->kind) {
        case IdentifierExpression: {
            const auto* id = static_cast<const ast::IdentifierExpression*>(expr);
            const Symbol* symbol = symbolTable.lookup(id->name);
            return (symbol != nullptr) ? symbol->isMutable : false;
        }
        case PrefixExpression: {
            const auto* prefix = static_cast<const ast::PrefixExpression*>(expr);
            if (prefix->op == lexer::TokenType::Dereference) {
                const SemanticType* operandType = prefix->right->semanticType;
                if (!operandType->isPointer()) { return false; }
                return static_cast<const Pointer*>(operandType)->isMutable;
            }
            return false;
        }
        case MemberAccessExpression: {
            const auto* mem = static_cast<const ast::MemberAccessExpression*>(expr);
            return isMutableExpression(mem->object);
        }
        case IndexExpression: {
            const auto* index = static_cast<const ast::IndexExpression*>(expr);
            return isMutableExpression(index->variable);
        }
        case ScopeResolutionExpression: {
            const auto* scope = static_cast<const ast::ScopeResolutionExpression*>(expr);
            const Symbol* scopeSymbol = resolveScopeSymbol(scope->scope);
            if ((scopeSymbol == nullptr) || (scopeSymbol->scopeDefined == nullptr)) { return false; }
            if (scope->element->kind != ast::ExpressionKind::IdentifierExpression) { return false; }
            const auto* memberId = static_cast<const ast::IdentifierExpression*>(scope->element);
            const Symbol* memberSymbol = decltype(symbolTable)::scopedLookup(scopeSymbol->scopeDefined, memberId->name);
            return (memberSymbol != nullptr) ? memberSymbol->isMutable : false;
        }
        default: return false;
    }
}

std::optional<std::uint64_t> SemanticAnalyzer::computeExplicitArrayLength(const ast::Expression* lengthExpression) {
    using enum lexer::TokenType;
    switch (lengthExpression->kind) {
        case ast::ExpressionKind::AggregateLiteralExpression:
        case ast::ExpressionKind::AlignofExpression: {
            const auto* alignofExpression = static_cast<const ast::AlignofExpression*>(lengthExpression);
            return alignofExpression->semanticType->alignment(typeContext.getTargetInfo());
        }
        case ast::ExpressionKind::BinaryExpression: {
            const auto* binaryExpression = static_cast<const ast::BinaryExpression*>(lengthExpression);
            auto leftValue = computeExplicitArrayLength(binaryExpression->left);
            auto rightValue = computeExplicitArrayLength(binaryExpression->right);
            if (!leftValue.has_value() || !rightValue.has_value()) { break; }
            switch (binaryExpression->op) {
                case Plus: return *leftValue + *rightValue;
                case Minus: return *leftValue - *rightValue;
                case Mul: return *leftValue * *rightValue;
                case Div:
                    return static_cast<std::uint64_t>(static_cast<float>(*leftValue) / static_cast<float>(*rightValue));
                case FloorDiv: return *leftValue / *rightValue;
                case Mod: return *leftValue % *rightValue;
                case GreaterThan: return static_cast<std::uint64_t>(*leftValue > *rightValue);
                case GreaterThanOrEqual: return static_cast<std::uint64_t>(*leftValue >= *rightValue);
                case LessThan: return static_cast<std::uint64_t>(*leftValue < *rightValue);
                case LessThanOrEqual: return static_cast<std::uint64_t>(*leftValue <= *rightValue);
                case Equal: return static_cast<std::uint64_t>(*leftValue == *rightValue);
                case NotEqual: return static_cast<std::uint64_t>(*leftValue != *rightValue);
                case And:
                    return static_cast<std::uint64_t>(static_cast<bool>(*leftValue) && static_cast<bool>(*rightValue));
                case Or:
                    return static_cast<std::uint64_t>(static_cast<bool>(*leftValue) || static_cast<bool>(*rightValue));
                case BitAnd: return static_cast<std::uint64_t>(*leftValue & *rightValue);
                case BitOr: return static_cast<std::uint64_t>(*leftValue | *rightValue);
                case BitXor: return static_cast<std::uint64_t>(*leftValue ^ *rightValue);
                case BitLShift: return static_cast<std::uint64_t>(*leftValue << *rightValue);
                case BitRShift: return static_cast<std::uint64_t>(*leftValue >> *rightValue);
                case MemberAccess:
                case ScopeResolution:
                case Assignment: return std::nullopt;  // can't fold these
                default: break;
            }
            ASSERT_UNREACHABLE(std::format("Unknown binary operator '{}' in computeExplicitArrayLength",
                                           lexer::tokenTypeToString(binaryExpression->op)));
        }
        case ast::ExpressionKind::BoolLiteralExpression: {
            const auto* boolLiteralExpression = static_cast<const ast::BoolLiteralExpression*>(lengthExpression);
            return boolLiteralExpression->value ? 1 : 0;
        }
        case ast::ExpressionKind::CharLiteralExpression: {
            const auto* charLiteralExpression = static_cast<const ast::CharLiteralExpression*>(lengthExpression);
            return static_cast<std::uint64_t>(charLiteralExpression->value);
        }
        case ast::ExpressionKind::NumberLiteralExpression: {
            const auto* num = static_cast<const ast::NumberLiteralExpression*>(lengthExpression);
            const utils::string_conversion_result_t result = utils::stringToNumber<std::uint64_t>(num->value, num->isFloat);
            if (!result.exists) {
                logError(lengthExpression, "Invalid number literal '{}'", num->value);
                break;
            }
            if (result.overflowed) {
                logError(lengthExpression, "Number literal '{}' cannot fit in a 64-bit value", num->value);
                break;
            }
            return result.value;
        }
        case ast::ExpressionKind::PostfixExpression: {
            const auto* postfixExpression = static_cast<const ast::PostfixExpression*>(lengthExpression);
            auto operandValue = computeExplicitArrayLength(postfixExpression->left);
            if (!operandValue.has_value()) { break; }
            switch (postfixExpression->op) {
                case UnaryPlus: return static_cast<std::uint64_t>(+*operandValue);
                case UnaryMinus: return static_cast<std::uint64_t>(-*operandValue);
                case Not: return static_cast<std::uint64_t>(!static_cast<bool>(*operandValue));
                case BitNot: return static_cast<std::uint64_t>(~*operandValue);
                case Inc:
                case AddressOf:
                case Dereference:
                    // can only increment/decrement lvalues which aren't foldable
                    // addresses are runtime values
                    return std::nullopt;
                default: break;
            }
            ASSERT_UNREACHABLE(std::format("Unknown postfix operator '{}' in computeExplicitArrayLength",
                                           lexer::tokenTypeToString(postfixExpression->op)));
        }
        case ast::ExpressionKind::PrefixExpression: {
            // can only increment/decrement lvalues which aren't foldable
            return std::nullopt;
        }
        case ast::ExpressionKind::SizeofExpression: {
            const auto* sizeofExpression = static_cast<const ast::SizeofExpression*>(lengthExpression);
            return sizeofExpression->semanticType->size(typeContext.getTargetInfo());
        }
        case ast::ExpressionKind::TypeCastExpression: {
            const auto* typeCastExpression = static_cast<const ast::TypeCastExpression*>(lengthExpression);
            return computeExplicitArrayLength(typeCastExpression->originalValue);
        }
        default: break;
    }
    return std::nullopt;
}

}  // namespace Manganese::semantic
