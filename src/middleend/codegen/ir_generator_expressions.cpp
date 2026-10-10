#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/BasicBlock.h>
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
#include <runtime/string.hpp>

namespace Manganese::codegen {

[[nodiscard]] auto IRGenerator::visit(const ast::AggregateInstantiationExpression* expression) -> exprvisit_t {
    auto* aggregateType = llvm::cast<llvm::StructType>(visit(expression->semanticType));
    llvm::Value* alloca = builder->CreateAlloca(aggregateType, nullptr, "aggregate_instantiation");

    for (unsigned i = 0; i < expression->fields.size; ++i) {
        llvm::Value* fieldValue = visit(expression->fields[i].value);
        llvm::Value* fieldPointer = builder->CreateStructGEP(aggregateType, alloca, i, "field_gep");
        builder->CreateStore(fieldValue, fieldPointer);
    }
    return alloca;
}

[[nodiscard]] auto IRGenerator::visit(const ast::AggregateLiteralExpression* expression) -> exprvisit_t {
    auto* aggregateType = llvm::cast<llvm::StructType>(visit(expression->semanticType));
    llvm::Value* alloca = builder->CreateAlloca(aggregateType, nullptr, "anonymous_aggregate_instantiation");

    for (unsigned i = 0; i < expression->elements.size; ++i) {
        llvm::Value* fieldValue = visit(expression->elements[i]);
        llvm::Value* fieldPointer = builder->CreateStructGEP(aggregateType, alloca, i, "field_gep");
        builder->CreateStore(fieldValue, fieldPointer);
    }
    return alloca;
}

[[nodiscard]] auto IRGenerator::visit(const ast::AlignofExpression* expression) -> exprvisit_t {
    // Note: no codegen for the nested expression occurs
    llvm::IntegerType* sizeType = module->getDataLayout().getIntPtrType(*context);
    return llvm::ConstantInt::get(sizeType, expression->semanticType->alignment(targetInfo));
}

[[nodiscard]] auto IRGenerator::visit(const ast::ArrayLiteralExpression* expression) -> exprvisit_t {
    llvm::Type* elementType = visitTypeAsValue(expression->semanticType);
    const std::size_t length = expression->elements.size;
    llvm::ArrayType* arrayType = llvm::ArrayType::get(elementType, length);
    llvm::AllocaInst* arrayAlloca = builder->CreateAlloca(arrayType, nullptr, "array_literal");
    for (std::size_t i = 0; i < length; ++i) {
        llvm::Value* elementValue = visit(expression->elements[i]);
        std::array<llvm::Value*, 2> indices = {builder->getInt32(0), builder->getInt64(i)};
        llvm::Value* elementPointer
            = builder->CreateInBoundsGEP(arrayType, arrayAlloca, indices, "array_literal_element");
        builder->CreateStore(elementValue, elementPointer);
    }
    return arrayAlloca;
}

[[nodiscard]] auto IRGenerator::visit(const ast::AssignmentExpression* expression) -> exprvisit_t {
    llvm::Value* assigneePointer = getLValue(expression->assignee);
    llvm::Value* newValue = visit(expression->value);

    const semantic::SemanticType* type = expression->semanticType;

    if ((type->isAggregate() || type->isArray()) && newValue->getType()->isPointerTy()) {
        DISCARD(visit(type));
        std::size_t sizeInBytes = type->size(targetInfo);
        std::size_t alignment = type->alignment(targetInfo);

        builder->CreateMemCpy(assigneePointer, llvm::MaybeAlign(alignment), newValue, llvm::MaybeAlign(alignment),
                              sizeInBytes);
        return newValue;
    }

    builder->CreateStore(newValue, assigneePointer);
    return newValue;
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
        case Plus: {
            if (expression->semanticType->isString()) {
                auto* stringType = visit(expression->semanticType);
                llvm::FunctionCallee concatenationFunction
                    = module->getOrInsertFunction(MN_STRINGIFY(mn_strcat), stringType, stringType, stringType);
                return builder->CreateCall(concatenationFunction, {lhs, rhs}, "string_concat_tmp");
            }
            return doFloatOperation ? builder->CreateFAdd(lhs, rhs, "fadd_tmp")
                                    : builder->CreateAdd(lhs, rhs, "iadd_tmp");
        }

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
            if (expression->left->semanticType->isString()) {
                auto* stringType = visit(expression->left->semanticType);

                llvm::FunctionCallee strcmpFn = module->getOrInsertFunction(
                    MN_STRINGIFY(mn_strcmp), builder->getInt32Ty(), stringType, stringType);

                llvm::Value* cmpResult = builder->CreateCall(strcmpFn, {lhs, rhs}, "strcmp_tmp");
                llvm::Value* zero = builder->getInt32(0);

                llvm::CmpInst::Predicate pred;
                switch (expression->op) {
                    case Equal: pred = llvm::CmpInst::ICMP_EQ; break;
                    case NotEqual: pred = llvm::CmpInst::ICMP_NE; break;
                    case LessThan: pred = llvm::CmpInst::ICMP_SLT; break;
                    case LessThanOrEqual: pred = llvm::CmpInst::ICMP_SLE; break;
                    case GreaterThan: pred = llvm::CmpInst::ICMP_SGT; break;
                    case GreaterThanOrEqual: pred = llvm::CmpInst::ICMP_SGE; break;
                    default: ASSERT_UNREACHABLE("Invalid string comparison operator");
                }

                return builder->CreateICmp(pred, cmpResult, zero, "str_cmp_bool");
            }
            if (doFloatOperation) {
                const llvm::CmpInst::Predicate predicate = getFloatPredicate(expression->op);
                return builder->CreateFCmp(predicate, lhs, rhs, "fcmp_tmp");
            }
            const bool isSigned = commonType->isSignedInteger();
            const llvm::CmpInst::Predicate predicate = getIntPredicate(expression->op, isSigned);
            return builder->CreateICmp(predicate, lhs, rhs, "icmp_tmp");
        }

