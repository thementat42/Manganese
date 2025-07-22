/**
 * @file symbol_table.cpp
 * @brief Implementation of symbol table functions
 */

#include <frontend/semantic/symbol_table.h>
#include <global_macros.h>

#include <format>
#include <iomanip>
#include <string>

namespace Manganese {
namespace semantic {

void SymbolTable::enterScope() {
    scopes.emplace_back();
    ++scopeDepth;
}

void SymbolTable::exitScope() noexcept {
    if (!scopes.empty()) {
        scopes.pop_back();
        --scopeDepth;
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

Symbol* SymbolTable::lookupInCurrentScope(const std::string& name) {
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

std::string SymbolKindToString(const SymbolKind kind) noexcept_if_release {
    switch (kind) {
        case SymbolKind::Variable:
            return "Variable";
        case SymbolKind::Constant:
            return "Constant";
        case SymbolKind::Function:
            return "Function";
        case SymbolKind::Bundle:
            return "Bundle";
        case SymbolKind::Enum:
            return "Enum";
        case SymbolKind::Module:
            return "Module";
        case SymbolKind::Import:
            return "Import";
        case SymbolKind::TypeAlias:
            return "TypeAlias";
        case SymbolKind::GenericType:
            return "GenericType";
        case SymbolKind::Invalid:
            return "Invalid";
        default:
            ASSERT_UNREACHABLE(std::format("No string translation for symbol kind {}", static_cast<int>(kind)));
    }
}

std::string Symbol::toString() const noexcept {
    std::ostringstream oss;
    oss << "Symbol: " << name << "\n";
    oss << "Kind: " << SymbolKindToString(kind) << "\n";
    oss << "Type: " << type->toString() << "\n";
    oss << "Constant: " << (isConstant ? "yes" : "no") << "\n";
    oss << "Visibility: " << ast::visibilityToString(visibility) << "\n";
    return oss.str();
}

}  // namespace semantic

}  // namespace Manganese
