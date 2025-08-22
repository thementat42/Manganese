#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Value.h>

#include <frontend/lexer.hpp>
#include <frontend/semantic.hpp>
#include <global_macros.hpp>
#include <middleend/codegen/codegen_base.hpp>
#include <type_traits>

namespace Manganese {

namespace codegen {

auto IRGenerator::visit(ast::AggregateInstantiationExpression* expression) -> exprvisit_t {
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
    llvm::Value* left = visit(expression->left);
    llvm::Value* right = visit(expression->right);
    if (!left || !right) {
        // If either side is null, we cannot generate the binary expression
        ASSERT_UNREACHABLE("Left or right expression in binary expression is null");
        return nullptr;
    }
    switch (expression->op) {
        // TODO: Handle unsigned/signed distinction in division and mod
        case Plus: return theBuilder->CreateAdd(left, right, "addtmp");
        case Minus: return theBuilder->CreateSub(left, right, "subtmp");
        case Mul: return theBuilder->CreateMul(left, right, "multmp");
        case Div: return theBuilder->CreateSDiv(left, right, "divtmp");
        case FloorDiv:
            NOT_IMPLEMENTED("Floor division is not implemented yet");
            return nullptr;  // Placeholder for future implementation
        case Mod: return theBuilder->CreateSRem(left, right, "modtmp");
        case Exp:
            NOT_IMPLEMENTED("Exponentiation is not implemented yet");
            return nullptr;  // Placeholder for future implementation
        case GreaterThan: return theBuilder->CreateICmpSGT(left, right, "cmptmp");
        case GreaterThanOrEqual: return theBuilder->CreateICmpSGE(left, right, "cmptmp");
        case LessThan: return theBuilder->CreateICmpSLT(left, right, "cmptmp");
        case LessThanOrEqual: return theBuilder->CreateICmpSLE(left, right, "cmptmp");
        case Equal: return theBuilder->CreateICmpEQ(left, right, "cmptmp");
        case NotEqual: return theBuilder->CreateICmpNE(left, right, "cmptmp");
        case BitAnd: return theBuilder->CreateAnd(left, right, "bitandtmp");
        case BitOr: return theBuilder->CreateOr(left, right, "bitorTmp");
        case BitXor: return theBuilder->CreateXor(left, right, "bitxortmp");
        case BitLShift: return theBuilder->CreateShl(left, right, "bitshltmp");
        case BitRShift:
            if (semantic::isSignedInt(expression->left->getType())) {
                return theBuilder->CreateAShr(left, right, "ashrtmp");
            }
            return theBuilder->CreateLShr(left, right, "lshrtmp");
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
    llvm::Value* left = visit(expression->left);
    llvm::Value* oldValue = theBuilder->CreateLoad(left->getType(), left, "loadtmp");
    if (semantic::isAnyInt(expression->left->getType())) {
        bool isSigned = semantic::isSignedInt(expression->left->getType());
        llvm::Value* one = llvm::ConstantInt::get(oldValue->getType(), 1, isSigned);
        llvm::Value* newValue;
        if (expression->op == lexer::TokenType::Inc) {
            newValue = theBuilder->CreateAdd(oldValue, one, "inc");
        } else {
            newValue = theBuilder->CreateSub(oldValue, one, "dec");
        }
        theBuilder->CreateStore(newValue, left);
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
        theBuilder->CreateStore(newValue, left);
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
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::TypeCastExpression* expression) -> exprvisit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}

}  // namespace codegen

}  // namespace Manganese