#include <frontend/semantic/symbol_table.h>

namespace Manganese {
namespace semantic {

Symbol* Scope::lookup(const std::string& name) {
    auto it = symbols.find(name);
    return (it != symbols.end()) ? &it->second : nullptr;
}

void SymbolTable::enterScope() {
    scopes.emplace_back();
    scopeDepth++;
}

void SymbolTable::exitScope() {
    if (!scopes.empty()) {
        scopes.pop_back();
        scopeDepth--;
    } else {
        logging::logInternal("Attempted to exit scope when no scopes are available.", logging::LogLevel::Warning);
    }
}

bool SymbolTable::declare(Symbol symbol) {
    if (scopes.empty()) {
        logging::logInternal("No active scope to declare symbol.", logging::LogLevel::Error);
        return false;
    }
    symbol.scopeDepth = scopeDepth;
    return scopes.back().insert(std::move(symbol));
}

Symbol* SymbolTable::lookup(const std::string& name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        Symbol* symbol = it->lookup(name);
        if (symbol) {
            return symbol;
        }
    }
    logging::logInternal("Symbol '" + name + "' not found in any scope.", logging::LogLevel::Warning);
    return nullptr;
}

Symbol* SymbolTable::currentScopeLookup(const std::string& name) {
    if (scopes.empty()) {
        logging::logInternal("No active scope to lookup symbol.", logging::LogLevel::Error);
        return nullptr;
    }
    Symbol* symbol = scopes.back().lookup(name);
    if (!symbol) {
        logging::logInternal("Symbol '" + name + "' not found in current scope.", logging::LogLevel::Warning);
    }
    return symbol;
}

}  // namespace semantic

}  // namespace Manganese