        case And:
        case Or: {
            const auto op = expression->op;
            llvm::Function* function = builder->GetInsertBlock()->getParent();
            llvm::Value* lhsValue = visit(expression->left);
            llvm::BasicBlock* lhsBlock = builder->GetInsertBlock();

            llvm::BasicBlock* rhsBlock = llvm::BasicBlock::Create(*context, op == And ? "and_rhs" : "or_rhs", function);
            llvm::BasicBlock* mergeBlock
                = llvm::BasicBlock::Create(*context, op == And ? "and_end" : "or_end", function);

            if (expression->op == And) {
                builder->CreateCondBr(/*Cond=*/lhsValue, /*True=*/rhsBlock, /*False=*/mergeBlock);
            } else {
                builder->CreateCondBr(/*Cond=*/lhsValue, /*True=*/mergeBlock, /*False=*/rhsBlock);
            }

            builder->SetInsertPoint(rhsBlock);
            llvm::Value* rhsValue = visit(expression->right);
            llvm::BasicBlock* rhsBlockEnd = builder->GetInsertBlock();
            builder->CreateBr(mergeBlock);

            builder->SetInsertPoint(mergeBlock);

            llvm::PHINode* phiNode
                = builder->CreatePHI(builder->getInt1Ty(), /*NumReservedValues=*/2, op == And ? "and_phi" : "or_phi");

            phiNode->addIncoming(builder->getInt1(op != And), lhsBlock);
            phiNode->addIncoming(rhsValue, rhsBlockEnd);
            return phiNode;
        }

        case BitAnd: return builder->CreateAnd(lhs, rhs, "bitand");
        case BitOr: return builder->CreateOr(lhs, rhs, "bitand");
        case BitXor: return builder->CreateXor(lhs, rhs, "bitxor");
        case BitLShift: return builder->CreateShl(lhs, rhs, "bitshl");
        case BitRShift: {
            return commonType->isSignedInteger() ? builder->CreateAShr(lhs, rhs, "arithmetic_shr_tmp")
                                                 : builder->CreateLShr(lhs, rhs, "logical_shr_tmp");
        }
        case MemberAccess:
        case ScopeResolution:
        default: break;
    }
    ASSERT_UNREACHABLE_FMT("Invalid binary operator {} in codegen", lexer::tokenTypeToString(expression->op));
}

