#ifndef MANGANESE_INCLUDE_FRONTEND_AST_AST_BASE_HPP
#define MANGANESE_INCLUDE_FRONTEND_AST_AST_BASE_HPP

#include <core.hpp>
#include <cstddef>
#include <cstdint>
#include <mnstl/fold_result.hxx>
#include <ostream>
#include <string>
#include <utils/type_names.hpp>
#include <vector>

#if MN_DEBUG
#define MN_AST_DUMP void dump(std::ostream& os, std::size_t indent = 0) const override;
#else
#define MN_AST_DUMP
#endif

#define MN_AST_STANDARD_INTERFACE                                \
    std::string toString(std::size_t indent = 0) const override; \
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

enum class ExpressionKind : std::uint8_t;
enum class StatementKind : std::uint8_t;
enum class TypeKind : std::uint8_t;

enum class PrimitiveType_t : std::uint8_t {
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
    not_primitive,
};

enum class Visibility : std::uint8_t {
    Public = 0,
    Private = 2,
};

struct ASTNode {
    constexpr ASTNode() noexcept = default;
    virtual ~ASTNode() noexcept = default;

    virtual std::string toString(std::size_t indent = 0) const = 0;
    constexpr std::size_t getLine() const noexcept { return line; }
    constexpr std::size_t getColumn() const noexcept { return column; }

#if MN_DEBUG
    virtual void dump(std::ostream& os, std::size_t indentDepth = 0) const = 0;
#endif  // MN_DEBUG

   protected:
    std::size_t line = 0, column = 0;
};

struct Expression : public ASTNode {
    const ExpressionKind kind;
    const semantic::SemanticType* semanticType = nullptr;

    virtual ~Expression() noexcept = default;
    virtual mnstl::fold_result_t fold() const NOEXCEPT_IF_RELEASE { return mnstl::fold_result_t{}; }

   protected:
    constexpr explicit Expression(ExpressionKind _kind) noexcept : kind(_kind) {}
};

struct Statement : public ASTNode {
    const StatementKind kind;

    virtual ~Statement() noexcept = default;

   protected:
    constexpr explicit Statement(StatementKind _kind) noexcept : kind(_kind) {}
};

struct Type : public ASTNode {
    const TypeKind kind;
    const PrimitiveType_t primitiveType;
    const semantic::SemanticType* semanticType = nullptr;

    virtual ~Type() noexcept = default;

   protected:
    constexpr explicit Type(TypeKind _kind, PrimitiveType_t _primitiveType = PrimitiveType_t::not_primitive) noexcept :
        kind(_kind), primitiveType(_primitiveType) {}
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

std::string_view primitiveTypeToString(PrimitiveType_t prim);

}  // namespace ast
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_AST_BASE_HPP