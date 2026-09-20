#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include <array>
#include <core.hpp>
#include <format>
#include <frontend/ast.hpp>
#include <frontend/semantic.hpp>
#include <middleend/codegen/ir_generator.hpp>

namespace Manganese::codegen {

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::AggregateInstantiationExpression* expression)
    -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::AggregateLiteralExpression* expression)
    -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit(const ast::AlignofExpression* expression) -> exprvisit_t {
    // Note: no codegen for the nested expression occurs
    llvm::IntegerType* sizeType = module->getDataLayout().getIntPtrType(*context);
    return llvm::ConstantInt::get(sizeType, expression->semanticType->alignment(targetInfo));
}

[[nodiscard]] auto IRGenerator::visit(const ast::ArrayLiteralExpression* expression) -> exprvisit_t {
    llvm::Type* elementType = visit(expression->semanticType);
    const std::size_t length = expression->elements.size();
    llvm::ArrayType* arrayType = llvm::ArrayType::get(elementType, length);
    llvm::AllocaInst* arrayAlloca = builder->CreateAlloca(arrayType, nullptr, "array_literal");
    for (std::size_t i = 0; i < length; ++i) {
        llvm::Value* elementValue = visit(expression->elements[i]);
        // GEP gets a pointer offset, not a direct array element
        // so we want to get a pointer to the first element of the array, stay there, and then go forward to the ith
        // element
        // effectively &array[0][i]
        std::array<llvm::Value*, 2> indices = {builder->getInt32(0), builder->getInt64(i)};
        llvm::Value* elemPtr = builder->CreateInBoundsGEP(arrayType, arrayAlloca, indices, "array_literal_element");
        builder->CreateStore(elementValue, elemPtr);
    }
    return arrayAlloca;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::AssignmentExpression* expression) -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit(const ast::BinaryExpression* expression) -> exprvisit_t {
    const semantic::SemanticType* lhsType = expression->left->semanticType;
    const semantic::SemanticType* rhsType = expression->right->semanticType;

    const semantic::SemanticType* commonType = expression->semanticType;

    exprvisit_t lhs = visit(expression->left);
    exprvisit_t rhs = visit(expression->right);

    lhs = convertNumberToType(lhs, lhsType, commonType);
    rhs = convertNumberToType(rhs, rhsType, commonType);

    using enum lexer::TokenType;
    const bool doFloatOperation = commonType->isFloat();

    switch (expression->op) {
        case Plus:
            return doFloatOperation ? builder->CreateFAdd(lhs, rhs, "fadd_tmp")
                                    : builder->CreateAdd(lhs, rhs, "iadd_tmp");

        case Minus:
            return doFloatOperation ? builder->CreateFSub(lhs, rhs, "fsub_tmp")
                                    : builder->CreateSub(lhs, rhs, "isub_tmp");

        case Mul:
            return doFloatOperation ? builder->CreateFMul(lhs, rhs, "fmul_tmp")
                                    : builder->CreateMul(lhs, rhs, "imul_tmp");

        case Div: return builder->CreateFDiv(lhs, rhs, "truediv_tmp");

        case FloorDiv: {
            bool hasFloatOperand = lhsType->isFloat() || rhsType->isFloat();
            if (!hasFloatOperand) {
                return commonType->isSignedInteger() ? builder->CreateSDiv(lhs, rhs, "sfloordiv_tmp")
                                                     : builder->CreateUDiv(lhs, rhs, "ufloordiv_tmp");
            }
            // temporarily do the division in floating-point then truncate to an int
            llvm::Value* floatDivisionResult = builder->CreateFDiv(lhs, rhs, "floordiv_mixed_int_float_tmp_val");
            typevisit_t floatType = visit(commonType);
            llvm::Function* floorIntrinsic = llvm::Intrinsic::getDeclarationIfExists(
                builder->GetInsertBlock()->getModule(), llvm::Intrinsic::floor, {floatType});
            if (floorIntrinsic == nullptr) { ASSERT_UNREACHABLE("Could not find LLVM float intrinsic function"); }
            llvm::Value* floored = builder->CreateCall(floorIntrinsic, {floatDivisionResult}, "fdiv_val");
            llvm::Type* llvmTargetIntType = visit(commonType);

            return commonType->isSignedInteger() ? builder->CreateFPToSI(floored, llvmTargetIntType, "fdiv_fptosi_tmp")
                                                 : builder->CreateFPToUI(floored, llvmTargetIntType, "fdiv_fptoui_tmp");
        }
        case Mod: {
            if (doFloatOperation) { return builder->CreateFRem(lhs, rhs, "fmod_tmp"); }
            return commonType->isSignedInteger() ? builder->CreateSRem(lhs, rhs, "smod_tmp")
                                                 : builder->CreateURem(lhs, rhs, "umod_tmp");
        }
        case GreaterThan:
        case GreaterThanOrEqual:
        case LessThan:
        case LessThanOrEqual:
        case Equal:
        case NotEqual: {
            if (doFloatOperation) {
                const llvm::CmpInst::Predicate predicate = getFloatPredicate(expression->op);
                return builder->CreateFCmp(predicate, lhs, rhs, "fcmp_tmp");
            }
            const bool isSigned = commonType->isSignedInteger();
            const llvm::CmpInst::Predicate predicate = getIntPredicate(expression->op, isSigned);
            return builder->CreateICmp(predicate, lhs, rhs, "icmp_tmp");
        }
        case And:
        // TODO
        case Or:
            // TODO
        case BitAnd: return builder->CreateAnd(lhs, rhs, "bitand");

        case BitOr: return builder->CreateOr(lhs, rhs, "bitand");
        case BitXor: return builder->CreateXor(lhs, rhs, "bitxor");
        case BitLShift: return builder->CreateShl(lhs, rhs, "bitshl");
        case BitRShift: {
            // signed types need sign extension and unsigned types need 0 extension
            return commonType->isSignedInteger() ? builder->CreateAShr(lhs, rhs, "arithmetic_shr_tmp")
                                                 : builder->CreateLShr(lhs, rhs, "logical_shr_tmp");
        }
        case MemberAccess:
        case ScopeResolution:
        default: break;
    }
    ASSERT_UNREACHABLE(std::format("Invalid binary operator {} in codegen", lexer::tokenTypeToString(expression->op)));
}

