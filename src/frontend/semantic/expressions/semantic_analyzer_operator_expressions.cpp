#include <frontend/semantic/semantic_analyzer.hpp>
#include <frontend/semantic/semantic_type_helpers.hpp>

namespace Manganese {
using ast::toStringOr;
namespace semantic {

void SemanticAnalyzer::checkBinaryExpression(ast::BinaryExpression* expression) {
    checkExpression(expression->left.get());
    checkExpression(expression->right.get());

    ast::Type* leftType = expression->left->getType();
    ast::Type* rightType = expression->right->getType();

    bool isInvalidBinaryExpression = false;

    if (!leftType) {
        logError("Could not deduce the type of {} in {} {} {}", expression, toStringOr(expression->left),
                 toStringOr(expression->left), lexer::tokenTypeToString(expression->op), toStringOr(expression->right));
        isInvalidBinaryExpression = true;
    }
    if (!rightType) {
        logError("Could not deduce the type of {} in {} {} {}", expression, toStringOr(expression->right),
                 toStringOr(expression->left), lexer::tokenTypeToString(expression->op), toStringOr(expression->right));
        isInvalidBinaryExpression = true;
    }
    if (isInvalidBinaryExpression) {
        expression->setType(nullptr);
        return;
    }
    ast::TypeSPtr_t resultType = resolveBinaryExpressionType(expression);
    if (!resultType) {
        logError("Invalid binary expression: {} (operator {} is not supported for types {} and {})", expression,
                 toStringOr(expression), lexer::tokenTypeToString(expression->op), toStringOr(leftType),
                 toStringOr(rightType));
    }
    expression->setType(resultType);
}

void SemanticAnalyzer::checkPostfixExpression(ast::PostfixExpression* expression) {
    DISCARD(expression);
    NOT_IMPLEMENTED;
}
void SemanticAnalyzer::checkPrefixExpression(ast::PrefixExpression* expression) {
    DISCARD(expression);
    NOT_IMPLEMENTED;
}

void SemanticAnalyzer::checkTypeCastExpression(ast::TypeCastExpression* expression) {
    checkExpression(expression->originalValue.get());
    if (!expression->originalValue->getType()) {
        logError("Cannot cast expression {} to type {} since it has no computed type.", expression,
                 toStringOr(expression->originalValue), toStringOr(expression->targetType));
        return;
    }
    if (!typeExists(expression->targetType)) {
        logError("Type {} in cast expression '{}' does not exist", expression, toStringOr(expression->targetType),
                 toStringOr(expression->originalValue));
        return;
    }
    bool isCastFromPrimitiveToPrimitive = true;
    if (!ast::isPrimitiveType(expression->originalValue->getType())) {
        isCastFromPrimitiveToPrimitive = false;
        logError("Cannot cast an object of type '{}'. Only casts between primitive types are supported.", expression,
                 toStringOr(expression->originalValue->getType()));
    }
    if (!ast::isPrimitiveType(expression->targetType)) {
        isCastFromPrimitiveToPrimitive = false;
        logError("Cannot cast an object to a '{}'. Only casts between primitive types are supported.", expression,
                 toStringOr(expression->targetType));
    }
    if (!isCastFromPrimitiveToPrimitive) { return; }
    if (isString(expression->originalValue->getType()) && isChar(expression->targetType.get())) {
        logError("Cast from type 'string' to type 'char' is not supported", expression);
        return;
    }
    // Casting to and from any other primitive type is supported
    expression->setType(expression->targetType);
}

// ===== Helpers =====

ast::TypeSPtr_t SemanticAnalyzer::resolveBinaryExpressionType(ast::BinaryExpression* binaryExpression) const
    noexcept_if_release {
    using ast::TypeKind;
    using enum lexer::TokenType;

    auto leftType = binaryExpression->left->getTypePtr();
    auto rightType = binaryExpression->right->getTypePtr();
    auto op = binaryExpression->op;

    if (!leftType || !rightType) {
        return nullptr;  // Can't resolve the type if we don't know both types
    }

    if (leftType->kind() == TypeKind::ArrayType) {
        return SemanticAnalyzer::resolveArrayBinaryExpressionType(binaryExpression);
        // Some limited operations between arrays and other types are allowed (with arrays always on the left)
    }

    if (leftType->kind() != rightType->kind()) {
        logError("Cannot perform binary operation {} on types {} and {}", binaryExpression,
                 lexer::tokenTypeToString(op), toStringOr(leftType), toStringOr(rightType));
        return nullptr;  // Types must match for binary operations
    }

    if (leftType->kind() != TypeKind::SymbolType) {
        logError(
            "Cannot perform binary operation {} '{}'. Only the primitive symbol types can be used in binary operations",
            binaryExpression, lexer::tokenTypeToString(op), toStringOr(leftType));
    }

    switch (op) {
        case Plus:
        case Minus:
        case Mul:
        case Exp:
        case Div:
        case FloorDiv:
        case Mod: return SemanticAnalyzer::resolveArithmeticBinaryExpressionType(binaryExpression, op);
        case GreaterThan:
        case GreaterThanOrEqual:
        case LessThan:
        case LessThanOrEqual:
        case Equal:
        case NotEqual: {
            if (!areTypesCompatible(leftType.get(), rightType.get())) {
                logError("Cannot compare types {} and {} for ordering", binaryExpression, toStringOr(leftType),
                         toStringOr(rightType));
                return nullptr;
            }
            return std::make_shared<ast::SymbolType>("bool");
        }

        case BitAnd:
        case BitOr:
        case BitXor:
        case BitLShift:
        case BitRShift: {
            if (!isAnyInt(leftType.get()) || !isAnyInt(rightType.get())) {
                logError("Bitwise operations can only be performed on integer types "
                         "not {} and {}",
                         binaryExpression, toStringOr(leftType), toStringOr(rightType));
                return nullptr;
            }
            return SemanticAnalyzer::widestNumericType(leftType.get(), rightType.get());
        }
        case And:
        case Or: {
            if (!isBool(leftType.get()) || !isBool(rightType.get())) {
                logError("Logical operations can only be performed on boolean types "
                         "not {} and {}",
                         binaryExpression, toStringOr(leftType), toStringOr(rightType));
                return nullptr;
            }
            return std::make_shared<ast::SymbolType>("bool");
        }

        default:
            ASSERT_UNREACHABLE(std::format("No binary expression handler for {}", lexer::tokenTypeToString(op)))
            return nullptr;
    }
}

ast::TypeSPtr_t SemanticAnalyzer::resolveArithmeticBinaryExpressionType(ast::BinaryExpression* binaryExpression,
                                                                        lexer::TokenType op) const noexcept_if_release {
    using ast::TypeKind;
    using lexer::TokenType;
    auto leftType = binaryExpression->left->getTypePtr();
    auto rightType = binaryExpression->right->getTypePtr();

    if (op == TokenType::Plus) {
        // string + string or string + char or char + string => string
        if ((isString(leftType.get()) && isString(rightType.get()))
            || (isString(leftType.get()) && isChar(rightType.get()))
            || (isChar(leftType.get()) && isString(rightType.get()))) {
            return std::make_shared<ast::SymbolType>("string");
        }
        // numeric + numeric => widest numeric type
        if ((isAnyInt(leftType.get()) || isFloat(leftType.get()))
            && (isAnyInt(rightType.get()) || isFloat(rightType.get()))) {
            return SemanticAnalyzer::widestNumericType(leftType.get(), rightType.get());
        }
        logError("Operator '+' not supported for types {} and {}", binaryExpression, toStringOr(leftType),
                 toStringOr(rightType));
        return nullptr;
    }

    if (op == TokenType::Minus || op == TokenType::Exp) {
        if ((isAnyInt(leftType.get()) || isFloat(leftType.get()))
            && (isAnyInt(rightType.get()) || isFloat(rightType.get()))) {
            return SemanticAnalyzer::widestNumericType(leftType.get(), rightType.get());
        }
        logError("Operator '{}' not supported for types {} and {}", binaryExpression, lexer::tokenTypeToString(op),
                 toStringOr(leftType), toStringOr(rightType));
        return nullptr;
    }

    if (op == TokenType::Mul) {
        // string * uint or uint * string => string (string repetition)
        if ((isString(leftType.get()) && isUInt(rightType.get()))
            || (isUInt(leftType.get()) && isString(rightType.get()))) {
            return std::make_shared<ast::SymbolType>("string");
        }
        // numeric * numeric => widest numeric type
        if ((isAnyInt(leftType.get()) || isFloat(leftType.get()))
            && (isAnyInt(rightType.get()) || isFloat(rightType.get()))) {
            return SemanticAnalyzer::widestNumericType(leftType.get(), rightType.get());
        }
        logError("Operator '*' not supported for types {} and {}", binaryExpression, toStringOr(leftType),
                 toStringOr(rightType));
        return nullptr;
    }

    if (op == TokenType::Div) {
        // Division always returns a float (float64 if either operand is float, else float32)
        bool isNumeric = (isAnyInt(leftType.get()) || isFloat(leftType.get()))
            && (isAnyInt(rightType.get()) || isFloat(rightType.get()));
        if (!isNumeric) {
            logError("Division can only be performed on numeric types, not {} and {}", binaryExpression,
                     toStringOr(leftType), toStringOr(rightType));
            return nullptr;
        }
        // Use the wider float type if available, else default to float32
        if (toStringOr(leftType).ends_with("64") || toStringOr(rightType).ends_with("64")) {
            return std::make_shared<ast::SymbolType>(float64_str);
        }
        return std::make_shared<ast::SymbolType>(float32_str);
    }
    if (op == TokenType::FloorDiv) {
        // Floor division always returns an int (int64 if either operand is int64, else int32)
        if (!(isAnyInt(leftType.get()) && isAnyInt(rightType.get()))) {
            logError("Floor division can only be performed on integer types, not {} and {}", binaryExpression,
                     toStringOr(leftType), toStringOr(rightType));
            return nullptr;
        }
        // Use the wider int type if available, else default to int32
        if (toStringOr(leftType) == int64_str || toStringOr(rightType) == int64_str) {
            return std::make_shared<ast::SymbolType>(int64_str);
        }
        if (toStringOr(leftType) == uint64_str || toStringOr(rightType) == uint64_str) {
            return std::make_shared<ast::SymbolType>(uint64_str);
        }
        if (toStringOr(leftType) == uint32_str || toStringOr(rightType) == uint32_str) {
            return std::make_shared<ast::SymbolType>(uint32_str);
        }
        return std::make_shared<ast::SymbolType>(int32_str);
    }

    if (op == TokenType::Mod) {
        if (isAnyInt(leftType.get()) && isAnyInt(rightType.get())) {
            return SemanticAnalyzer::widestNumericType(leftType.get(), rightType.get());
        }
        logError("Modulus operation can only be performed on integer types not {} and {}", binaryExpression,
                 toStringOr(leftType), toStringOr(rightType));
        return nullptr;
    }
    return nullptr;
}

ast::TypeSPtr_t SemanticAnalyzer::widestNumericType(const ast::Type* type1,
                                                    const ast::Type* type2) const noexcept_if_release {
    using namespace semantic;
    if (!type1 || !type2) return nullptr;

    std::string t1 = toStringOr(type1);
    std::string t2 = toStringOr(type2);

    // Try direct lookup in foo
    auto it = numericTypePromotionTable.find({t1, t2});
    if (it != numericTypePromotionTable.end()) { return std::make_shared<ast::SymbolType>(it->second); }
    // Try reverse order
    it = numericTypePromotionTable.find({t2, t1});
    if (it != numericTypePromotionTable.end()) { return std::make_shared<ast::SymbolType>(it->second); }

    // Fallback: if both are the same, return that type
    if (t1 == t2) { return std::make_shared<ast::SymbolType>(t1); }

    for (const auto& candidate : fallbackTypeOrder) {
        if (t1 == candidate || t2 == candidate) { return std::make_shared<ast::SymbolType>(candidate); }
    }

    // Unknown combination
    return nullptr;
}

ast::TypeSPtr_t SemanticAnalyzer::resolveArrayBinaryExpressionType(ast::BinaryExpression* binaryExpression) const
    noexcept_if_release {
    using ast::TypeKind;
    using lexer::TokenType;

    const auto* left = binaryExpression->left.get();
    auto leftArrayType = static_cast<const ast::ArrayType*>(left->getType());
    const auto* right = binaryExpression->right.get();
    auto op = binaryExpression->op;

    switch (op) {
        case TokenType::Plus: {
            // Array + Array => Array of the same type
            if (right->getType()->kind() == TypeKind::ArrayType) {
                auto rightArrayType = static_cast<ast::ArrayType*>(right->getType());
                if (areTypesCompatible(leftArrayType->elementType.get(), rightArrayType->elementType.get())) {
                    return std::make_shared<ast::ArrayType>(leftArrayType->elementType);
                }
                logError("Cannot add arrays of different element types: {} and {}", binaryExpression,
                         toStringOr(left->getType()), toStringOr(right->getType()));
                return nullptr;
            }
            logError("Operator '+' not supported for array and {}", binaryExpression, toStringOr(right->getType()));

            return nullptr;
        }
        case TokenType::Mul: {
            // Array * Int => Array of the same type repeated n times
            if (isUInt(right->getType())) { return std::make_shared<ast::ArrayType>(leftArrayType->elementType); }
            logError("Operator '*' not supported for array and {}", binaryExpression, toStringOr(right->getType()));
            return nullptr;
        }
        case TokenType::Equal:
        case TokenType::NotEqual:
        case TokenType::GreaterThan:
        case TokenType::GreaterThanOrEqual:
        case TokenType::LessThan:
        case TokenType::LessThanOrEqual: {
            if (right->getType()->kind() != TypeKind::ArrayType) {
                logError("Cannot compare array with non-array type: {} and {}", binaryExpression,
                         toStringOr(left->getType()), toStringOr(right->getType()));
                return nullptr;
            }
            auto rightArrayType = static_cast<ast::ArrayType*>(right->getType());
            if (areTypesCompatible(leftArrayType->elementType.get(), rightArrayType->elementType.get())) {
                return std::make_shared<ast::SymbolType>("bool");
            }
            logError("Cannot compare arrays of different element types: {} and {}", binaryExpression,
                     toStringOr(left->getType()), toStringOr(right->getType()));
            return nullptr;
        }
        default:
            logError("Operator '{}' not supported for arrays", binaryExpression, lexer::tokenTypeToString(op));
            return nullptr;
    }
}

}  // namespace semantic

}  // namespace Manganese