#ifndef MANGANESE_INCLUDE_FRONTEND_SEMANTIC_PRIMITIVES_HPP
#define MANGANESE_INCLUDE_FRONTEND_SEMANTIC_PRIMITIVES_HPP 1

#include <frontend/ast.hpp>
#include <utils/type_names.hpp>

namespace Manganese {
namespace semantic {

struct primitives {
    const ast::TypeSPtr_t int8 = std::make_shared<ast::SymbolType>(int8_str);
    const ast::TypeSPtr_t int16 = std::make_shared<ast::SymbolType>(int16_str);
    const ast::TypeSPtr_t int32 = std::make_shared<ast::SymbolType>(int32_str);
    const ast::TypeSPtr_t int64 = std::make_shared<ast::SymbolType>(int64_str);
    const ast::TypeSPtr_t int128 = std::make_shared<ast::SymbolType>(int128_str);
    const ast::TypeSPtr_t uint8 = std::make_shared<ast::SymbolType>(uint8_str);
    const ast::TypeSPtr_t uint16 = std::make_shared<ast::SymbolType>(uint16_str);
    const ast::TypeSPtr_t uint32 = std::make_shared<ast::SymbolType>(uint32_str);
    const ast::TypeSPtr_t uint64 = std::make_shared<ast::SymbolType>(uint64_str);
    const ast::TypeSPtr_t uint128 = std::make_shared<ast::SymbolType>(uint128_str);
    const ast::TypeSPtr_t float32 = std::make_shared<ast::SymbolType>(float32_str);
    const ast::TypeSPtr_t float64 = std::make_shared<ast::SymbolType>(float64_str);
    const ast::TypeSPtr_t character = std::make_shared<ast::SymbolType>(char_str);
    const ast::TypeSPtr_t boolean = std::make_shared<ast::SymbolType>(bool_str);
    const ast::TypeSPtr_t string = std::make_shared<ast::SymbolType>(string_str);

    primitives() {
        int8->setPrimitiveType(ast::PrimitiveType_t::i8);
        int16->setPrimitiveType(ast::PrimitiveType_t::i16);
        int32->setPrimitiveType(ast::PrimitiveType_t::i32);
        int64->setPrimitiveType(ast::PrimitiveType_t::i64);
        int128->setPrimitiveType(ast::PrimitiveType_t::i128);
        uint8->setPrimitiveType(ast::PrimitiveType_t::ui8);
        uint16->setPrimitiveType(ast::PrimitiveType_t::ui16);
        uint32->setPrimitiveType(ast::PrimitiveType_t::ui32);
        uint64->setPrimitiveType(ast::PrimitiveType_t::ui64);
        uint128->setPrimitiveType(ast::PrimitiveType_t::ui128);
        float32->setPrimitiveType(ast::PrimitiveType_t::f32);
        float64->setPrimitiveType(ast::PrimitiveType_t::f64);
        character->setPrimitiveType(ast::PrimitiveType_t::character);
        boolean->setPrimitiveType(ast::PrimitiveType_t::boolean);
        string->setPrimitiveType(ast::PrimitiveType_t::str);
    }

    [[nodiscard]] const ast::TypeSPtr_t* fromLexeme(std::string_view lexeme) const noexcept {
        if (lexeme == int8_str) { return &int8; }
        if (lexeme == int16_str) { return &int16; }
        if (lexeme == int32_str) { return &int32; }
        if (lexeme == int64_str) { return &int64; }
        if (lexeme == int128_str) { return &int128; }
        if (lexeme == uint8_str) { return &uint8; }
        if (lexeme == uint16_str) { return &uint16; }
        if (lexeme == uint32_str) { return &uint32; }
        if (lexeme == uint64_str) { return &uint64; }
        if (lexeme == uint128_str) { return &uint128; }
        if (lexeme == float32_str) { return &float32; }
        if (lexeme == float64_str) { return &float64; }
        if (lexeme == char_str) { return &character; }
        if (lexeme == bool_str) { return &boolean; }
        if (lexeme == string_str) { return &string; }
        return nullptr;
    };
};

}  // namespace semantic

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_SEMANTIC_PRIMITIVES_HPP