#ifndef MANGANESE_INCLUDE_FRONTEND_SEMANTIC_ANALYZER_HPP
#define MANGANESE_INCLUDE_FRONTEND_SEMANTIC_ANALYZER_HPP

#include <core.hpp>
#include <cstddef>
#include <cstdint>
#include <format>
#include <frontend/ast.hpp>
#include <frontend/lexer.hpp>
#include <frontend/lexer/token_type.hpp>
#include <frontend/parser.hpp>
#include <frontend/semantic/symbol_table.hpp>
#include <frontend/semantic/type_context.hpp>
#include <mnstl/chunk_allocator.hxx>
#include <mnstl/enum_matches.hxx>
#include <string>
#include <utility>

namespace Manganese {
namespace semantic {

/**
 * Makes updating context flags easier
 * the destructor handles resetting a value rather than having to manually reset it everywhere
 * useful on branches that exit early
 */
template <class T>
struct [[nodiscard]] ContextGuard {
    T& ref;
    T old_val;

    ContextGuard(T& target, T new_val) : ref(target), old_val(target) { ref = new_val; }
    ~ContextGuard() { ref = old_val; }

    ContextGuard(const ContextGuard&) = delete;
    ContextGuard& operator=(const ContextGuard&) = delete;
};

using _analyzer_base_t = ast::Visitor<Result, Result, Result>;

class analyzer final : public _analyzer_base_t {
   private:
    SymbolTable symbolTable;
    TypeContext typeContext;
    parser::ParsedFile& parsedFile;

    struct {
        bool inFunction : 1 = false;
        uint8_t typeCastDepth = 0;
        uint8_t ifStatementDepth = 0;
        uint8_t switchStatementDepth = 0;
        uint8_t forLoopDepth = 0;
        uint8_t whileLoopDepth = 0;
        const SemanticType* currentFunctionReturnType = nullptr;
        const SemanticType* currentVariableDeclarationType = nullptr;
    } context;

    enum class Compatible_t : std::int8_t {
        Error = -1,
        Warning = 0,
        Valid = 1
    };

    struct typeCompatibilityResult {
        const Compatible_t result;
        const std::string message = "";

        constexpr operator bool() const noexcept { return result != Compatible_t::Error; }
    };

   public:
    analyzer(parser::ParsedFile& file, mnstl::chunk_allocator& arena) :
        symbolTable(arena), typeContext(arena), parsedFile(file) {}

    Result analyze();

    ~analyzer() override = default;

   private:
    Result collectTypes();

    void _reportRedeclaration(std::string_view redeclaredSymbolName, ast::ASTNode* node) const;
    Result _collectTypesInStatement(ast::Statement*);
    Result _collectTypesInStatementBody(const ast::Block&);
    Result collectGlobals();
    Result collectAndSpecializeGenerics();
    Result checkStatements();

    typeCompatibilityResult areTypesCompatible(const SemanticType* from, const SemanticType* to) const;
    typeCompatibilityResult arePrimitivesCompatible(const SemanticType* from, const SemanticType* to) const;
    typeCompatibilityResult areTypesComparable(const SemanticType* lhs, const SemanticType* rhs) const;
    const SemanticType* promoteNumericTypes(const SemanticType* lhs, const SemanticType* rhs) const;
    const SemanticType* resolveType(const ast::Type* astType);
    Result analyzePointerArithmetic(const SemanticType* lhs, const SemanticType* rhs) const;

    template <class... Args>
    static void logError(const ast::ASTNode* node, std::format_string<Args...> message, Args&&... args) noexcept {
        logging::logError(node->getLine(), node->getColumn(), message, std::forward<Args>(args)...);
    }

    template <class... Args>
    static void logWarning(ast::ASTNode* node, std::format_string<Args...> message, Args&&... args) noexcept {
        logging::logWarning(node->getLine(), node->getColumn(), message, std::forward<Args>(args)...);
    }

   protected:
    // overrides for visitor functions
    using _analyzer_base_t::visit;

#define STMT(name, str) stmtvisit_t visit(ast::name*) override;
#define EXPR(name, str) exprvisit_t visit(ast::name*) override;
#define TYPE(name, str) typevisit_t visit(ast::name*) override;

#include <frontend/ast/ast.def>

#undef STMT
#undef EXPR
#undef TYPE

    Result visit(std::nullptr_t) const noexcept {
        logging::logInternal(logging::LogLevel::Warning, "visit() called on nullptr in analyzer");
        return Result::Failure;
    }

    Result visit(ast::Block& block) {
        Result result = Result::Success;
        symbolTable.enterScope();
        for (auto statement : block) {
            auto stmtResult = visit(statement);
            result = (result == Result::Success && stmtResult == Result::Success) ? Result::Success : Result::Failure;
        }
        symbolTable.exitScope();
        return result;
    }
};

constexpr bool isInteger(ast::PrimitiveType_t t) noexcept {
    using enum ast::PrimitiveType_t;
    return mnstl::enum_matches<ast::PrimitiveType_t>(t, i8, i16, i32, i64, i128, u8, u16, u32, u64, u128);
}

constexpr bool isFloat(ast::PrimitiveType_t t) noexcept {
    using enum ast::PrimitiveType_t;
    return mnstl::enum_matches<ast::PrimitiveType_t>(t, f32, f64);
}

constexpr bool isUnsignedInteger(ast::PrimitiveType_t t) noexcept {
    using enum ast::PrimitiveType_t;
    return mnstl::enum_matches<ast::PrimitiveType_t>(t, u8, u16, u32, u64, u128);
}

constexpr bool isNumeric(ast::PrimitiveType_t t) noexcept { return isInteger(t) || isFloat(t); }

constexpr bool isLogicalOp(lexer::TokenType t) noexcept {
    using enum lexer::TokenType;
    return mnstl::enum_matches<lexer::TokenType>(t, And, Or, Not);
}

constexpr bool isArithmeticOp(lexer::TokenType t) noexcept {
    using enum lexer::TokenType;
    return mnstl::enum_matches<lexer::TokenType>(t, Plus, Minus, Mul, Div, FloorDiv, Mod);
}

constexpr bool isRelationalOp(lexer::TokenType t) {
    using enum lexer::TokenType;
    return mnstl::enum_matches<lexer::TokenType>(t, GreaterThan, GreaterThanOrEqual, LessThan, LessThanOrEqual, Equal,
                                                 NotEqual);
}

constexpr bool isBitwiseOp(lexer::TokenType t) noexcept {
    using enum lexer::TokenType;
    return mnstl::enum_matches<lexer::TokenType>(t, BitAnd, BitOr, BitNot, BitXor, BitLShift, BitRShift);
}

}  // namespace semantic
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_SEMANTIC_ANALYZER_HPP