#include <core.hpp>
#include <frontend/ast.hpp>
#include <frontend/lexer/token.hpp>
#include <frontend/semantic/analyzer.hpp>
#include <frontend/semantic/symbol_table.hpp>
#include <frontend/semantic/type_context.hpp>
#include <mnstl/number.hxx>
#include <utils/result.hpp>

namespace Manganese::semantic {
auto analyzer::visit(ast::AssignmentExpression* expression) -> exprvisit_t {
    auto result = exprvisit_t::Success;
    if (visit(expression->assignee) == exprvisit_t::Failure) { result = exprvisit_t::Failure; }
    if (visit(expression->value) == exprvisit_t::Failure) { result = exprvisit_t::Failure; }

    if (!expression->assignee->semanticType || !expression->value->semanticType) { return exprvisit_t::Failure; }

    if (!isLvalue(expression->assignee)) {
        logError(expression->assignee, "Cannot assign a value to expression '{}'", expression->assignee->toString());
        result = exprvisit_t::Failure;
    }

    const typeCompatibilityResult isAssignmentValid
        = areTypesCompatible(expression->assignee->semanticType, expression->value->semanticType);
    if (!isAssignmentValid) {
        logError(expression, "Cannot assign a value of type {} to a value of type {}",
                 expression->value->semanticType->toString(), expression->assignee->semanticType->toString());
        result = exprvisit_t::Failure;
    } else if (isAssignmentValid.result == Compatible_t::Warning) {
        logWarning(expression, "{}", isAssignmentValid.message);
    }
    if (!isMutableExpression(expression->assignee)) {
        logError(expression->assignee, "Cannot assign a value to immutable value '{}'",
                 expression->assignee->toString());
        result = exprvisit_t::Failure;
    }

    expression->semanticType = expression->assignee->semanticType;
    return result;
}

auto analyzer::visit(ast::BinaryExpression* expression) -> exprvisit_t {
    auto result = exprvisit_t::Success;

    if (visit(expression->left) == exprvisit_t::Failure) { result = exprvisit_t::Failure; }
    if (visit(expression->right) == exprvisit_t::Failure) { result = exprvisit_t::Failure; }
    if (!expression->left->semanticType) {
        logError(expression, "Could not deduce type of expression {}", expression->left->toString());
        return exprvisit_t::Failure;
    }
    if (!expression->right->semanticType) {
        logError(expression, "Could not deduce type of expression {}", expression->right->toString());
        return exprvisit_t::Failure;
    }

    const SemanticType* lhsType = expression->left->semanticType;
    const SemanticType* rhsType = expression->right->semanticType;
    const lexer::TokenType op = expression->op;

    if (isLogicalOp(op)) {
        if (!lhsType->isBoolean() || !rhsType->isBoolean()) {
            logError(expression, "Operator '{}' requires boolean operands, got {} and {}", lexer::tokenTypeToString(op),
                     lhsType->toString(), rhsType->toString());
            result = exprvisit_t::Failure;
        }
        expression->semanticType = typeContext.getPrimitive(ast::PrimitiveType_t::boolean);
        return result;
    } else if (isBitwiseOp(op)) {
        if (!lhsType->isInteger() || !rhsType->isInteger()) {
            logError(expression, "Bitwise operators require integer operands, got {} and {}", lhsType->toString(),
                     rhsType->toString());
            result = exprvisit_t::Failure;
        }
        expression->semanticType = promoteNumericTypes(lhsType, rhsType);
        return result;
    } else if (isRelationalOp(op)) {
        if (!areTypesComparable(lhsType, rhsType)) {
            logError(expression, "Cannot compare incompatible types {} and {}", lhsType->toString(),
                     rhsType->toString());
            result = exprvisit_t::Failure;
        }
        expression->semanticType = typeContext.getPrimitive(ast::PrimitiveType_t::boolean);
        return result;
    } else if (isArithmeticOp(op)) {
        if (lhsType->isPointer() || rhsType->isPointer()) { return analyzePointerArithmetic(expression); }
        if (op == lexer::TokenType::Plus && lhsType->primitiveType == ast::PrimitiveType_t::str
            && rhsType->primitiveType == ast::PrimitiveType_t::str) {
            expression->semanticType = typeContext.getPrimitive(ast::PrimitiveType_t::str);
            return exprvisit_t::Success;
        }
        const SemanticType* commonType = promoteNumericTypes(lhsType, rhsType);
        if (!commonType) {
            logError(expression, "Invalid operands for arithmetic operator '{}': {} and {}",
                     lexer::tokenTypeToString(op), lhsType->toString(), rhsType->toString());
            return exprvisit_t::Failure;
        }

        expression->semanticType = commonType;
        return result;
    }
    ASSERT_UNREACHABLE(
        std::format("Unhandled binary operator {} in visit(BinaryExpression)", lexer::tokenTypeToString(op)));
};

auto analyzer::visit(ast::PostfixExpression* expression) -> exprvisit_t {
    auto result = visit(expression->left);
    if (result == exprvisit_t::Failure) { return result; }
    // the only postfix operators are ++ and -- so the expression must be an integer
    if (!expression->left->semanticType) {
        logError(expression, "Could not deduce type of expression {}", expression->toString());
        return exprvisit_t::Failure;
    }

    // set the type here even if the expression is invalid so we don't have a bunch of propagating nulls
    expression->semanticType = typeContext.getPrimitive(expression->left->semanticType->primitiveType);
    ast::Expression* operand = expression->left;

    if (!isLvalue(operand)) {
        logError(operand, "Cannot apply operator {} to an rvalue '{}'", lexer::tokenTypeToString(expression->op),
                 operand->toString());
        return exprvisit_t::Failure;
    }
    if (!operand->semanticType->isInteger() && operand->semanticType->isPointer()) {
        logError(expression, "operator {} can only be applied to integer or pointer types",
                 lexer::tokenTypeToString(expression->op));
        return exprvisit_t::Failure;
    }
    if (!isMutableExpression(operand)) {
        logError(operand, "Cannot apply operator {} to an immutable value '{}'",
                 lexer::tokenTypeToString(expression->op), operand->toString());
        return exprvisit_t::Failure;
    }

    return exprvisit_t::Success;
}

auto analyzer::visit(ast::PrefixExpression* expression) -> exprvisit_t {
    if (visit(expression->right) == exprvisit_t::Failure) { return exprvisit_t::Failure; }
    if (!expression->right->semanticType) {
        logError(expression, "Could not deduce type of expression {}", expression->toString());
        return exprvisit_t::Failure;
    }

    const SemanticType* rhsType = expression->right->semanticType;

    using enum lexer::TokenType;
    switch (expression->op) {
        case Inc:
        case Dec: {
            expression->semanticType = typeContext.getPrimitive(rhsType->primitiveType);
            if (!isLvalue(expression->right)) {
                logError(expression->right, "Cannot apply operator {} to an rvalue '{}'",
                         lexer::tokenTypeToString(expression->op), expression->right->toString());
                return exprvisit_t::Failure;
            }
            if (!rhsType->isInteger() && !rhsType->isPointer()) {
                logError(expression, "operator {} can only be applied to integer or pointer types",
                         lexer::tokenTypeToString(expression->op));
                return exprvisit_t::Failure;
            }
            if (!isMutableExpression(expression->right)) {
                logError(expression->right, "Cannot apply operator {} to an immutable value '{}'",
                         lexer::tokenTypeToString(expression->op), expression->right->toString());
                return exprvisit_t::Failure;
            }
        } break;

        case BitNot: {
            expression->semanticType = typeContext.getPrimitive(rhsType->primitiveType);
            if (!rhsType->isInteger()) {
                logError(expression, "operator {} can only be applied to integer types",
                         lexer::tokenTypeToString(expression->op));
                return exprvisit_t::Failure;
            }
        } break;

        case UnaryPlus:
        case UnaryMinus: {
            if (visit(expression->right) == exprvisit_t::Failure) { return exprvisit_t::Failure; }
            const SemanticType* opType = expression->right->semanticType;
            if (!opType) { return exprvisit_t::Failure; }
            if (!opType->isNumeric()) {
                logError(expression, "Operator '{}' can only be applied to numeric types (got '{}')",
                         lexer::tokenTypeToString(expression->op), opType->toString());
                return exprvisit_t::Failure;
            }
            if (expression->op == UnaryMinus && opType->isUnsignedInteger()) {
                logWarning(expression, "Applying a '-' to an unsigned integer type  ('{}') causes wrapping",
                           opType->toString());
            }
            expression->semanticType = opType;
        } break;

        case AddressOf: {
            // temporary, to avoid failures
            expression->semanticType = typeContext.getPointer(expression->right->semanticType, false);
            if (!isLvalue(expression->right)) {
                logError(expression->right, "Cannot take the address of r-value '{}'", expression->right->toString());
                return exprvisit_t::Failure;
            }
            const bool isMut = isMutableExpression(expression->right);
            expression->semanticType = typeContext.getPointer(expression->right->semanticType, isMut);
        } break;

        case Dereference: {
            if (!expression->right->semanticType->isPointer()) {
                logError(expression, "Dereferencing cannot be applied to a non-pointer type");
                // dummy (figure out a better option later)
                expression->semanticType = typeContext.getPrimitive(ast::PrimitiveType_t::u8);
                return exprvisit_t::Failure;
            }
            expression->semanticType = static_cast<const Pointer*>(expression->right->semanticType)->baseType;
        } break;

        case Not: {
            const typeCompatibilityResult canBeBool
                = areTypesCompatible(rhsType, typeContext.getPrimitive(ast::PrimitiveType_t::boolean));

            if (!canBeBool) {
                logError(expression, "Operator '!' requires a boolean operand, got '{}'", rhsType->toString());
                return exprvisit_t::Failure;
            } else if (canBeBool.result == Compatible_t::Warning) {
                logWarning(expression, "{}", canBeBool.message);
            }
            expression->semanticType = typeContext.getPrimitive(ast::PrimitiveType_t::boolean);
        } break;

        default:
            ASSERT_UNREACHABLE(std::format("Unknown prefix operator {}", lexer::tokenTypeToString(expression->op)));
    }
    return exprvisit_t::Success;
}

auto analyzer::visit(ast::TypeCastExpression* expression) -> exprvisit_t {
    auto result = exprvisit_t::Success;
    ContextGuard guard(context.typeCastDepth, static_cast<decltype(context.typeCastDepth)>(context.typeCastDepth + 1));
    if (visit(expression->originalValue) == exprvisit_t::Failure) { result = exprvisit_t::Failure; }
    if (!expression->originalValue->semanticType) {
        logError(expression, "Could not deduce type of expression {}", expression->originalValue->toString());
        return exprvisit_t::Failure;
    }
    if (visit(expression->targetType) == exprvisit_t::Failure) { result = exprvisit_t::Failure; }
    expression->semanticType = expression->targetType->semanticType;

    return result;
}

}  // namespace Manganese::semantic