[[nodiscard]] auto IRGenerator::visit(const ast::BoolLiteralExpression* expression) -> exprvisit_t {
    return llvm::ConstantInt::getBool(*context, expression->value);
}

[[nodiscard]] auto IRGenerator::visit(const ast::CharLiteralExpression* expression) -> exprvisit_t {
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), static_cast<std::uint64_t>(expression->value));
}

[[nodiscard]] auto IRGenerator::visit(const ast::FunctionCallExpression* expression) -> exprvisit_t {
    auto* functionType = llvm::cast<llvm::FunctionType>(visit(expression->callee->semanticType));

    std::vector<llvm::Value*> argumentValues;
    argumentValues.reserve(expression->arguments.size);

    for (unsigned int i = 0; i < expression->arguments.size; ++i) {
        const ast::Expression* argument = expression->arguments[i];
        llvm::Value* argumentValue = visit(argument);

        llvm::Type* expectedParameterType = functionType->getParamType(i);
        if (expectedParameterType->isStructTy() && argumentValue->getType()->isPointerTy()) {
            argumentValue = builder->CreateLoad(expectedParameterType, argumentValue, "argument_load_tmp");
        }
        argumentValues.push_back(argumentValue);
    }

    llvm::Value* calleeValue = nullptr;
    if (expression->callee->kind == ast::ExpressionKind::IdentifierExpression) {
        const auto* idExpr = static_cast<const ast::IdentifierExpression*>(expression->callee);
        if (idExpr->resolvedDeclaration != nullptr && idExpr->resolvedDeclaration->mangledName.has_value()) {
            calleeValue = builder->GetInsertBlock()->getModule()->getFunction(interner.get_view(*idExpr->resolvedDeclaration->mangledName));
        }
        if (calleeValue == nullptr) { calleeValue = builder->GetInsertBlock()->getModule()->getFunction(interner.get_view(idExpr->name)); }
    } else if (expression->callee->kind == ast::ExpressionKind::ScopeResolutionExpression) {
        const auto* scopeExpr = static_cast<const ast::ScopeResolutionExpression*>(expression->callee);
        calleeValue = builder->GetInsertBlock()->getModule()->getFunction(interner.get_view(scopeExpr->mangledName));
    } else {
        calleeValue = visit(expression->callee);
    }

    return builder->CreateCall(functionType, calleeValue, argumentValues, "call_tmp");
}

auto IRGenerator::visit(const ast::GenericInstantiationExpression* expression) -> exprvisit_t {
    const semantic::Symbol* symbol = analyzer.resolveScopeSymbol(expression->identifier);
    if (symbol == nullptr || symbol->node == nullptr) {
        ASSERT_UNREACHABLE_FMT(
            "Use of undeclared symbol '{}' in generic instantiation was not flagged as semantically invalid",
            expression->identifier->toString(interner));
    }

    semantic::InstantiationKey key{.declNode = symbol->node, .typeArgs = expression->semanticTypes};

    const auto* instantiationResult = analyzer.instantiationCache.find(key);
    if (instantiationResult == nullptr || instantiationResult->state != ResolutionStatus::Success) {
        ASSERT_UNREACHABLE_FMT("Generic instantiation not successfully resolved", expression->toString(interner));
    }

    const std::string_view mangledName = interner.get_view(instantiationResult->mangledName);

    if (symbol->kind == semantic::SymbolKind::Function) {
        llvm::Function* llvmFunc = module->getFunction(mangledName);
        if (llvmFunc == nullptr) {
            if (instantiationResult->clonedNode != nullptr) { visit(instantiationResult->clonedNode); }

            llvmFunc = module->getFunction(mangledName);
            if (llvmFunc == nullptr) {
                ASSERT_UNREACHABLE_FMT("Instantiated function '{}' was not generated in the LLVM module", mangledName);
            }
        }
        return llvmFunc;
    }

    if (symbol->kind == semantic::SymbolKind::Aggregate || symbol->kind == semantic::SymbolKind::GenericType) {
        llvm::StructType* llvmStruct = llvm::StructType::getTypeByName(*context, mangledName);
        if (llvmStruct == nullptr) {
            if (instantiationResult->clonedNode != nullptr) { visit(instantiationResult->clonedNode); }
            llvmStruct = llvm::StructType::getTypeByName(*context, mangledName);
            if (llvmStruct == nullptr) {
                ASSERT_UNREACHABLE_FMT("Instantiated aggregate '{}' was not generated in the LLVM module", mangledName);
            }
        }
        return nullptr;
    }

    ASSERT_UNREACHABLE_FMT("Generic instantiation {} was not flagged as semantically invalid", expression->toString(interner));
}

