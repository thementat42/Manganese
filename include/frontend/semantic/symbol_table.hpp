#ifndef MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SYMBOL_TABLE_HPP
#define MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SYMBOL_TABLE_HPP

#include <stdint.h>

#include <core.hpp>
#include <format>
#include <frontend/ast.hpp>
#include <functional>
#include <io/logging.hpp>
#include <mnstl/chunk_allocator.hxx>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <utils/result.hpp>
#include <vector>

namespace Manganese {
namespace semantic {

enum class SymbolKind : uint8_t {
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
    Invalid
};

struct Symbol {
    SemanticType* type = nullptr;
    ast::ASTNode* node = nullptr;
    SymbolKind kind;
    ast::Visibility visibility = ast::Visibility::Private;
    bool isMutable;
    std::string toString() const noexcept;
};

struct Scope {
    // std::equal_to<> enables heterogenous lookup (e.g. looking up with std::string or const char*)
    // so explicit conversions are not required
    std::unordered_map<std::string_view, Symbol, std::hash<std::string_view>, std::equal_to<>> symbols;
    Scope* parent = nullptr;
    std::vector<Scope*> children;
    size_t currentChildIndex = 0;

    inline Result insert(std::string_view name, Symbol symbol) {
        bool emplace_succeeded = symbols.emplace(name, std::move(symbol)).second;
        return emplace_succeeded ? Result::Success : Result::Failure;
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

    void enterScope() {
        if (_flags._isFirstPass) {
            // Allocate memory to build a new scope
            Scope* newScope = _arena.emplace<Scope>();
            newScope->parent = _currentScope;

            _currentScope->children.push_back(newScope);
            _currentScope = newScope;
        } else {
            // Retrieve the next child scope in the same order it was recorded in in pass 1
            if (_currentScope->currentChildIndex >= _currentScope->children.size()) [[unlikely]] {
                logging::logInternal(logging::LogLevel::Error, "Mismatched scope structural traversal");
                return;
            }
            _currentScope = _currentScope->children[_currentScope->currentChildIndex++];
        }
    }

    void exitScope() noexcept {
        if (noScopeAvailable() || !_currentScope->parent) [[unlikely]] {
            logging::logInternal(logging::LogLevel::Warning,
                                 "Attempted to exit scope when no parent scope was available");
            return;
        }
        _currentScope = _currentScope->parent;
    }

    Result declare(std::string_view name, Symbol symbol) {
        if (noScopeAvailable()) [[unlikely]] {
            logging::logInternal(logging::LogLevel::Error, "No active scope in which to declare a symbol");
            return Result::Failure;
        }
        return _currentScope->insert(name, std::move(symbol));
    }

    const Symbol* lookup(std::string_view name) const noexcept {
        // Safe, upward lexical lookup through parent scopes without index array tracking
        const Scope* probe = _currentScope;
        while (probe) {
            const Symbol* symbol = probe->lookup(name);
            if (symbol) { return symbol; }
            probe = probe->parent;
        }

        logging::logInternal(logging::LogLevel::Warning, "Symbol '{}' not found in any visible lexical scope.", name);
        return nullptr;
    }

    const Symbol* lookupAtCurrentDepth(std::string_view name) const noexcept {
        if (noScopeAvailable()) {
            logging::logInternal(logging::LogLevel::Error, "No active scope in which to look up symbol");
            return nullptr;
        }
        const Symbol* symbol = _currentScope->lookup(name);
        if (!symbol) {
            logging::logInternal(logging::LogLevel::Warning, "Symbol '{}' not found at current local depth", name);
        }
        return symbol;
    }
};

}  // namespace semantic
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SYMBOL_TABLE_HPP