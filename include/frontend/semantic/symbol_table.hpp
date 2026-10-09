#ifndef MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SYMBOL_TABLE_HPP
#define MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SYMBOL_TABLE_HPP

#include <cstddef>
#include <cstdint>
#include <format>
#include <frontend/ast.hpp>
#include <frontend/semantic/type_context.hpp>
#include <io/logging.hpp>
#include <mnstl/chunk_allocator.hxx>
#include <unordered_map>
#include <utils/resolution_status.hpp>
#include <utils/result.hpp>
#include <utils/string_interner.hpp>
#include <vector>

namespace Manganese::semantic {

enum class SymbolKind : std::uint8_t {
    Aggregate,
    Constant,
    ConstantParameter,
    Enum,
    Function,
    GenericType,
    Import,
    Module,
    Namespace,
    Parameter,
    TypeAlias,
    Variable
};

struct Scope;

struct Symbol {
    const SemanticType* type;
    ast::ASTNode* node;
    Scope* hostScope = nullptr;  // which scope this symbol lives in (set in SymbolTable::declare())
    Scope* scopeDefined = nullptr;  // for namespaces/modules, indicates that this symbol defines a scope
    SymbolKind kind;
    ast::Visibility visibility = ast::Visibility::Private;
    bool isMutable;
    ResolutionStatus status = ResolutionStatus::NotStarted;
    std::size_t ID = static_cast<size_t>(-1);  // placeholder, set by declare()
};

struct Scope {
    std::unordered_map<utils::StringID, Symbol> symbols;
    Scope* parent = nullptr;
    std::vector<Scope*> children;
    std::size_t currentChildIndex = 0;
    utils::StringID namespaceName = {};

    inline Result insert(utils::StringID name, Symbol symbol) {
        const bool emplace_succeeded = symbols.emplace(name, symbol).second;
        return emplace_succeeded ? Result::Success : Result::Failure;
    }

    [[nodiscard]] inline Symbol* lookup(utils::StringID name) noexcept {
        auto it = symbols.find(name);
        return it == symbols.end() ? nullptr : &(it->second);
    }

    [[nodiscard]] inline const Symbol* lookup(utils::StringID name) const noexcept {
        auto it = symbols.find(name);
        return it == symbols.end() ? nullptr : &(it->second);
    }
    std::string getQualifiedName(const utils::StringInterner&) const;
};

class SymbolTable {
   private:
    mnstl::chunk_allocator& _arena;
    Scope* _root;
    Scope* _currentScope;
    struct {
        bool _isFirstPass : 1 = true;  // Toggles table from allocation mode to tree-tracking mode
        std::uint8_t _genericDepth = 0;
    } _flags;
    std::size_t currentSymbolID = 0;

    inline bool noScopeAvailable() const noexcept { return _currentScope == nullptr; }

   public:
    SymbolTable(mnstl::chunk_allocator& arena) noexcept :
        _arena(arena), _root(_arena.emplace<Scope>()), _currentScope(_root) {}

    ~SymbolTable() noexcept = default;

    void resetToRoot() noexcept {
        auto resetIndices = [](auto& self, Scope* scope) -> void {
            scope->currentChildIndex = 0;
            for (Scope* child : scope->children) { self(self, child); }
        };

        resetIndices(resetIndices, _root);
        _currentScope = _root;
    }

    // Call before beginning pass 2
    void switchToCheckingMode() noexcept {
        _flags._isFirstPass = false;
        resetToRoot();
    }

    void enterScope();
    void enterNamespace(utils::StringID name, ast::ASTNode* node);
    void enterGenericCheckingMode() noexcept { ++_flags._genericDepth; }
    void exitScope() NOEXCEPT_IF_RELEASE;
    void FORCE_INLINE exitNamespace() NOEXCEPT_IF_RELEASE { exitScope(); }
    void exitGenericCheckingMode() noexcept { --_flags._genericDepth; }

    Scope* getCurrentScope() noexcept { return _currentScope; }
    const Scope* getCurrentScope() const noexcept { return _currentScope; }
    void setCurrentScope(Scope* scope) noexcept { _currentScope = scope; }
    std::size_t getSize() const noexcept { return currentSymbolID; }

    Result declare(utils::StringID name, Symbol&& symbol) {
        if (noScopeAvailable()) [[unlikely]] {
            logging::logInternal(logging::LogLevel::Error, "No active scope in which to declare a symbol");
            return Result::Failure;
        }
        symbol.hostScope = _currentScope;
        symbol.ID = currentSymbolID++;
        return _currentScope->insert(name, symbol);
    }

    Symbol* lookup(utils::StringID name) NOEXCEPT_IF_RELEASE;
    const Symbol* lookup(utils::StringID name) const NOEXCEPT_IF_RELEASE;

    static Symbol* scopedLookup(Scope* targetScope, utils::StringID member) noexcept {
        return targetScope->lookup(member);
    }

    static const Symbol* scopedLookup(const Scope* targetScope, utils::StringID member) noexcept {
        return targetScope->lookup(member);
    }
};

}  // namespace Manganese::semantic

#endif  // MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SYMBOL_TABLE_HPP