#ifndef MANGANESE_INCLUDE_FRONTEND_SEMANTIC_ANALYZER_HPP
#define MANGANESE_INCLUDE_FRONTEND_SEMANTIC_ANALYZER_HPP

#include <core.hpp>
#include <cstddef>
#include <cstdint>
#include <format>
#include <frontend/ast.hpp>
#include <frontend/lexer.hpp>
#include <frontend/parser.hpp>
#include <frontend/semantic/symbol_table.hpp>
#include <frontend/semantic/type_context.hpp>
#include <mnstl/chunk_allocator.hxx>
#include <utility>

namespace Manganese {
namespace semantic {
using _analyzer_base_t = ast::Visitor<Result, Result, Result>;

class analyzer final : public _analyzer_base_t {
   private:
    SymbolTable symbolTable;
    TypeContext typeContext;
    parser::ParsedFile& parsedFile;

    struct {
        bool inFunction : 1 = false;
        uint8_t ifStatementDepth = 0;
        uint8_t switchStatementDepth = 0;
        uint8_t forLoopDepth = 0;
        uint8_t whileLoopDepth = 0;
        SemanticType* currentFunctionReturnType = nullptr;
    } context;

    struct typeCompatibilityResult {
        enum class result_t : std::int8_t {
            Error = -1,
            Warning = 0,
            Valid = 1
        };
        const result_t result;
        const std::string message = "";

        operator bool() const noexcept { return result != result_t::Error; }
    };

   public:
    analyzer(parser::ParsedFile& file, mnstl::chunk_allocator& arena) :
        symbolTable(arena), typeContext(), parsedFile(file) {}

    Result analyze() {
        collectTypes();
        collectGlobals();
        collectAndSpecializeGenerics();
        // Don't want errors cascading because of conflicting redeclarations
        if (symbolTable.hasError()) { return Result::Failure; }

        symbolTable.switchToCheckingMode();
        Result isSemanticallyValid = checkStatements();
        return isSemanticallyValid;
    }

    ~analyzer() override = default;

   private:
    inline void collectTypes() {  // first pass -- collect all user-defined types
        for (const auto& stmt : parsedFile.program) { _collectTypesInStatement(stmt); }
    }

    typeCompatibilityResult areTypesCompatible(const SemanticType* from, const SemanticType* to) const;

    void _collectTypesInStatement(ast::Statement*);
    void _collectTypesInStatementBody(ast::Statement*);
    void collectGlobals();  // second pass -- collect publicly available symbols for modules
    void collectAndSpecializeGenerics() {
        // third pass -- look at specific generic instantiations and specialize them (e.g.
        // foo@[int] -> create a specialization of foo w/ int)
        // does nothing for now
    }

    inline Result checkStatements() {  // semantic analysis pass (this can also check the generic specializations)
        Result programIsSemanticallyValid = Result::Success;
        for (auto& stmt : parsedFile.program) {
            if (this->visit(stmt) == Result::Failure) { programIsSemanticallyValid = Result::Failure; }
        }
        return programIsSemanticallyValid;
    }

    constexpr static bool isInteger(ast::PrimitiveType_t t) noexcept {
        using enum ast::PrimitiveType_t;
        return (t == i8) || (t == i16) || (t == i32) || (t == i64) || (t == i128) || (t == u8) || (t == u16)
            || (t == u32) || (t == u64) || (t == u128);
    }

    constexpr static bool isFloat(ast::PrimitiveType_t t) noexcept {
        using enum ast::PrimitiveType_t;
        return (t == f32) || (t == f64);
    }

    constexpr static bool isUnsignedInteger(ast::PrimitiveType_t t) noexcept {
        using enum ast::PrimitiveType_t;
        return (t == u8) || (t == u16) || (t == u32) || (t == u64) || (t == u128);
    }

    constexpr static bool isNumeric(ast::PrimitiveType_t t) noexcept { return isInteger(t) || isFloat(t); }

    template <class... Args>
    static void logError(ast::ASTNode* node, std::format_string<Args...> message, Args&&... args) noexcept {
        logging::logError(node->getLine(), node->getColumn(), message, std::forward<Args>(args)...);
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
        for (auto statement : block) {
            auto stmtResult = visit(statement);
            result = (result == Result::Success && stmtResult == Result::Success) ? Result::Success : Result::Failure;
        }
        return result;
    }
};

}  // namespace semantic
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_SEMANTIC_ANALYZER_HPP