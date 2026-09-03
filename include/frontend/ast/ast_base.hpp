#ifndef MANGANESE_INCLUDE_FRONTEND_AST_AST_BASE_HPP
#define MANGANESE_INCLUDE_FRONTEND_AST_AST_BASE_HPP

#include <core.hpp>
#include <cstddef>
#include <cstdint>
#include <mnstl/fold_result.hxx>
#include <ostream>
#include <string>
#include <string_view>
#include <utils/target_info.hpp>
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

enum class StatementKind : std::uint8_t {
#define STMT(name) name,
#define EXPR(name)
#define TYPE(name)
#include <frontend/ast/ast.def>
#undef STMT
#undef EXPR
#undef TYPE
};

enum class ExpressionKind : std::uint8_t {
#define STMT(name)
#define EXPR(name) name,
#define TYPE(name)
#include <frontend/ast/ast.def>
#undef STMT
#undef EXPR
#undef TYPE
};

enum class TypeKind : std::uint8_t {
#define STMT(name)
#define EXPR(name)
#define TYPE(name) name,
#include <frontend/ast/ast.def>
#undef STMT
#undef EXPR
#undef TYPE
};

using Block = std::vector<Statement*>;

enum class PrimitiveType : std::uint8_t {
    int8,
    uint8,
    int16,
    uint16,
    int32,
    uint32,
    int64,
    uint64,
    int128,
    uint128,
    floata32,
    float64,
    character,
    string,
    boolean,
    not_primitive,
};

std::string_view primitiveTypeToString(PrimitiveType prim);

enum class Visibility : std::uint8_t {
    Public,
    Private,
};

constexpr const char* visibilityToString(Visibility visibility) noexcept {
    return visibility == Visibility::Public ? "public" : "private";
}

struct ASTNode {
    std::size_t line = 0, column = 0;
    ASTNode() noexcept = default;
    virtual ~ASTNode() noexcept = default;

    virtual std::string toString(std::size_t indent = 0) const = 0;

#if MN_DEBUG
    virtual void dump(std::ostream& os, std::size_t indentDepth = 0) const = 0;
#endif  // MN_DEBUG
};

struct Statement : public ASTNode {
    const StatementKind kind;

    constexpr bool isPoisoned() const noexcept { return kind == StatementKind::PoisonedStatement; };

   protected:
    explicit Statement(StatementKind _kind) noexcept : kind(_kind) {}
};

struct Expression : public ASTNode {
    const ExpressionKind kind;
    const semantic::SemanticType* semanticType = nullptr;

    virtual mnstl::fold_result_t fold(const TargetInfo&) const NOEXCEPT_IF_RELEASE { return mnstl::fold_result_t{}; }
    constexpr bool isPoisoned() const noexcept { return kind == ExpressionKind::PoisonedExpression; };


   protected:
    explicit Expression(ExpressionKind _kind) noexcept : kind(_kind) {}
};

struct Type : public ASTNode {
    const TypeKind kind;
    const PrimitiveType primitiveType;
    const semantic::SemanticType* semanticType = nullptr;

    constexpr bool isPoisoned() const noexcept { return kind == TypeKind::PoisonedType; };

   protected:
    explicit Type(TypeKind _kind, PrimitiveType _primitiveType = PrimitiveType::not_primitive) noexcept :
        kind(_kind), primitiveType(_primitiveType) {}
};

}  // namespace ast
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_AST_BASE_HPP