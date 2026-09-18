#include <llvm/IR/Constants.h>

#include <core.hpp>
#include <frontend/ast.hpp>
#include <frontend/semantic.hpp>
#include <middleend/codegen/ir_generator.hpp>

namespace Manganese::codegen {

[[nodiscard]] llvm::IntegerType* IRGenerator::getLLVMIntegerType(const ast::NumberLiteralExpression* expression,
                                                        std::string_view lexeme) const {
    if (lexeme.ends_with("i8") || lexeme.ends_with("u8") || lexeme.ends_with("I8") || lexeme.ends_with("U8")) {
        lexeme.remove_suffix(2);
        return llvm::Type::getInt8Ty(*context);
    }
    if (lexeme.ends_with("i16") || lexeme.ends_with("u16") || lexeme.ends_with("I16") || lexeme.ends_with("U16")) {
        lexeme.remove_suffix(3);
        return llvm::Type::getInt16Ty(*context);
    }
    if (lexeme.ends_with("i32") || lexeme.ends_with("u32") || lexeme.ends_with("I32") || lexeme.ends_with("U32")) {
        lexeme.remove_suffix(3);
        return llvm::Type::getInt32Ty(*context);
    }
    if (lexeme.ends_with("i64") || lexeme.ends_with("u64") || lexeme.ends_with("I64") || lexeme.ends_with("U64")) {
        lexeme.remove_suffix(3);
        return llvm::Type::getInt64Ty(*context);
    }
    if (lexeme.ends_with("i128") || lexeme.ends_with("u128") || lexeme.ends_with("I128") || lexeme.ends_with("U128")) {
        lexeme.remove_suffix(4);
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
    ASSERT_UNREACHABLE(std::format("Invalid integer primitive type '{}' in IRGenerator::visit(NumberLiteralExpression)",
                                   ast::primitiveTypeToString(p)));
}

} // namespace Manganese::codegen