[[nodiscard]] auto IRGenerator::visit(const ast::IdentifierExpression* expression) -> exprvisit_t {
    if (expression->semanticType != nullptr && expression->semanticType->isFunction()) {
        llvm::Function* function = nullptr;
        if (expression->resolvedDeclaration != nullptr && expression->resolvedDeclaration->mangledName.has_value()) {
            function = module->getFunction(interner.get_view(*expression->resolvedDeclaration->mangledName));
        }
        if (function == nullptr) { function = module->getFunction(interner.get_view(expression->name)); }
        if (function == nullptr) {
            ASSERT_UNREACHABLE_FMT("Function '{}' could not be found in the LLVM module", interner.get_view(expression->name));
        }
        return function;
    }

    llvm::Value* value = nullptr;
    if ((expression->resolvedDeclaration != nullptr) && expression->resolvedDeclaration->mangledName.has_value()) {
        value = namedValues[*expression->resolvedDeclaration->mangledName];
        if (value == nullptr) {
            value = builder->GetInsertBlock()->getModule()->getNamedGlobal(interner.get_view(*expression->resolvedDeclaration->mangledName));
        }
    }

    if (value == nullptr) { value = namedValues[expression->name]; }
    if (value == nullptr) {
        ASSERT_UNREACHABLE_FMT("Variable '{}' was not flagged as undeclared during semantic analysis",
                               interner.get_view(expression->name));
    }
    llvm::Type* llvmType = visitTypeAsValue(expression->semanticType);
    return builder->CreateLoad(llvmType, value, std::format("load_val_of_{}", interner.get_view(expression->name)));
}

[[nodiscard]] auto IRGenerator::visit(const ast::IndexExpression* expression) -> exprvisit_t {
    llvm::Value* elementPointer = getLValue(expression);
    llvm::Type* elementLLVMType = visitTypeAsValue(expression->semanticType);
    if (expression->semanticType->isFunction()) { elementLLVMType = llvm::PointerType::get(*context, 0); }
    return builder->CreateLoad(elementLLVMType, elementPointer, "load_index_expr_val");
}

[[nodiscard]] auto IRGenerator::visit(const ast::MemberAccessExpression* expression) -> exprvisit_t {
    llvm::Value* fieldPointer = getLValue(expression);
    llvm::Type* fieldLLVMType = visitTypeAsValue(expression->semanticType);
    return builder->CreateLoad(fieldLLVMType, fieldPointer, "load_member_access_expr");
}

