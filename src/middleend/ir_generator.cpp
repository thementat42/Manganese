#include <llvm/IR/Constants.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/Support/raw_os_ostream.h>

#include <core.hpp>
#include <format>
#include <frontend/ast.hpp>
#include <frontend/semantic.hpp>
#include <frontend/semantic/type_context.hpp>
#include <middleend/ir_generator.hpp>

namespace Manganese::codegen {

void IRGenerator::generate() noexcept {
    for (const parser::ParsedFile& file : files) {
        for (const ast::Statement* statement : file.program) { visit(statement); }
    }
}

void IRGenerator::dump(std::ostream& os) const {
    llvm::raw_os_ostream ros{os};
    module->print(ros, nullptr);
}

[[nodiscard]] llvm::IntegerType* IRGenerator::getLLVMIntegerType(const ast::NumberLiteralExpression* expression) const {
    const std::string_view lexeme = interner.get_view(expression->value);

    if (lexeme.ends_with("i8") || lexeme.ends_with("u8") || lexeme.ends_with("I8") || lexeme.ends_with("U8")) {
        return llvm::Type::getInt8Ty(*context);
    }
    if (lexeme.ends_with("i16") || lexeme.ends_with("u16") || lexeme.ends_with("I16") || lexeme.ends_with("U16")) {
        return llvm::Type::getInt16Ty(*context);
    }
    if (lexeme.ends_with("i32") || lexeme.ends_with("u32") || lexeme.ends_with("I32") || lexeme.ends_with("U32")) {
        return llvm::Type::getInt32Ty(*context);
    }
    if (lexeme.ends_with("i64") || lexeme.ends_with("u64") || lexeme.ends_with("I64") || lexeme.ends_with("U64")) {
        return llvm::Type::getInt64Ty(*context);
    }
    if (lexeme.ends_with("i128") || lexeme.ends_with("u128") || lexeme.ends_with("I128") || lexeme.ends_with("U128")) {
        return llvm::Type::getInt128Ty(*context);
    }
    using enum ast::PrimitiveType;
    auto p = expression->semanticType->primitiveType;
    switch (p) {
        case int8:
        case uint8: return llvm::Type::getInt8Ty(*context);
        case int16:
        case uint16: return llvm::Type::getInt16Ty(*context);
        case int32:
        case uint32: return llvm::Type::getInt32Ty(*context);
        case int64:
        case uint64: return llvm::Type::getInt64Ty(*context);
        case int128:
        case uint128: return llvm::Type::getInt128Ty(*context);
        default: break;
    }
    ASSERT_UNREACHABLE_FMT("Invalid integer primitive type '{}' in IRGenerator::visit(NumberLiteralExpression)",
                           ast::primitiveTypeToString(p));
}

[[nodiscard]] llvm::Value* IRGenerator::convertNumberToType(llvm::Value* val, const semantic::SemanticType* fromType,
                                                            const semantic::SemanticType* toType) {
    if (fromType == toType) { return val; }
    llvm::Type* toTypeLLVM = visit(toType);
    auto fromWidth = semantic::getPrimitiveInfo(fromType->primitiveType).bitWidth;
    auto toWidth = semantic::getPrimitiveInfo(toType->primitiveType).bitWidth;

    if (fromType->isInteger()) {
        if (toType->isFloat()) {
            // int to float
            return (fromType->isSignedInteger()) ? builder->CreateSIToFP(val, toTypeLLVM)
                                                 : builder->CreateUIToFP(val, toTypeLLVM);
        }

        // int to int

        // widening so extend (0 extend for unsigned, sign extend for signed)
        if (fromWidth < toWidth) {
            return fromType->isSignedInteger() ? builder->CreateSExt(val, toTypeLLVM)
                                               : builder->CreateZExt(val, toTypeLLVM);
        }

        // narrowing so truncate
        return builder->CreateTrunc(val, toTypeLLVM);
    }
    // float conversion
    if (toType->isInteger()) {
        // float to int
        return toType->isSignedInteger() ? builder->CreateFPToSI(val, toTypeLLVM)
                                         : builder->CreateFPToUI(val, toTypeLLVM);
    }
    // float to float
    return fromWidth > toWidth ? builder->CreateFPTrunc(val, toTypeLLVM) : builder->CreateFPExt(val, toTypeLLVM);
}

llvm::CmpInst::Predicate IRGenerator::getFloatPredicate(lexer::TokenType op) {
    using enum lexer::TokenType;
    using enum llvm::CmpInst::Predicate;
    if (op == GreaterThan) { return FCMP_UGT; }
    if (op == GreaterThanOrEqual) { return FCMP_UGE; }
    if (op == LessThan) { return FCMP_ULT; }
    if (op == LessThanOrEqual) { return FCMP_ULE; }
    if (op == Equal) { return FCMP_UEQ; }
    if (op == NotEqual) { return FCMP_UNE; }
    ASSERT_UNREACHABLE_FMT("Unknown binary comparison operator '{}' in IRGenerator::getFloatPredicate",
                           lexer::tokenTypeToString(op));
}
llvm::CmpInst::Predicate IRGenerator::getIntPredicate(lexer::TokenType op, bool isSigned) {
    using enum lexer::TokenType;
    using enum llvm::CmpInst::Predicate;
    if (op == GreaterThan) { return isSigned ? ICMP_SGT : ICMP_UGT; }
    if (op == GreaterThanOrEqual) { return isSigned ? ICMP_SGE : ICMP_UGE; }
    if (op == LessThan) { return isSigned ? ICMP_SLT : ICMP_ULT; }
    if (op == LessThanOrEqual) { return isSigned ? ICMP_SLE : ICMP_ULE; }
    if (op == Equal) { return ICMP_EQ; }
    if (op == NotEqual) { return ICMP_NE; }
    ASSERT_UNREACHABLE_FMT("Unknown binary comparison operator '{}' in IRGenerator::getIntPredicate",
                           lexer::tokenTypeToString(op));
}

[[nodiscard]] auto IRGenerator::getLValue(const ast::Expression* expr) -> exprvisit_t {
    if (expr->kind == ast::ExpressionKind::IdentifierExpression) {
        const auto* identifierExpression = static_cast<const ast::IdentifierExpression*>(expr);
        llvm::Value* ptr = namedValues[identifierExpression->name];
        if (ptr == nullptr) {
            ASSERT_UNREACHABLE_FMT("Variable '{}' was not flagged as undeclared during semantic analysis",
                                   interner.get_view(identifierExpression->name));
        }
        return ptr;
    }
    if (expr->kind == ast::ExpressionKind::PrefixExpression) {
        const auto* prefixExpression = static_cast<const ast::PrefixExpression*>(expr);
        if (prefixExpression->op == lexer::TokenType::Dereference) { return visit(prefixExpression->right); }
    }

    if (expr->kind == ast::ExpressionKind::IndexExpression) {
        const auto* indexExpression = static_cast<const ast::IndexExpression*>(expr);
        const semantic::SemanticType* varType = indexExpression->variable->semanticType;
        llvm::Value* indexVal = visit(indexExpression->index);

        if (varType->isSlice()) {
            llvm::Value* sliceDescPtr = getLValue(indexExpression->variable);  // Pointer to { ptr, i64 }
            llvm::Type* sliceDescLLVMType = visitTypeAsValue(varType);

            // Get pointer to the data pointer field (index 0 of the slice descriptor)
            llvm::Value* dataPtrField
                = builder->CreateStructGEP(sliceDescLLVMType, sliceDescPtr, 0, "slice_data_field_ptr");
            // Load the actual raw data pointer (ptr) stored in the slice descriptor
            llvm::Value* dataPtr = builder->CreateLoad(builder->getPtrTy(), dataPtrField, "slice_data_ptr");

            const auto* sliceType = static_cast<const semantic::Slice*>(varType);
            llvm::Type* elementType = visitTypeAsValue(sliceType->elementType);
            return builder->CreateInBoundsGEP(elementType, dataPtr, {indexVal}, "slice_element_ptr");
        }

        llvm::Value* arrayPtr = getLValue(indexExpression->variable);  // Get pointer to the array
        llvm::Type* arrayLlvmType = visitTypeAsValue(varType);
        return builder->CreateInBoundsGEP(arrayLlvmType, arrayPtr, {builder->getInt32(0), indexVal},
                                          "array_element_ptr");
    }

    if (expr->kind == ast::ExpressionKind::MemberAccessExpression) {
        const auto* accessExpression = static_cast<const ast::MemberAccessExpression*>(expr);
        llvm::Value* base = getLValue(accessExpression->object);
        llvm::Type* aggregateLLVMType = visit(accessExpression->object->semanticType);
        auto fieldIndex = static_cast<unsigned int>(accessExpression->fieldIndex);
        return builder->CreateStructGEP(aggregateLLVMType, base, fieldIndex, "field_ptr");
    }
    if (expr->kind == ast::ExpressionKind::ScopeResolutionExpression) {
        const auto* scopeExpression = static_cast<const ast::ScopeResolutionExpression*>(expr);
        llvm::Value* ptr = namedValues[scopeExpression->mangledName];
        if (ptr == nullptr) {
            ptr = builder->GetInsertBlock()->getModule()->getNamedGlobal(
                interner.get_view(scopeExpression->mangledName));
        }
        if (ptr == nullptr) {
            ASSERT_UNREACHABLE_FMT("Could not find LLVM value for mangled name '{}'",
                                   interner.get_view(scopeExpression->mangledName));
        }
        return ptr;
    }

    ASSERT_UNREACHABLE_FMT("Unknown lvalue expression '{}' in IRGenerator::getLValue", expr->toString(interner));
}

llvm::Value* IRGenerator::emitComparison(lexer::TokenType op, llvm::Value* left, llvm::Value* right,
                                         const semantic::SemanticType* semanticType) {
    using enum lexer::TokenType;

    if (semanticType != nullptr && semanticType->isString()) {
        auto* stringType = visit(semanticType);

        const llvm::FunctionCallee strcmpFn
            = module->getOrInsertFunction(MN_STRINGIFY(mn_strcmp), builder->getInt32Ty(), stringType, stringType);

        llvm::Value* cmpResult = builder->CreateCall(strcmpFn, {left, right}, "strcmp_tmp");
        llvm::Value* zero = builder->getInt32(0);

        llvm::CmpInst::Predicate pred;
        switch (op) {
            case Equal: pred = llvm::CmpInst::ICMP_EQ; break;
            case NotEqual: pred = llvm::CmpInst::ICMP_NE; break;
            case LessThan: pred = llvm::CmpInst::ICMP_SLT; break;
            case LessThanOrEqual: pred = llvm::CmpInst::ICMP_SLE; break;
            case GreaterThan: pred = llvm::CmpInst::ICMP_SGT; break;
            case GreaterThanOrEqual: pred = llvm::CmpInst::ICMP_SGE; break;
            default: llvm_unreachable("Invalid string comparison operator");
        }

        return builder->CreateICmp(pred, cmpResult, zero, "str_cmp_bool");
    }

    if (left->getType()->isFloatingPointTy()) {
        llvm::CmpInst::Predicate pred;
        switch (op) {
            case Equal: pred = llvm::CmpInst::FCMP_OEQ; break;
            case NotEqual: pred = llvm::CmpInst::FCMP_ONE; break;
            case LessThan: pred = llvm::CmpInst::FCMP_OLT; break;
            case LessThanOrEqual: pred = llvm::CmpInst::FCMP_OLE; break;
            case GreaterThan: pred = llvm::CmpInst::FCMP_OGT; break;
            case GreaterThanOrEqual: pred = llvm::CmpInst::FCMP_OGE; break;
            default: llvm_unreachable("Invalid float comparison operator");
        }
        return builder->CreateFCmp(pred, left, right, "fcmp_tmp");
    }

    const bool isSigned = semanticType != nullptr ? semanticType->isSignedInteger() : true;
    llvm::CmpInst::Predicate pred;
    switch (op) {
        case Equal: pred = llvm::CmpInst::ICMP_EQ; break;
        case NotEqual: pred = llvm::CmpInst::ICMP_NE; break;
        case LessThan: pred = isSigned ? llvm::CmpInst::ICMP_SLT : llvm::CmpInst::ICMP_ULT; break;
        case LessThanOrEqual: pred = isSigned ? llvm::CmpInst::ICMP_SLE : llvm::CmpInst::ICMP_ULE; break;
        case GreaterThan: pred = isSigned ? llvm::CmpInst::ICMP_SGT : llvm::CmpInst::ICMP_UGT; break;
        case GreaterThanOrEqual: pred = isSigned ? llvm::CmpInst::ICMP_SGE : llvm::CmpInst::ICMP_UGE; break;
        default: llvm_unreachable("Invalid integer comparison operator");
    }

    return builder->CreateICmp(pred, left, right, "icmp_tmp");
}

[[nodiscard]] llvm::Value* IRGenerator::emitTypeCast(llvm::Value* originalValue, const semantic::SemanticType* fromType,
                                                     const semantic::SemanticType* toType) {
    if (fromType == toType) { return originalValue; }

    if (fromType->isArray() && toType->isSlice()) {
        const auto* arraySemanticType = static_cast<const semantic::Array*>(fromType);
        const auto* sliceSemanticType = static_cast<const semantic::Slice*>(toType);

        llvm::Type* arrayLLVMType = visit(arraySemanticType);
        llvm::Type* sliceLLVMType = visit(sliceSemanticType);

        llvm::Value* arrayPtr = originalValue;
        if (!arrayPtr->getType()->isPointerTy()) {
            llvm::AllocaInst* tempAlloca = builder->CreateAlloca(arrayLLVMType, nullptr, "array_to_slice_temp");
            // clang-tidy complains about arguments maybe being swapped (this is a false positive)
            builder->CreateStore(arrayPtr, tempAlloca);
            arrayPtr = tempAlloca;
        }

        std::array<llvm::Value*, 2> indices = {builder->getInt32(0), builder->getInt32(0)};
        llvm::Value* elementPtr = builder->CreateInBoundsGEP(arrayLLVMType, arrayPtr, indices, "array_to_slice_ptr");
        llvm::Value* lengthVal = builder->getInt64(arraySemanticType->length);

        llvm::Value* sliceVal = llvm::Constant::getNullValue(sliceLLVMType);

        // clang-tidy complains about arguments maybe being swapped (this is a false positive)
        sliceVal = builder->CreateInsertValue(sliceVal, elementPtr, 0, "slice_ptr_ins");
        sliceVal = builder->CreateInsertValue(sliceVal, lengthVal, 1, "slice_len_ins");

        return sliceVal;
    }

    const llvm::Type* sourceTypeLLVM = visit(fromType);
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

}  // namespace Manganese::codegen