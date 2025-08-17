#include <llvm/IR/Value.h>

#include <global_macros.hpp>
#include <middleend/codegen/codegen_base.hpp>
#include <type_traits>

namespace Manganese {

namespace codegen {

auto IRGenerator::visit(ast::AggregateInstantiationExpression* expression) -> visit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::ArrayLiteralExpression* expression) -> visit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::AssignmentExpression* expression) -> visit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::BinaryExpression* expression) -> visit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::BoolLiteralExpression* expression) -> visit_t {
    // Note: LLVM doesn't have a dedicated boolean type per se (it represents boolean values as i1)
    return llvm::ConstantInt::get(llvm::Type::getInt1Ty(*theContext), expression->value);
}
auto IRGenerator::visit(ast::CharLiteralExpression* expression) -> visit_t {
    // Since we're using char32_t for character literals
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(*theContext), static_cast<uint32_t>(expression->value),
                                  /*isSigned=*/false);
}
auto IRGenerator::visit(ast::FunctionCallExpression* expression) -> visit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::GenericExpression* expression) -> visit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::IdentifierExpression* expression) -> visit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::IndexExpression* expression) -> visit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::MemberAccessExpression* expression) -> visit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::NumberLiteralExpression* expression) -> visit_t {
    auto numberVisitor = [this](auto&& arg) -> llvm::Constant* {
        llvm::LLVMContext& ctxt = *theContext;
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_integral_v<T> && std::is_signed_v<T>) {
            // Note: llvm::ConstantInt::get expects a uint64_t for the value, so we cast it accordingly
            // This does not change the actual value since LLVM will interpret the bits as a signed integer (from the
            // isSigned parameter)

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
auto IRGenerator::visit(ast::PostfixExpression* expression) -> visit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::PrefixExpression* expression) -> visit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::ScopeResolutionExpression* expression) -> visit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::StringLiteralExpression* expression) -> visit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}
auto IRGenerator::visit(ast::TypeCastExpression* expression) -> visit_t {
    DISCARD(expression);
    NOT_IMPLEMENTED("Codegen is not available yet");
}

}  // namespace codegen

}  // namespace Manganese