auto IRGenerator::visit(const ast::NumberLiteralExpression* expression) -> exprvisit_t {
    std::string_view lexeme = interner.get_view(expression->value);
    if (expression->isFloat) {
        if (lexeme.ends_with("f32") || lexeme.ends_with("F32")) {
            std::string mutableLexeme(lexeme);
            mutableLexeme.erase(mutableLexeme.size() - 3);
            return llvm::ConstantFP::get(llvm::Type::getFloatTy(*context), mutableLexeme);
        }
        if (lexeme.ends_with("f64") || lexeme.ends_with("F64")) {
            std::string mutableLexeme(lexeme);
            mutableLexeme.erase(mutableLexeme.size() - 3);
            return llvm::ConstantFP::get(llvm::Type::getDoubleTy(*context), mutableLexeme);
        }
        return llvm::ConstantFP::get(llvm::Type::getDoubleTy(*context), lexeme);
    }

    std::uint8_t radix = 10;
    std::string mutableLexeme(lexeme);

    if (mutableLexeme.starts_with("0x") || mutableLexeme.starts_with("0X")) {
        radix = 16;
        mutableLexeme.erase(0, 2);
    } else if (mutableLexeme.starts_with("0d") || mutableLexeme.starts_with("0D")) {
        radix = 10;
        mutableLexeme.erase(0, 2);
    } else if (mutableLexeme.starts_with("0o") || mutableLexeme.starts_with("0O")) {
        radix = 8;
        mutableLexeme.erase(0, 2);
    } else if (mutableLexeme.starts_with("0b") || mutableLexeme.starts_with("0B")) {
        radix = 2;
        mutableLexeme.erase(0, 2);
    }

    return llvm::ConstantInt::get(getLLVMIntegerType(expression), mutableLexeme, radix);
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
        ASSERT_UNREACHABLE_FMT("Unknown postfix operator '{}'", lexer::tokenTypeToString(expression->op));
    }
    builder->CreateStore(updatedValue, ptrToValue);
    return originalValue;
}

