/**
 * @file ast_base.h
 * @brief Defines the base classes and utilities for the Abstract Syntax Tree (AST) in the Manganese frontend.
 *
 * This header provides the foundational ASTNode class and its main derivatives: Type, Expression, and Statement.
 * It also defines common type aliases for AST node pointers and blocks, as well as macros to facilitate
 * overriding methods and declaring parser friendships.
 *
 * @see ast_expressions.h
 * @see ast_statements.h
 * @see ast_types.h
 */
/**
 *
 */

#ifndef MANGANESE_INCLUDE_FRONTEND_AST_AST_BASE_H
#define MANGANESE_INCLUDE_FRONTEND_AST_AST_BASE_H

#include <frontend/lexer.h>
#include <global_macros.h>
#include <utils/stox.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#if DEBUG
#define OVERRIDE_DUMP_METHOD void dump(std::ostream& os, int indent = 0) const override;  // Makes overriding dump() less cumbersome to type
#else
#define OVERRIDE_DUMP_METHOD  // Don't dump in non-debug builds
#endif

#define OVERRIDE_TO_STRING std::string toString() const override;  // Makes overriding toString() less cumbersome to type

#define __NODE_OVERRIDES \
    OVERRIDE_TO_STRING   \
    OVERRIDE_DUMP_METHOD

#define __FRIEND_DECLS     \
    friend parser::Parser; \
    friend semantic::SemanticAnalyzer;

/**
 * AST_STANDARD_INTERFACE - Common interface for all nodes
 * Combines required methods overrides (toString/dump)
 * and friend declarations (the parser and semantic analyzer) for access to protected members
 */
#define AST_STANDARD_INTERFACE __NODE_OVERRIDES __FRIEND_DECLS

namespace Manganese {

namespace parser {
class Parser;
}  // namespace parser

namespace semantic {
class SemanticAnalyzer;
}  // namespace semantic

namespace ast {
class Expression;
class Statement;
class Type;
using ExpressionPtr_t = std::unique_ptr<Expression>;
using StatementPtr_t = std::unique_ptr<Statement>;
using TypePtr_t = std::unique_ptr<Type>;
using Block = std::vector<StatementPtr_t>;

enum class ExpressionKind;
enum class StatementKind;
enum class TypeKind;

enum class Visibility : char {
    Public = 0,
    ReadOnly = 1,
    Private = 2,
};

class ASTNode {
   protected:
    size_t line = 0, column = 0;

   public:
    virtual ~ASTNode() noexcept = default;

    virtual std::string toString() const = 0;

#if DEBUG
    /**
     * @brief Dump the AST node to an output stream
     * @param os The output stream to dump to
     * @param indent The indentation level for pretty-printing
     */
    virtual void dump(std::ostream& os, int indent = 0) const = 0;
#endif  // DEBUG

    size_t getLine() const { return line; }
    size_t getColumn() const { return column; }

    friend parser::Parser;
};

class Expression : public ASTNode {
   protected:
    TypePtr_t computedType;

   public:
    virtual ~Expression() noexcept = default;
    virtual Type* getType() const noexcept {
        return computedType.get();
    };
    virtual void setType(TypePtr_t type) noexcept {
        computedType = std::move(type);
    }
    virtual ExpressionKind kind() const noexcept = 0;
};

class Statement : public ASTNode {
   public:
    virtual ~Statement() noexcept = default;
    virtual StatementKind kind() const noexcept = 0;
};

class Type : public ASTNode {
   public:
    virtual ~Type() noexcept = default;
    virtual TypeKind kind() const noexcept = 0;
};

}  // namespace ast

// TODO: Add line and column setting methods to the derived classes
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_AST_BASE_H