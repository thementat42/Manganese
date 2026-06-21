/**
 * @file ast_base.hpp
 * @brief Defines the base classes and utilities for the Abstract Syntax Tree (AST) in the Manganese frontend.
 *
 * This header provides the foundational ASTNode class and its main derivatives: Type, Expression, and Statement.
 * It also defines common type aliases for AST node pointers and blocks
 *
 * @see ast_expressions.hpp
 * @see ast_statements.hpp
 * @see ast_types.hpp
 */
#ifndef MANGANESE_INCLUDE_FRONTEND_AST_AST_BASE_HPP
#define MANGANESE_INCLUDE_FRONTEND_AST_AST_BASE_HPP

#include <core.hpp>
#include <frontend/lexer.hpp>
#include <string>
#include <utils/type_names.hpp>
#include <vector>

#if MN_DEBUG
#define OVERRIDE_DUMP_METHOD_ \
    void dump(std::ostream& os, size_t indent = 0) const override;  // Makes overriding dump() less cumbersome to type
#else
#define OVERRIDE_DUMP_METHOD_  // Don't dump in non-debug builds
#endif

#define OVERRIDE_TO_STRING_ \
    std::string toString() const override;  // Makes overriding toString() less cumbersome to type

#define NODE_OVERRIDES_ \
    OVERRIDE_TO_STRING_ \
    OVERRIDE_DUMP_METHOD_

/**
 * Common interface functions for all nodes
 */
#define AST_STANDARD_INTERFACE NODE_OVERRIDES_

namespace Manganese {

namespace ast {
struct Expression;
struct Statement;
struct Type;
typedef std::vector<Statement*> Block;

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
    constexpr virtual ~ASTNode() noexcept = default;
    ASTNode(const ASTNode&) = delete;
    ASTNode& operator=(const ASTNode&) = delete;

    ASTNode(ASTNode&&) = default;
    ASTNode& operator=(ASTNode&&) = default;

    virtual std::string toString() const = 0;

#if MN_DEBUG
    /**
     * @brief Dump the AST node to an output stream
     * @param os The output stream to dump to
     * @param indent The indentation level for pretty-printing
     */
    virtual void dump(std::ostream& os, size_t indent = 0) const = 0;
#endif  // MN_DEBUG

    constexpr inline size_t getLine() const noexcept { return line; }
    constexpr inline size_t getColumn() const noexcept { return column; }
};

struct Expression : public ASTNode {
    ExpressionKind kind;

    virtual ~Expression() noexcept = default;

   protected:
    constexpr explicit Expression(ExpressionKind kind_) noexcept : kind(kind_) {}
};

struct Statement : public ASTNode {
    StatementKind kind;

    virtual ~Statement() noexcept = default;

   protected:
    constexpr explicit Statement(StatementKind kind_) noexcept : kind(kind_) {}
};

struct Type : public ASTNode {
    TypeKind kind;
    PrimitiveType_t primitiveType;

    virtual ~Type() noexcept = default;

   protected:
    constexpr explicit Type(TypeKind kind_, PrimitiveType_t primitiveType_ = PrimitiveType_t::not_primitive) noexcept :
        kind(kind_), primitiveType(primitiveType_) {}
};

constexpr const char* visibilityToString(Visibility visibility) noexcept {
    return visibility == Visibility::Public ? "public " : "private ";
}

/**
 * @brief A wrapper around Expression::toString which handles nullptrs with a fallback
 * @param fallback The fallback string representation if the expression is a nullptr
 */
inline std::string toStringOr(const Expression* expression, const char* fallback = "unknown expression") {
    return expression ? expression->toString() : fallback;
}

/**
 * @brief A wrapper around Statement::toString which handles nullptrs with a fallback
 * @param fallback The fallback string representation if the statement is a nullptr
 */
inline std::string toStringOr(const Statement* statement, const char* fallback = "unknown statement") {
    return statement ? statement->toString() : fallback;
}

/**
 * @brief A wrapper around Type::toString which handles nullptrs with a fallback
 * @param fallback The fallback string representation if the type is a nullptr
 */
inline std::string toStringOr(const Type* type, const char* fallback = "no type") {
    return type ? type->toString() : fallback;
}

}  // namespace ast
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_AST_BASE_HPP