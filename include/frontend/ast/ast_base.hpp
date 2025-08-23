/**
 * @file ast_base.hpp
 * @brief Defines the base classes and utilities for the Abstract Syntax Tree (AST) in the Manganese frontend.
 *
 * This header provides the foundational ASTNode class and its main derivatives: Type, Expression, and Statement.
 * It also defines common type aliases for AST node pointers and blocks, as well as macros to facilitate
 * overriding methods and declaring parser friendships.
 *
 * @see ast_expressions.hpp
 * @see ast_statements.hpp
 * @see ast_types.hpp
 */
#ifndef MANGANESE_INCLUDE_FRONTEND_AST_AST_BASE_HPP
#define MANGANESE_INCLUDE_FRONTEND_AST_AST_BASE_HPP

#include <frontend/lexer.hpp>
#include <global_macros.hpp>
#include <memory>
#include <string>
#include <utils/number_utils.hpp>

#if DEBUG
#define __OVERRIDE_DUMP_METHOD \
    void dump(std::ostream& os, int indent = 0) const override;  // Makes overriding dump() less cumbersome to type
#else
#define __OVERRIDE_DUMP_METHOD  // Don't dump in non-debug builds
#endif

#define __OVERRIDE_TO_STRING \
    std::string toString() const override;  // Makes overriding toString() less cumbersome to type

#define __NODE_OVERRIDES \
    __OVERRIDE_TO_STRING \
    __OVERRIDE_DUMP_METHOD

/**
 * Common interface functions for all nodes
 * Combines required methods overrides (toString/dump)
 * and friend declarations (the parser and semantic analyzer) for access to protected members
 */
#define AST_STANDARD_INTERFACE __NODE_OVERRIDES

namespace Manganese {

namespace ast {
class Expression;
class Statement;
class Type;
using ExpressionUPtr_t = std::unique_ptr<Expression>;
using StatementUPtr_t = std::unique_ptr<Statement>;
using TypeSPtr_t = std::shared_ptr<Type>;

/*
TypeSPtr_t is a shared pointer since, in the semantic analysis phase, multiple AST nodes may refer to the same type.
e.g. in a variable declaration (let x = 1 + 2), both the variable and the assignment expression will have the same type
rather than constantly cloning types, it's easier to just have a shared pointer so that multiple nodes can refer to the
same type object. this reduces memory usage since the actual type is only allocated and stored once instead of multiple
redundant deep copies cloning types would also require cloning expressions (e.g. array types), which is even more
expensive, so shared_pointer is much more efficient
*/

enum class ExpressionKind;
enum class StatementKind;
enum class TypeKind;

enum class Visibility : char {
    Public = 0,
    ReadOnly = 1,
    Private = 2,
};

class ASTNode {
   public:
    size_t line = 0, column = 0;

    constexpr ASTNode() noexcept = default;
    constexpr virtual ~ASTNode() noexcept = default;
    ASTNode(const ASTNode&) = delete;
    ASTNode& operator=(const ASTNode&) = delete;

    ASTNode(ASTNode&&) = default;
    ASTNode& operator=(ASTNode&&) = default;

    virtual std::string toString() const = 0;

#if DEBUG
    /**
     * @brief Dump the AST node to an output stream
     * @param os The output stream to dump to
     * @param indent The indentation level for pretty-printing
     */
    virtual void dump(std::ostream& os, int indent = 0) const = 0;
#endif  // DEBUG

    constexpr inline size_t getLine() const noexcept { return line; } 
    constexpr inline size_t getColumn() const noexcept { return column; }
    constexpr inline void setLineColumn(size_t line_, size_t column_) noexcept {line = line_; column = column_;}
};

class Expression : public ASTNode {
   private:
    TypeSPtr_t computedType;

   public:
    virtual ~Expression() noexcept = default;
    inline Type* getType() const noexcept { return computedType.get(); };
    inline TypeSPtr_t getTypePtr() const noexcept { return computedType; }
    void setType(TypeSPtr_t type) noexcept { computedType = type; }
    constexpr virtual ExpressionKind kind() const noexcept = 0;
};

class Statement : public ASTNode {
   public:
    virtual ~Statement() noexcept = default;
    constexpr virtual StatementKind kind() const noexcept = 0;
};

class Type : public ASTNode {
   public:
    virtual ~Type() noexcept = default;
    constexpr virtual TypeKind kind() const noexcept = 0;
    virtual bool operator==(const Type& other) const noexcept = 0;
};

constexpr std::string visibilityToString(const Visibility& visibility) noexcept_if_release {
    switch (visibility) {
        case Visibility::Public: return "public ";
        case Visibility::ReadOnly: return "readonly ";
        case Visibility::Private: return "private ";
        default: ASSERT_UNREACHABLE("Invalid visibility");
    }
}

/**
 * @brief A wrapper around Expression::toString which handles nullptrs with a fallback
 * @param fallback The fallback string representation if the expression is a nullptr
 */
inline std::string toStringOr(const Expression* expression, const char* fallback = "unknown expression") {
    return expression ? expression->toString() : fallback;
}

/**
 * @brief A wrapper around Expression::toString which handles nullptrs with a fallback
 * @param fallback The fallback string representation if the expression is a nullptr
 */
inline std::string toStringOr(const ExpressionUPtr_t& expression, const char* fallback = "unknown expression") {
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
 * @brief A wrapper around Statement::toString which handles nullptrs with a fallback
 * @param fallback The fallback string representation if the statement is a nullptr
 */
inline std::string toStringOr(const StatementUPtr_t& statement, const char* fallback = "unknown statement") {
    return statement ? statement->toString() : fallback;
}

/**
 * @brief A wrapper around Type::toString which handles nullptrs with a fallback
 * @param fallback The fallback string representation if the type is a nullptr
 */
inline std::string toStringOr(const Type* type, const char* fallback = "no type") {
    return type ? type->toString() : fallback;
}

/**
 * @brief A wrapper around Type::toString which handles nullptrs with a fallback
 * @param fallback The fallback string representation if the type is a nullptr
 */
inline std::string toStringOr(const TypeSPtr_t& type, const char* fallback = "no type") {
    return type ? type->toString() : fallback;
}

}  // namespace ast
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_AST_BASE_HPP