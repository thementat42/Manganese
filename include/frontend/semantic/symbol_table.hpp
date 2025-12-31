#ifndef MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SYMBOL_TABLE_HPP
#define MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SYMBOL_TABLE_HPP

#include <stdint.h>

#include <format>
#include <frontend/ast.hpp>
#include <global_macros.hpp>
#include <io/logging.hpp>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Manganese {
namespace semantic {

enum class SymbolKind {
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
    Invalid = -1
};

struct Symbol {
    std::string name;
    SymbolKind kind;
    ast::TypeSPtr_t type;
    size_t line, column;

    ast::ASTNode* node = nullptr;

    // === Semantic Info ===
    bool isMutable;
    size_t scopeDepth = 0;
    ast::Visibility visibility = ast::Visibility::Private;

    std::string toString() const noexcept;
};

struct Scope {
    std::unordered_map<std::string, Symbol> symbols;
    inline bool insert(Symbol&& symbol) { return symbols.emplace(symbol.name, std::move(symbol)).second; }

    const Symbol* lookup(const std::string& name) const noexcept {
        auto it = symbols.find(name);
        return it == symbols.end() ? nullptr : &(it->second);
    }
};

class SymbolTable {
   private:
    std::vector<Scope> _scopes;
    size_t _currentDepth;

   public:
    SymbolTable() noexcept: _currentDepth(0) { enterScope(); }
    constexpr ~SymbolTable() = default;

    void enterScope() {
        ++_currentDepth;
        // if we've exceeded the current symbol table depth, create a new scope
        // otherwise, we're just moving our current scope to the next nested one
        if (_currentDepth >= _scopes.size()) {_scopes.emplace_back(); }
    }
    void exitScope() noexcept {
        if (_scopes.empty()) [[unlikely]] {
            using logging::logInternal, logging::LogLevel;
            logInternal("Attempted to exit scope when no scope was available", LogLevel::Warning);
            return;
        }
        --_currentDepth;
    }
    bool declare(Symbol symbol) {
        if (_scopes.empty()) [[unlikely]] {
            using logging::logInternal, logging::LogLevel;
            logInternal("No active scope to declare a symbol", LogLevel::Error);
            return false;
        }
        symbol.scopeDepth = getCurrentDepth();
        return _scopes[getCurrentDepth() - 1].insert(std::move(symbol));
    }

    const Symbol* lookup(const std::string& name) const noexcept {
        for (auto it = _scopes.rbegin(); it != _scopes.rend(); ++it) {
            const Symbol* _sym = it->lookup(name);
            if (_sym) { return _sym; }
        }
        logging::logInternal("Symbol '" + name + "' not found in any scope.", logging::LogLevel::Warning);
        return nullptr;
    }
    const Symbol* lookupAtCurrentDepth(const std::string& name) const noexcept {
        if (_scopes.empty()) {
            logging::logInternal("No active scope to lookup symbol.", logging::LogLevel::Error);
            return nullptr;
        }
        const Symbol* symbol = _scopes[getCurrentDepth() - 1].lookup(name);
        if (!symbol) {
            logging::logInternal("Symbol '" + name + "' not found in current scope.", logging::LogLevel::Warning);
        }
        return symbol;
    }
    const Symbol* lookupAtDepth(const std::string& name, int64_t depth) const noexcept {
        if (depth < 0 || depth >= (int64_t)getCurrentDepth()) [[unlikely]] {
            logging::logInternal(std::format("Invalid scope depth {} (valid range: 0-{})", depth, getCurrentDepth() - 1),
                                 logging::LogLevel::Warning);
            return nullptr;
        }
        size_t _index = getCurrentDepth() - 1 - (size_t)depth;  // go to the appropriate depth
        return _scopes[_index].lookup(name);
    }
    constexpr inline size_t getCurrentDepth() const noexcept { return _currentDepth; }
};

}  // namespace semantic
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SYMBOL_TABLE_HPP