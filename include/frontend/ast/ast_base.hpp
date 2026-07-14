#ifndef MANGANESE_INCLUDE_FRONTEND_AST_AST_BASE_HPP
#define MANGANESE_INCLUDE_FRONTEND_AST_AST_BASE_HPP

#include <core.hpp>
#include <frontend/lexer.hpp>
#include <mnstl/fold_result.hxx>
#include <string>
#include <utils/type_names.hpp>
#include <vector>

#if MN_DEBUG
#define MN_AST_DUMP void dump(std::ostream& os, size_t indent = 0) const override;
#else
#define MN_AST_DUMP
#endif

#define MN_AST_STANDARD_INTERFACE          \
    std::string toString() const override; \
    MN_AST_DUMP

namespace Manganese {

namespace semantic {

struct SemanticType;

}  // namespace semantic

namespace ast {
struct Expression;
struct Statement;
struct Type;

using Block = std::vector<Statement*>;

enum class ExpressionKind : uint8_t;
enum class StatementKind : uint8_t;
enum class TypeKind : uint8_t;

enum class PrimitiveType_t : uint8_t {
    i8,
    u8,
    i16,
    u16,
    i32,
    u32,
    i64,
    u64,
    i128,
    u128,
    f32,
    f64,
    character,
    str,
    boolean,
    not_primitive = 0xFF,
};

enum class Visibility : uint8_t {
    Public = 0,
    Private = 2,
};

struct ASTNode {
   protected:
    size_t line = 0, column = 0;

   public:
    constexpr ASTNode() noexcept = default;

    ASTNode(const ASTNode&) = delete;
    ASTNode& operator=(const ASTNode&) = delete;
    ASTNode(ASTNode&&) = delete;
    ASTNode& operator=(ASTNode&&) = delete;

    virtual ~ASTNode() noexcept = default;

    virtual std::string toString() const = 0;

    constexpr inline size_t getLine() const noexcept { return line; }
    constexpr inline size_t getColumn() const noexcept { return column; }

#if MN_DEBUG
    virtual void dump(std::ostream& os, size_t indentDepth = 0) const = 0;
#endif  // MN_DEBUG
};

struct Expression : public ASTNode {
    const ExpressionKind kind;
    const semantic::SemanticType* semanticType = nullptr;

    virtual ~Expression() noexcept = default;
    virtual mnstl::fold_result_t fold() const noexcept { return mnstl::fold_result_t{}; }

   protected:
    constexpr explicit Expression(ExpressionKind kind_) noexcept : kind(kind_) {}
};

struct Statement : public ASTNode {
    const StatementKind kind;

    virtual ~Statement() noexcept = default;

   protected:
    constexpr explicit Statement(StatementKind kind_) noexcept : kind(kind_) {}
};

struct Type : public ASTNode {
    const TypeKind kind;
    const PrimitiveType_t primitiveType;

    virtual ~Type() noexcept = default;

   protected:
    constexpr explicit Type(TypeKind kind_, PrimitiveType_t primitiveType_ = PrimitiveType_t::not_primitive) noexcept :
        kind(kind_), primitiveType(primitiveType_) {}
};

constexpr const char* visibilityToString(Visibility visibility) noexcept {
    return visibility == Visibility::Public ? "public" : "private";
}

/**
 * @param fallback The fallback string representation if the expression is a nullptr
 */
inline std::string toStringOr(const Expression* expression, const char* fallback = "unknown expression") {
    return expression ? expression->toString() : fallback;
}

/**
 * @param fallback The fallback string representation if the statement is a nullptr
 */
inline std::string toStringOr(const Statement* statement, const char* fallback = "unknown statement") {
    return statement ? statement->toString() : fallback;
}

/**
 * @param fallback The fallback string representation if the type is a nullptr
 */
inline std::string toStringOr(const Type* type, const char* fallback = "no type") {
    return type ? type->toString() : fallback;
}

constexpr std::string_view primitiveTypeToString(PrimitiveType_t prim) {
    switch (prim) {
        case PrimitiveType_t::not_primitive: return "not primitive";
        case PrimitiveType_t::i8: return int8_str;
        case PrimitiveType_t::i16: return int16_str;
        case PrimitiveType_t::i32: return int32_str;
        case PrimitiveType_t::i64: return int64_str;
        case PrimitiveType_t::i128: return int128_str;
        case PrimitiveType_t::u8: return uint8_str;
        case PrimitiveType_t::u16: return uint16_str;
        case PrimitiveType_t::u32: return uint32_str;
        case PrimitiveType_t::u64: return uint64_str;
        case PrimitiveType_t::u128: return uint128_str;
        case PrimitiveType_t::f32: return float32_str;
        case PrimitiveType_t::f64: return float64_str;
        case PrimitiveType_t::character: return char_str;
        case PrimitiveType_t::str: return string_str;
        case PrimitiveType_t::boolean: return bool_str;
        default: ASSERT_UNREACHABLE("Invalid primitive type");
    }
}

}  // namespace ast
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_AST_BASE_HPP