[[nodiscard]] auto IRGenerator::visit(const ast::PrefixExpression* expression) -> exprvisit_t {
    using enum lexer::TokenType;

    lexer::TokenType op = expression->op;
    const semantic::SemanticType* valueType = expression->right->semanticType;
    llvm::Type* valueTypeLLVM = visit(valueType);

    if (op == UnaryPlus) { return visit(expression->right); }
    if (op == UnaryMinus) {
        llvm::Value* value = visit(expression->right);
        return valueType->isFloat() ? builder->CreateFNeg(value, "fneg") : builder->CreateNeg(value, "ineg");
    }
    if (op == Not) {
        llvm::Value* value = visit(expression->right);
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
        return updatedValue;
    }
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit(const ast::ScopeResolutionExpression* expression) -> exprvisit_t {
    if (expression->semanticType->isEnum()) {
        const auto* enumType = static_cast<const semantic::Enum*>(expression->semanticType);
        const auto* identifierExpression = static_cast<const ast::IdentifierExpression*>(expression->element);
        auto val = enumType->getVariantValue(interner.get_view(identifierExpression->name));
        if (!val) { ASSERT_UNREACHABLE_FMT("Enum variant '{}' did not have a value set", interner.get_view(identifierExpression->name)); }
        auto* llvmIntType = llvm::cast<llvm::IntegerType>(visit(enumType->underlyingType));
        return llvm::ConstantInt::get(llvmIntType, static_cast<std::uint64_t>(*val),
                                      /*isSigned=*/enumType->underlyingType->isSignedInteger());
    }
    llvm::Value* scopeElementPointer = getLValue(expression);
    llvm::Type* scopeElementLLVMType = visitTypeAsValue(expression->semanticType);
    return builder->CreateLoad(scopeElementLLVMType, scopeElementPointer, "load_scope_res_val");
}

[[nodiscard]] auto IRGenerator::visit(const ast::SizeofExpression* expression) -> exprvisit_t {
    llvm::IntegerType* sizeType = module->getDataLayout().getIntPtrType(*context);
    return llvm::ConstantInt::get(sizeType, expression->semanticType->size(targetInfo));
}

[[nodiscard]] auto IRGenerator::visit(const ast::StringLiteralExpression* expression) -> exprvisit_t {
    std::string_view strView = interner.get_view(expression->value);
    llvm::StringRef strRef(strView.data(), strView.size());
    llvm::Constant* globalString = builder->CreateGlobalString(strRef, "str_literal");

    auto* stringType = llvm::cast<llvm::StructType>(visit(expression->semanticType));
    llvm::Constant* zero = builder->getInt32(0);
    std::vector<llvm::Constant*> indices = {zero, zero};
    llvm::Constant* dataPtr
        = llvm::ConstantExpr::getInBoundsGetElementPtr(globalString->getType(), globalString, indices);
    llvm::Constant* length = builder->getInt64(strView.size());
    return llvm::ConstantStruct::get(stringType, {dataPtr, length});
}

[[nodiscard]] auto IRGenerator::visit(const ast::TernaryExpression* expression) -> exprvisit_t {
    llvm::Function* function = builder->GetInsertBlock()->getParent();

    llvm::BasicBlock* trueBlock = llvm::BasicBlock::Create(*context, "ternary_true", function);
    llvm::BasicBlock* falseBlock = llvm::BasicBlock::Create(*context, "ternary_false", function);
    llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(*context, "ternary_end", function);

    llvm::Value* conditionValue = visit(expression->condition);
    builder->CreateCondBr(conditionValue, /*True=*/trueBlock, /*False=*/falseBlock);

    builder->SetInsertPoint(trueBlock);
    llvm::Value* trueBranchValue = visit(expression->ifTrue);
    builder->CreateBr(mergeBlock);
    llvm::BasicBlock* trueEndBlock = builder->GetInsertBlock();

    builder->SetInsertPoint(falseBlock);
    llvm::Value* falseBranchValue = visit(expression->ifFalse);
    builder->CreateBr(mergeBlock);
    llvm::BasicBlock* falseEndBlock = builder->GetInsertBlock();

    builder->SetInsertPoint(mergeBlock);

    llvm::Type* ternaryType = trueBranchValue->getType();
    llvm::PHINode* phiNode = builder->CreatePHI(ternaryType, 2, "ternary_phi");

    phiNode->addIncoming(trueBranchValue, trueEndBlock);
    phiNode->addIncoming(falseBranchValue, falseEndBlock);
    return phiNode;
}

[[nodiscard]] auto IRGenerator::visit(const ast::TypeCastExpression* expression) -> exprvisit_t {
    llvm::Value* originalValue = visit(expression->originalValue);

    const semantic::SemanticType* fromType = expression->originalValue->semanticType;
    const semantic::SemanticType* toType = expression->targetType->semanticType;

    if (fromType == toType) { return originalValue; }
    llvm::Type* sourceTypeLLVM = visit(fromType);
    llvm::Type* destTypeLLVM = visit(toType);

    if (sourceTypeLLVM->isIntOrIntVectorTy() && destTypeLLVM->isIntOrIntVectorTy()) {
        return builder->CreateIntCast(originalValue, destTypeLLVM, /*isSigned=*/fromType->isSignedInteger(),
                                      "int_to_int_cast");
    }
    if (sourceTypeLLVM->isFPOrFPVectorTy() && destTypeLLVM->isFPOrFPVectorTy()) {
        return builder->CreateFPCast(originalValue, destTypeLLVM, "fp_cast");
    }
    if (sourceTypeLLVM->isIntOrIntVectorTy() && destTypeLLVM->isFPOrFPVectorTy()) {
        return fromType->isSignedInteger() ? builder->CreateSIToFP(originalValue, destTypeLLVM, "si_to_fp_cast")
                                           : builder->CreateUIToFP(originalValue, destTypeLLVM, "ui_to_fp_cast");
    }
    if (sourceTypeLLVM->isFPOrFPVectorTy() && destTypeLLVM->isIntOrIntVectorTy()) {
        return toType->isSignedInteger() ? builder->CreateFPToSI(originalValue, destTypeLLVM, "fp_to_si_cast")
                                         : builder->CreateFPToUI(originalValue, destTypeLLVM, "fp_to_ui_cast");
    }

    ASSERT_UNREACHABLE_FMT("Unsupported LLVM type cast from '{}' to '{}'", fromType->toString(), toType->toString());
}

[[nodiscard]] auto IRGenerator::visit(const ast::UninitializedExpression* /*unused*/) -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit(const ast::PoisonedExpression* /*unused*/) -> exprvisit_t {
    ASSERT_UNREACHABLE("Poisoned expression was not flagged as semantically invalid during semantic analysis");
}

}  // namespace Manganese::codegen