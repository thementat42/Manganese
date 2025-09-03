#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Value.h>

#include <frontend/lexer.hpp>
#include <frontend/semantic.hpp>
#include <global_macros.hpp>
#include <middleend/codegen/codegen_base.hpp>
#include <type_traits>

#include "frontend/semantic/semantic_analyzer.hpp"

namespace Manganese {

namespace codegen {

auto IRGenerator::visit(ast::AggregateInstantiationExpression* expression) -> exprvisit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::AggregateLiteralExpression* expression) -> exprvisit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::ArrayLiteralExpression* expression) -> exprvisit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::AssignmentExpression* expression) -> exprvisit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::BinaryExpression* expression) -> exprvisit_t {
    using enum lexer::TokenType;
    if (expression->op == And || expression->op == Or) { return generateShortCircuitBinaryExpression(expression); }
    ast::Expression* left = expression->left.get();
    ast::Expression* right = expression->right.get();
    llvm::Value* leftValue = visit(left);
    llvm::Value* rightValue = visit(right);
    if (!leftValue || !rightValue) {
        // If either side is null, we cannot generate the binary expression
        ASSERT_UNREACHABLE("Left or right expression in binary expression is null");
        return nullptr;
    }
    switch (expression->op) {
        // TODO: Handle unsigned/signed distinction in division and mod
        case Plus: return theBuilder->CreateAdd(leftValue, rightValue, "addtmp");
        case Minus: return theBuilder->CreateSub(leftValue, rightValue, "subtmp");
        case Mul: return theBuilder->CreateMul(leftValue, rightValue, "multmp");
        case Div:
            if (semantic::isUInt(left->getType()) && semantic::isUInt(right->getType())) {
                // Only do unsigned division if both arguments are unsigned (since there's no sign info to lose)
                return theBuilder->CreateUDiv(leftValue, rightValue, "udivtmp");
            } else if (semantic::isFloat(left->getType()) || semantic::isFloat(right->getType())) {
                // If at least one of the arguments is a float, we want to preserve that
                return theBuilder->CreateFDiv(leftValue, rightValue, "fdivtmp");
            } else {
                // IF at least one of the arguments is signed, we want to preserve signedness
                return theBuilder->CreateSDiv(leftValue, rightValue, "sdivtmp");
            }
        case FloorDiv:
            NOT_IMPLEMENTED("Floor division is not implemented yet");
            return nullptr;  // Placeholder for future implementation
        case Mod: return theBuilder->CreateSRem(leftValue, rightValue, "modtmp");
        case Exp:
            NOT_IMPLEMENTED("Exponentiation is not implemented yet");
            return nullptr;  // Placeholder for future implementation
        case GreaterThan: return theBuilder->CreateICmpSGT(leftValue, rightValue, "cmptmp");
        case GreaterThanOrEqual: return theBuilder->CreateICmpSGE(leftValue, rightValue, "cmptmp");
        case LessThan: return theBuilder->CreateICmpSLT(leftValue, rightValue, "cmptmp");
        case LessThanOrEqual: return theBuilder->CreateICmpSLE(leftValue, rightValue, "cmptmp");
        case Equal: return theBuilder->CreateICmpEQ(leftValue, rightValue, "cmptmp");
        case NotEqual: return theBuilder->CreateICmpNE(leftValue, rightValue, "cmptmp");
        case BitAnd: return theBuilder->CreateAnd(leftValue, rightValue, "bitandtmp");
        case BitOr: return theBuilder->CreateOr(leftValue, rightValue, "bitorTmp");
        case BitXor: return theBuilder->CreateXor(leftValue, rightValue, "bitxortmp");
        case BitLShift: return theBuilder->CreateShl(leftValue, rightValue, "bitshltmp");
        case BitRShift:
            if (semantic::isSignedInt(left->getType())) {
                return theBuilder->CreateAShr(leftValue, rightValue, "ashrtmp");
            }
            return theBuilder->CreateLShr(leftValue, rightValue, "lshrtmp");
        case MemberAccess:
            NOT_IMPLEMENTED("Member access operator is not implemented yet");
            return nullptr;  // Placeholder for future implementation
        case ScopeResolution:
            NOT_IMPLEMENTED("Scope resolution operator is not implemented yet");
            return nullptr;  // Placeholder for future implementation
        default:
            ASSERT_UNREACHABLE(std::format("Unsupported binary operator: {}", static_cast<int>(expression->op)));
            return nullptr;
    }
}
auto IRGenerator::visit(ast::BoolLiteralExpression* expression) -> exprvisit_t {
    // Note: LLVM doesn't have a dedicated boolean type per se (it represents boolean values as i1)
    return llvm::ConstantInt::get(llvm::Type::getInt1Ty(*theContext), expression->value);
}
auto IRGenerator::visit(ast::CharLiteralExpression* expression) -> exprvisit_t {
    // Since we're using char32_t for character literals
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(*theContext), static_cast<uint32_t>(expression->value),
                                  /*isSigned=*/false);
}
auto IRGenerator::visit(ast::FunctionCallExpression* expression) -> exprvisit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::GenericExpression* expression) -> exprvisit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::IdentifierExpression* expression) -> exprvisit_t {
    // Look up the identifier in the named values map
    llvm::Value* value = namedValues[expression->value];
    if (!value) {
        // This should have been checked during semantic analysis
        ASSERT_UNREACHABLE(std::format("Identifier '{}' not found in named values", expression->value));
        return nullptr;
    }
    return value;
}
auto IRGenerator::visit(ast::IndexExpression* expression) -> exprvisit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::MemberAccessExpression* expression) -> exprvisit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::NumberLiteralExpression* expression) -> exprvisit_t {
    auto numberVisitor = [this](auto&& arg) -> llvm::Constant* {
        llvm::LLVMContext& ctxt = *theContext;
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_integral_v<T> && std::is_signed_v<T>) {
            // Note: llvm::ConstantInt::get expects a uint64_t for the value, so we cast it accordingly
            // This does not change the actual value since LLVM will interpret the bits as a signed integer (from
            // the isSigned parameter)

            return llvm::ConstantInt::get(llvm::IntegerType::get(ctxt, getBitWidth<T>()), static_cast<uint64_t>(arg),
                                          /*isSigned=*/true);
        } else if constexpr (std::is_integral_v<T> && std::is_unsigned_v<T>) {
            return llvm::ConstantInt::get(llvm::IntegerType::get(ctxt, getBitWidth<T>()), arg, /*isSigned=*/false);
        } else if constexpr (std::is_same_v<T, float32_t>) {
            return llvm::ConstantFP::get(llvm::Type::getFloatTy(ctxt), arg);
        } else if constexpr (std::is_same_v<T, float64_t>) {
            return llvm::ConstantFP::get(llvm::Type::getDoubleTy(ctxt), arg);
        } else {
            ASSERT_UNREACHABLE(std::format("Unsupported number literal type: {}", typeid(T).name()));
            return llvm::ConstantInt::get(llvm::IntegerType::get(ctxt, 32), 0);  // Fallback to zero + error
        }
    };
    return std::visit(numberVisitor, expression->value);
}
auto IRGenerator::visit(ast::PostfixExpression* expression) -> exprvisit_t {
    if (expression->op != lexer::TokenType::Inc && expression->op != lexer::TokenType::Dec) {
        ASSERT_UNREACHABLE(std::format("Unsupported postfix operator: {}", static_cast<int>(expression->op)));
        return nullptr;
    }
    ast::Expression* left = expression->left.get();
    llvm::Value* leftVal = visit(expression->left);
    llvm::Value* oldValue = theBuilder->CreateLoad(leftVal->getType(), leftVal, "loadtmp");
    if (semantic::isAnyInt(left->getType())) {
        bool isSigned = semantic::isSignedInt(left->getType());
        llvm::Value* one = llvm::ConstantInt::get(oldValue->getType(), 1, isSigned);
        llvm::Value* newValue;
        if (expression->op == lexer::TokenType::Inc) {
            newValue = theBuilder->CreateAdd(oldValue, one, "inc");
        } else {
            newValue = theBuilder->CreateSub(oldValue, one, "dec");
        }
        theBuilder->CreateStore(newValue, leftVal);
        return oldValue;  // Postfix increment/decrement returns the old value
    } else {
        // Floating point types
        llvm::Value* one = llvm::ConstantFP::get(oldValue->getType(), 1.0);
        llvm::Value* newValue;
        if (expression->op == lexer::TokenType::Inc) {
            newValue = theBuilder->CreateFAdd(oldValue, one, "finc");
        } else {
            newValue = theBuilder->CreateFSub(oldValue, one, "fdec");
        }
        theBuilder->CreateStore(newValue, leftVal);
        return oldValue;
    }
}
auto IRGenerator::visit(ast::PrefixExpression* expression) -> exprvisit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::ScopeResolutionExpression* expression) -> exprvisit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::StringLiteralExpression* expression) -> exprvisit_t {
    llvm::Constant* strConstant = llvm::ConstantDataArray::getString(*theContext, expression->value, /*AddNull=*/true);
    llvm::GlobalVariable* globalStr = new llvm::GlobalVariable(
        *theModule, strConstant->getType(), /*isConstant=*/true, llvm::GlobalValue::PrivateLinkage, strConstant,
        /*Name=*/"str_literal_" + std::to_string(this->globalStrCounter++));

    // Index in GEP (tells LLVM to start from the beginning)
    llvm::Constant* zero = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*theContext), 0);
    llvm::Constant* indices[] = {zero /*indexes into the global variable holding the string literal*/,
                                 zero /*indexes into the first element of the array (first char)*/};
    llvm::Constant* strPtr = llvm::ConstantExpr::getInBoundsGetElementPtr(strConstant->getType(), globalStr, indices);
    return strPtr;
}
auto IRGenerator::visit(ast::TypeCastExpression* expression) -> exprvisit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}

}  // namespace codegen

}  // namespace Manganese