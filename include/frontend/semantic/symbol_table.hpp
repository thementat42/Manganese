#ifndef MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SYMBOL_TABLE_HPP
#define MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SYMBOL_TABLE_HPP

#include <cstddef>
#include <cstdint>
#include <format>
#include <frontend/ast.hpp>
#include <frontend/semantic/type_context.hpp>
#include <functional>
#include <io/logging.hpp>
#include <mnstl/chunk_allocator.hxx>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utils/result.hpp>
#include <vector>

namespace Manganese::semantic {

enum class SymbolKind : std::uint8_t {
    Variable,
    Constant,
    Function,
    Parameter,
    ConstantParameter,
    Aggregate,
    Enum,
    Module,
    Import,
    TypeAlias,
    GenericType,
    Namespace,
    Invalid
};

struct Scope;

struct Symbol {
    const SemanticType* type = nullptr;
    ast::ASTNode* node = nullptr;
    SymbolKind kind;
    ast::Visibility visibility = ast::Visibility::Private;
    bool isMutable;
    ResolutionStatus status = ResolutionStatus::NotStarted;
    Scope* scope = nullptr;
    std::string toString() const;
};

struct Scope {
    // std::equal_to<> enables heterogenous lookup (e.g. looking up with std::string or const char*)
    // so explicit conversions are not required
    std::unordered_map<std::string_view, Symbol, std::hash<std::string_view>, std::equal_to<>> symbols;
    Scope* parent = nullptr;
    std::vector<Scope*> children;
    std::size_t currentChildIndex = 0;
    std::string_view namespaceName = {};

    inline Result insert(std::string_view name, Symbol symbol) {
        const bool emplace_succeeded = symbols.emplace(name, symbol).second;
        return emplace_succeeded ? Result::Success : Result::Failure;
    }

    [[nodiscard]] inline Symbol* lookup(std::string_view name) noexcept {
        auto it = symbols.find(name);
        return it == symbols.end() ? nullptr : &(it->second);
    }

    [[nodiscard]] inline const Symbol* lookup(std::string_view name) const noexcept {
        auto it = symbols.find(name);
        return it == symbols.end() ? nullptr : &(it->second);
    }
};

class SymbolTable {
   private:
    mnstl::chunk_allocator& _arena;
    Scope* _root;
    Scope* _currentScope;
    struct {
        bool _isFirstPass : 1 = true;  // Toggles table from allocation mode to tree-tracking mode
    } _flags;

    inline bool noScopeAvailable() const noexcept { return _currentScope == nullptr; }

   public:
    SymbolTable(mnstl::chunk_allocator& arena) noexcept :
        _arena(arena), _root(_arena.emplace<Scope>()), _currentScope(_root) {}

    ~SymbolTable() noexcept = default;

    // Call before beginning pass 2
    void switchToCheckingMode() noexcept {
        _flags._isFirstPass = false;

        auto resetIndices = [](auto& self, Scope* scope) -> void {
            scope->currentChildIndex = 0;
            for (Scope* child : scope->children) { self(self, child); }
        };

        resetIndices(resetIndices, _root);
        _currentScope = _root;
    }

    void enterScope();
    void enterNamespace(std::string_view name, ast::ASTNode* node);
    void exitScope() noexcept;
    void FORCE_INLINE exitNamespace() noexcept { exitScope(); }

    Result declare(std::string_view name, Symbol&& symbol) {
        if (noScopeAvailable()) [[unlikely]] {
            logging::logInternal(logging::LogLevel::Error, "No active scope in which to declare a symbol");
            return Result::Failure;
        }
        return _currentScope->insert(name, std::move(symbol));
    }

    Symbol* lookup(std::string_view name) noexcept;
    const Symbol* lookup(std::string_view name) const noexcept;

    Symbol* scopedLookup(Scope* targetScope, std::string_view member) noexcept { return targetScope->lookup(member); }

    const Symbol* scopedLookup(const Scope* targetScope, std::string_view member) const noexcept {
        return targetScope->lookup(member);
    }
};

}  // namespace Manganese::semantic

#endif  // MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SYMBOL_TABLE_HPP