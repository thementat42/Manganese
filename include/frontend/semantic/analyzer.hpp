#ifndef MANGANESE_INCLUDE_FRONTEND_SEMANTIC_ANALYZER_HPP
#define MANGANESE_INCLUDE_FRONTEND_SEMANTIC_ANALYZER_HPP

#include <cstddef>
#include <cstdint>
#include <format>
#include <frontend/ast.hpp>
#include <frontend/lexer.hpp>
#include <frontend/lexer/token.hpp>
#include <frontend/parser.hpp>
#include <frontend/semantic/generics_helpers.hpp>
#include <frontend/semantic/symbol_table.hpp>
#include <frontend/semantic/type_context.hpp>
#include <mnstl/chunk_allocator.hxx>
#include <mnstl/enum_matches.hxx>
#include <mnstl/tiny_stack.hxx>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <utils/result.hpp>

namespace Manganese::semantic {

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

template <class T>
struct [[nodiscard]] StackGuard {
    mnstl::tiny_stack<T>& stack;
    explicit StackGuard(mnstl::tiny_stack<T>& _stack, T&& _new_element) : stack(_stack) {
        stack.push(std::move(_new_element));
    }
    ~StackGuard() noexcept { stack.pop(); }
};

using _analyzer_base_t = ast::Visitor<Result, Result, Result>;

class analyzer final : public _analyzer_base_t {
   private:
    SymbolTable symbolTable;
    TypeContext typeContext;
    parser::ParsedFile& parsedFile;
    mnstl::tiny_stack<TypeList> genericsStack;
    InstantiationCache instantiationCache;
    std::unordered_map<std::string_view, std::size_t> activeGenericParams;

    struct {
        bool inFunction = false;
        std::uint8_t typeCastDepth = 0;
        std::uint8_t ifStatementDepth = 0;
        std::uint8_t forLoopDepth = 0;
        std::uint8_t whileLoopDepth = 0;
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
        symbolTable(arena), typeContext(arena), parsedFile(file), genericsStack() {}

    Result analyze();

    ~analyzer() override = default;

   private:
    Result collectTypes();

    Result _collectTypesInStatement(ast::Statement*);
    Result _collectTypesInStatementBody(const ast::Block&);
    Result checkStatements();

    typeCompatibilityResult areTypesCompatible(const SemanticType* from, const SemanticType* to) const;
    typeCompatibilityResult arePrimitivesCompatible(const SemanticType* from, const SemanticType* to) const;
    typeCompatibilityResult areTypesComparable(const SemanticType* lhs, const SemanticType* rhs) const;
    const SemanticType* promoteNumericTypes(const SemanticType* lhs, const SemanticType* rhs) const;
    Result analyzePointerArithmetic(ast::BinaryExpression* expr) const;
    const SemanticType* resolveGenericType(const ast::Type* type);

    template <class... Args>
    static void logError(const ast::ASTNode* node, std::format_string<Args...> message, Args&&... args) noexcept {
        logging::logError(node->line, node->column, message, std::forward<Args>(args)...);
    }

    template <class... Args>
    static void logWarning(ast::ASTNode* node, std::format_string<Args...> message, Args&&... args) noexcept {
        logging::logWarning(node->line, node->column, message, std::forward<Args>(args)...);
    }

   protected:
    // overrides for visitor functions
    using _analyzer_base_t::visit;

#define STMT(name) stmtvisit_t visit(ast::name*) override;
#define EXPR(name) exprvisit_t visit(ast::name*) override;
#define TYPE(name) typevisit_t visit(ast::name*) override;

#include <frontend/ast/ast.def>

#undef STMT
#undef EXPR
#undef TYPE

    static Result visit(std::nullptr_t) {
        logging::logInternal(logging::LogLevel::Warning, "visit() called on nullptr in analyzer");
        return Result::Failure;
    }

    Result visit(ast::Block& block, bool enterNewScope = true) {
        Result result = Result::Success;
        if (enterNewScope) { symbolTable.enterScope(); }
        for (ast::Statement* statement : block) {
            auto stmtResult = visit(statement);
            if (stmtResult == Result::Failure) { result = Result::Failure; }
        }
        symbolTable.exitScope();
        return result;
    }

    // Overloads to handle generics specializations
    stmtvisit_t visit(ast::AggregateDeclarationStatement*, generic_tag_t);
    stmtvisit_t visit(ast::FunctionDeclarationStatement*, generic_tag_t);
    const SemanticType* getInstantiatedFunctionType(const ast::FunctionDeclarationStatement* decl,
                                                    const TypeList& typeArgs);
    const SemanticType* getInstantiatedAggregateType(const ast::AggregateDeclarationStatement* decl,
                                                     const TypeList& typeArgs);

    static ast::Expression* unwrapBaseDeclaration(ast::Expression* expr) {
        if (!expr) [[unlikely]] { return nullptr; }
        using enum ast::ExpressionKind;
        switch (expr->kind) {
            case GenericExpression:
                return unwrapBaseDeclaration(static_cast<ast::GenericExpression*>(expr)->identifier);
            case ScopeResolutionExpression:
                return unwrapBaseDeclaration(static_cast<ast::ScopeResolutionExpression*>(expr)->element);
            default: return expr;
        }
    }

    bool isMutableExpression(const ast::Expression* expr);
};

constexpr bool isLogicalOp(lexer::TokenType t) noexcept {
    using enum lexer::TokenType;
    return mnstl::enum_matches(t, And, Or, Not);
}

constexpr bool isArithmeticOp(lexer::TokenType t) noexcept {
    using enum lexer::TokenType;
    return mnstl::enum_matches(t, Plus, Minus, Mul, Div, FloorDiv, Mod);
}

constexpr bool isRelationalOp(lexer::TokenType t) {
    using enum lexer::TokenType;
    return mnstl::enum_matches(t, GreaterThan, GreaterThanOrEqual, LessThan, LessThanOrEqual, Equal, NotEqual);
}

constexpr bool isBitwiseOp(lexer::TokenType t) noexcept {
    using enum lexer::TokenType;
    return mnstl::enum_matches(t, BitAnd, BitOr, BitNot, BitXor, BitLShift, BitRShift);
}

constexpr bool isLvalue(const ast::Expression* expr) noexcept {
    using enum ast::ExpressionKind;
    const ast::ExpressionKind k = expr->kind;
    if (k == PrefixExpression) {
        return static_cast<const ast::PrefixExpression*>(expr)->op == lexer::TokenType::Dereference;
    }
    return mnstl::enum_matches(expr->kind, IdentifierExpression, IndexExpression, MemberAccessExpression, ScopeResolutionExpression);
}

}  // namespace Manganese::semantic

#endif  // MANGANESE_INCLUDE_FRONTEND_SEMANTIC_ANALYZER_HPP