#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include <core.hpp>
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

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::ArrayLiteralExpression* expression) -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::AssignmentExpression* expression) -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::BinaryExpression* expression) -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::BoolLiteralExpression* expression) -> exprvisit_t {
    return llvm::ConstantInt::getBool(*context, expression->value);
}

[[nodiscard]] auto IRGenerator::visit(const ast::CharLiteralExpression* expression) -> exprvisit_t {
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), static_cast<std::uint64_t>(expression->value));
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::FunctionCallExpression* expression) -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::GenericInstantiationExpression* expression)
    -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::IdentifierExpression* expression) -> exprvisit_t {
    return nullptr;
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

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::PostfixExpression* expression) -> exprvisit_t {
    return nullptr;
}

[[nodiscard]] auto IRGenerator::visit([[maybe_unused]] const ast::PrefixExpression* expression) -> exprvisit_t {
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
    return builder->CreateGlobalString(strRef, ".str");
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