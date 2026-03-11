#ifndef MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SYMBOL_TABLE_HPP
#define MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SYMBOL_TABLE_HPP

#include <stdint.h>

#include <format>
#include <frontend/ast.hpp>
#include <functional>
#include <global_macros.hpp>
#include <io/logging.hpp>
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
    ast::TypeSPtr_t type;
    ast::ASTNode* node = nullptr;
    size_t scopeDepth = 0;
    SymbolKind kind;
    ast::Visibility visibility = ast::Visibility::Private;
    bool isMutable;
    std::string toString() const noexcept;
};

struct Scope {
    // std::equal_to<> enables heterogenous lookup
    std::unordered_map<std::string_view, Symbol, std::hash<std::string_view>, std::equal_to<>> symbols;
    Result insert(std::string_view name, Symbol symbol) {
        bool result = symbols.emplace(name, std::move(symbol)).second;
        return result ? Result::Success : Result::Failure;
    }

    const Symbol* lookup(std::string_view name) const noexcept {
        auto it = symbols.find(name);
        return it == symbols.end() ? nullptr : &(it->second);
    }
};

class SymbolTable {
   private:
    std::vector<Scope> _scopes;
    int64_t _currentDepth;
    bool _hasError;

    constexpr bool noScopeAvailable() const noexcept { return _scopes.empty() || _currentDepth < 0; }

   public:
    SymbolTable() noexcept : _scopes(), _currentDepth(0), _hasError(false) { _scopes.emplace_back(); }
    constexpr ~SymbolTable() = default;

    constexpr bool hasError() const noexcept { return _hasError; }

    void enterScope() {
        ++_currentDepth;
        // if we've exceeded the current symbol table depth, create a new scope
        // otherwise, we're just moving our current scope to the next nested one
        if (size_t(_currentDepth) >= _scopes.size()) { _scopes.emplace_back(); }
    }
    void exitScope() noexcept {
        if (noScopeAvailable()) [[unlikely]] {
            logging::logInternal(logging::LogLevel::Warning, "Attempted to exit scope when no scope was available");
            return;
        }
        // since we're doing multiple passes, we want to preserve scope information between passes
        --_currentDepth;
    }
    Result declare(std::string_view name, Symbol symbol) {
        if (noScopeAvailable()) [[unlikely]] {
            logging::logInternal(logging::LogLevel::Error, "No active scope in which to declare a symbol");
            return Result::Failure;
        }
        symbol.scopeDepth = getCurrentDepth();
        return _scopes[getCurrentDepth()].insert(name, std::move(symbol));
    }

    const Symbol* lookup(std::string_view name) const noexcept {
        for (int64_t i = _currentDepth; i >= 0; --i) {  // prioritize local symbols over globals
            const Symbol* _sym = _scopes[size_t(i)].lookup(name);
            if (_sym) { return _sym; }
        }
        logging::logInternal(logging::LogLevel::Warning, "Symbol '{}' not found in any scope.", name);
        return nullptr;
    }
    const Symbol* lookupAtCurrentDepth(std::string_view name) const noexcept {
        if (noScopeAvailable()) {
            logging::logInternal(logging::LogLevel::Error, "No active scope in which to look up symbol");
            return nullptr;
        }
        const Symbol* symbol = _scopes[getCurrentDepth()].lookup(name);
        if (!symbol) { logging::logInternal(logging::LogLevel::Warning, "Symbol '{}' not found in any scope", name); }
        return symbol;
    }
    constexpr inline size_t getCurrentDepth() const noexcept { return size_t(_currentDepth); }
};

}  // namespace semantic
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SYMBOL_TABLE_HPP