#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Value.h>

#include <frontend/lexer.hpp>
#include <frontend/semantic.hpp>
#include <global_macros.hpp>
#include <middleend/codegen/codegen_base.hpp>

namespace Manganese {

namespace codegen {

auto IRGenerator::generateDivInstruction(ast::Expression* left, ast::Expression* right, exprvisit_t leftValue,
                                         exprvisit_t rightValue) -> exprvisit_t {
    if (semantic::isSInt(left->getType()) && semantic::isSInt(right->getType())) {
        return theBuilder->CreateSDiv(leftValue, rightValue, "ssdivtmp");
    }

    else if (semantic::isUInt(left->getType()) && semantic::isUInt(right->getType())) {
        return theBuilder->CreateUDiv(leftValue, rightValue, "usdivtmp");
    }

    else if (semantic::isSInt(left->getType()) && semantic::isUInt(right->getType())) {
        llvm::Value* rhsSigned = theBuilder->CreateSExt(rightValue, leftValue->getType(), "sudivsexttmp");
        return theBuilder->CreateSDiv(leftValue, rhsSigned, "sudivtmp");
    }

    else if (semantic::isUInt(left->getType()) && semantic::isSInt(right->getType())) {
        llvm::Value* lhsUnsigned = theBuilder->CreateZExt(leftValue, rightValue->getType(), "uudivzexttmp");
        return theBuilder->CreateUDiv(lhsUnsigned, rightValue, "uudivtmp");
    }

    else if (semantic::isFloat(left->getType()) && semantic::isFloat(right->getType())) {
        return theBuilder->CreateFDiv(leftValue, rightValue, "ffdivtmp");
    } else if (semantic::isFloat(left->getType()) && semantic::isInt(right->getType())) {
        llvm::Value* rhsFloat = theBuilder->CreateSIToFP(rightValue, llvm::Type::getFloatTy(*theContext));
        return theBuilder->CreateFDiv(leftValue, rhsFloat, "fidivtmp");
    } else {
        // left is int, right is float
        llvm::Value* lhsFloat = theBuilder->CreateSIToFP(leftValue, llvm::Type::getFloatTy(*theContext));
        return theBuilder->CreateFDiv(lhsFloat, rightValue, "ifdivtmp");
    }
}

auto IRGenerator::generateFloorDivInstruction(ast::Expression* left, ast::Expression* right, exprvisit_t leftValue,
                                              exprvisit_t rightValue) -> exprvisit_t {
    if (semantic::isUInt(left->getType()) && semantic::isUInt(right->getType())) {
        return theBuilder->CreateUDiv(leftValue, rightValue, "uudivtmp");
    } else if (semantic::isSInt(left->getType()) && semantic::isSInt(right->getType())) {
        llvm::Value* divResult = theBuilder->CreateSDiv(leftValue, rightValue, "ssdivtmp");
        // Adjust for truncation towards negative infinity
        llvm::Value* remainder = theBuilder->CreateSRem(leftValue, rightValue, "ssdivremtmp");
        llvm::Value* correction = theBuilder->CreateICmpSGT(remainder, llvm::ConstantInt::get(leftValue->getType(), 0));
        return theBuilder->CreateSelect(
            correction, theBuilder->CreateSub(divResult, llvm::ConstantInt::get(divResult->getType(), 1)), divResult);
    } else if (semantic::isSInt(left->getType()) && semantic::isUInt(right->getType())) {
        llvm::Value* rhsSigned = theBuilder->CreateSExt(rightValue, leftValue->getType(), "sudivsexttmp");
        return theBuilder->CreateSDiv(leftValue, rhsSigned, "sudivtmp");
    } else {
        // left is uint, right is sint
        llvm::Value* lhsUnsigned = theBuilder->CreateZExt(leftValue, rightValue->getType(), "usdivzexttmp");
        return theBuilder->CreateUDiv(lhsUnsigned, rightValue, "usdivtmp");
    }
}

auto IRGenerator::generateShortCircuitBinaryExpression(ast::BinaryExpression* expression) noexcept -> exprvisit_t {
    /*
    We want short-circuit behaviour for logical AND and OR operations.
    For AND:
        - If the left operand is true, we evaluate the right operand (since true && b = b).
        - If the left operand is false, we can short-circuit to false (since false && b = false).
    For OR:
        - If the left operand is true, we can short-circuit to true (since true || b = true).
        - If the left operand is false, we evaluate the right operand (since false || b = b).

    LLVM doesn't have built-in short-circuiting behaviour, so we can model this using branches
    Each side of the expression has a branch. If the left side can be short-circuited, we jump over the right
    side and go straight to setting the result to the short circuit value (true for OR and false for AND).
    If the left side can't be short circuited, we continue to evaluating the right branch, then setting the result
    to the value of the right hand side.
*/

    llvm::Value* left = visit(expression->left);
    llvm::Function* function = theBuilder->GetInsertBlock()->getParent();
    llvm::BasicBlock* leftBB = theBuilder->GetInsertBlock();
    llvm::BasicBlock* rightBB = llvm::BasicBlock::Create(*theContext, "right", function);
    llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(*theContext, "merge", function);

    bool isAnd = (expression->op == lexer::TokenType::And);
    /*
    if we arrived at the phi node from the left:
        for AND: the result is false (since left was false)
        for OR: the result is true (since left was true)
    If we arrived at the phi node from the right, the result is whatever the right evaluated to
    */
    llvm::BasicBlock* jumpIfLeftIsTrue = isAnd ? rightBB : mergeBB;
    llvm::BasicBlock* jumpIfLeftIsFalse = isAnd ? mergeBB : rightBB;
    llvm::ConstantInt* shortCircuitValue = isAnd ? theBuilder->getFalse() : theBuilder->getTrue();
    const char* phiNodeName = isAnd ? "andtmp" : "ortmp";

    theBuilder->CreateCondBr(left, jumpIfLeftIsTrue, jumpIfLeftIsFalse);

    theBuilder->SetInsertPoint(rightBB);
    llvm::Value* right = visit(expression->right);  // right needs to be inserted in rightBB
    theBuilder->CreateBr(mergeBB);  // After evaluating right, jump to merge

    theBuilder->SetInsertPoint(mergeBB);
    llvm::PHINode* phiNode = theBuilder->CreatePHI(llvm::Type::getInt1Ty(*theContext), 2, phiNodeName);
    phiNode->addIncoming(shortCircuitValue, leftBB);
    phiNode->addIncoming(right, rightBB);
    return phiNode;
}

}  // namespace codegen
}  // namespace Manganese