[[nodiscard]] auto IRGenerator::visit(const ast::BoolLiteralExpression* expression) -> exprvisit_t {
    return llvm::ConstantInt::getBool(*context, expression->value);
}

[[nodiscard]] auto IRGenerator::visit(const ast::CharLiteralExpression* expression) -> exprvisit_t {
    // LLVM expects a uint64 as the value for an integer even if it's a smaller type
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), static_cast<std::uint64_t>(expression->value));
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::FunctionCallExpression* expression) -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::GenericInstantiationExpression* expression)
    -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit(const ast::IdentifierExpression* expression) -> exprvisit_t {
    llvm::Value* value = nullptr;
    if ((expression->resolvedDeclaration != nullptr) && !expression->resolvedDeclaration->mangledName.empty()) {
        value = namedValues[expression->resolvedDeclaration->mangledName];
        if (value == nullptr) {
            value
                = builder->GetInsertBlock()->getModule()->getNamedGlobal(expression->resolvedDeclaration->mangledName);
        }
    }

    if (value == nullptr) { value = namedValues[expression->name]; }
    if (value == nullptr) {
        ASSERT_UNREACHABLE(
            std::format("Variable '{}' was not flagged as undeclared during semantic analysis", expression->name));
    }
    llvm::Type* llvmType = visit(expression->semanticType);
    return builder->CreateLoad(llvmType, value, std::format("load_val_of_{}", expression->name));
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::IndexExpression* expression) -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::MemberAccessExpression* expression) -> exprvisit_t {
    return nullptr;
}

auto IRGenerator::visit(const ast::NumberLiteralExpression* expression) -> exprvisit_t {
    std::string_view lexeme = expression->value;
    if (expression->isFloat) {
        if (lexeme.ends_with("f32") || lexeme.ends_with("F32")) {
            lexeme.remove_suffix(3);
            return llvm::ConstantFP::get(llvm::Type::getFloatTy(*context), lexeme);
        }
        if (lexeme.ends_with("f64") || lexeme.ends_with("F64")) { lexeme.remove_suffix(3); }
        return llvm::ConstantFP::get(llvm::Type::getDoubleTy(*context), lexeme);
    }

    std::uint8_t radix = 10;

    // Figure out the radix and the type of the integer literal based on prefixes and suffixes for LLVM
    // Note: LLVM doesn't have a concept of signed vs unsigned integers, just sequences of N bits
    // instead other places need to tell it how to handle those bits
    // (e.g. 'udiv' vs 'idiv' for unsigned vs signed division)

    if (lexeme.starts_with("0x") || lexeme.starts_with("0X")) {
        radix = 16;
        lexeme.remove_prefix(2);
    } else if (lexeme.starts_with("0D") || lexeme.starts_with("0D")) {
        radix = 10;
        lexeme.remove_prefix(2);
    } else if (lexeme.starts_with("0o") || lexeme.starts_with("0O")) {
        radix = 8;
        lexeme.remove_prefix(2);
    } else if (lexeme.starts_with("0b") || lexeme.starts_with("0B")) {
        radix = 2;
        lexeme.remove_prefix(2);
    }

    return llvm::ConstantInt::get(getLLVMIntegerType(expression, lexeme), lexeme, radix);
}

[[nodiscard]] auto IRGenerator::visit(const ast::PostfixExpression* expression) -> exprvisit_t {
    using enum lexer::TokenType;
    llvm::Value* ptrToValue = getLValue(expression->left);
    const semantic::SemanticType* valueType = expression->left->semanticType;
    llvm::Type* valueTypeLLVM = visit(valueType);

    llvm::Value* originalValue = builder->CreateLoad(valueTypeLLVM, ptrToValue, "postfix_incdec_load");

    llvm::Value* updatedValue = nullptr;
    llvm::Value* one
        = valueType->isInteger() ? llvm::ConstantInt::get(valueTypeLLVM, 1) : llvm::ConstantFP::get(valueTypeLLVM, 1.0);

    if (expression->op == Inc) {
        updatedValue = valueType->isInteger() ? builder->CreateAdd(originalValue, one, "postfix_inc_iadd")
                                              : builder->CreateFAdd(originalValue, one, "postfix_inc_fadd");
    } else if (expression->op == Dec) {
        updatedValue = valueType->isInteger() ? builder->CreateSub(originalValue, one, "postfix_dec_isub")
                                              : builder->CreateFSub(originalValue, one, "postfix_dec_fsub");
    } else {
        ASSERT_UNREACHABLE(std::format("Unknown postfix operator '{}'", lexer::tokenTypeToString(expression->op)));
    }
    builder->CreateStore(updatedValue, ptrToValue);
    // postfix does the operation but returns the original value
    return originalValue;
}

[[nodiscard]] auto IRGenerator::visit(const ast::PrefixExpression* expression) -> exprvisit_t {
    using enum lexer::TokenType;

    lexer::TokenType op = expression->op;
    const semantic::SemanticType* valueType = expression->right->semanticType;
    llvm::Type* valueTypeLLVM = visit(valueType);

    if (op == UnaryPlus) { return visit(expression->right); /*+x is the same as x*/ }
    if (op == UnaryMinus) {
        llvm::Value* value = visit(expression->right);

        return valueType->isFloat() ? builder->CreateFNeg(value, "fneg") : builder->CreateNeg(value, "ineg");
    }
    if (op == Not) {
        llvm::Value* value = visit(expression->right);

        // expression's semantic type is bool
        value = convertNumberToType(value, valueType, expression->semanticType);
        return builder->CreateNot(value);
    }
    if (op == BitNot) {
        llvm::Value* value = visit(expression->right);
        return builder->CreateNot(value, "bitnot_tmp");
    }
    if (op == AddressOf) { return getLValue(expression->right); }
    if (op == Dereference) {
        llvm::Value* value = visit(expression->right);
        return builder->CreateLoad(valueTypeLLVM, value, "deref_load");
    }

    if (op == Inc || op == Dec) {
        llvm::Value* ptrToValue = getLValue(expression->right);
        llvm::Value* originalValue = builder->CreateLoad(valueTypeLLVM, ptrToValue, "postfix_incdec_load");

        llvm::Value* updatedValue = nullptr;
        llvm::Value* one = valueType->isInteger() ? llvm::ConstantInt::get(valueTypeLLVM, 1)
                                                  : llvm::ConstantFP::get(valueTypeLLVM, 1.0);

        if (expression->op == Inc) {
            updatedValue = valueType->isInteger() ? builder->CreateAdd(originalValue, one, "prefix_inc_iadd")
                                                  : builder->CreateFAdd(originalValue, one, "prefix_inc_fadd");
        } else if (expression->op == Dec) {
            updatedValue = valueType->isInteger() ? builder->CreateSub(originalValue, one, "prefix_dec_isub")
                                                  : builder->CreateFSub(originalValue, one, "prefix_dec_fsub");
        }
        builder->CreateStore(updatedValue, ptrToValue);
        // prefix does the operation and returns that value
        return updatedValue;
    }
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::ScopeResolutionExpression* expression)
    -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit(const ast::SizeofExpression* expression) -> exprvisit_t {
    // Note: no codegen for the nested expression occurs
    llvm::IntegerType* sizeType = module->getDataLayout().getIntPtrType(*context);
    return llvm::ConstantInt::get(sizeType, expression->semanticType->size(targetInfo));
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::StringLiteralExpression* expression) -> exprvisit_t {
    llvm::StringRef strRef(expression->value.data(), expression->value.size());
    return builder->CreateGlobalString(strRef, "str_literal");
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::TypeCastExpression* expression) -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::UninitializedExpression* expression) -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::PoisonedExpression* expression) -> exprvisit_t {
    return nullptr;
}

}  // namespace Manganese::codegen