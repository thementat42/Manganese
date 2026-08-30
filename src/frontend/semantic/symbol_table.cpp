#include <algorithm>
#include <frontend/ast.hpp>
#include <frontend/semantic/symbol_table.hpp>

namespace Manganese::semantic {

void SymbolTable::enterScope() {
    if (_flags._isFirstPass) {
        // Allocate memory to build a new scope
        auto* newScope = _arena.emplace<Scope>();
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

void SymbolTable::enterNamespace(std::string_view name, ast::ASTNode* node) {
    if (_flags._isFirstPass) {
        Symbol* namespaceSymbol = _currentScope->lookup(name);
        // Declare the namespace in the current scope if it's not already declared
        if (namespaceSymbol == nullptr) {
            DISCARD(_currentScope->insert(name,
                                          Symbol{.type = nullptr,
                                                 .node = node,
                                                 .hostScope = _currentScope,
                                                 .kind = SymbolKind::Namespace,
                                                 .visibility = ast::Visibility::Public,
                                                 .isMutable = false,
                                                 .status = ResolutionStatus::Success}));
            namespaceSymbol = _currentScope->lookup(name);
        }

        Scope* namespaceScope = nullptr;
        if (auto existingNamespace = std::ranges::find(_currentScope->children, name, &Scope::namespaceName);
            existingNamespace != _currentScope->children.end()) {
            // the namespace was opened again, we want to just re-enter that scope
            namespaceScope = *existingNamespace;
        }

        // first time seeing this namespace; register it
        if (!namespaceScope) {
            namespaceScope = _arena.emplace<Scope>();
            namespaceScope->parent = _currentScope;
            namespaceScope->namespaceName = name;
            _currentScope->children.push_back(namespaceScope);
        }
        if (namespaceSymbol) { namespaceSymbol->scopeDefined = namespaceScope; }
        _currentScope = namespaceScope;
    } else {
        Symbol* namespaceSymbol = _currentScope->lookup(name);
        if (!namespaceSymbol || !namespaceSymbol->scopeDefined) {
            logging::logInternal(logging::LogLevel::Error, "Failed to resolve namespace scope during pass");
            return;
        }

        _currentScope = namespaceSymbol->scopeDefined;
    }
}

std::string Scope::getQualifiedName() const {
    if (!parent || parent->namespaceName.empty()) { return std::string(namespaceName); }
    return parent->getQualifiedName() + "::" + std::string(namespaceName);
}

void SymbolTable::exitScope() noexcept {
    if (noScopeAvailable() || !_currentScope->parent) [[unlikely]] {
        logging::logInternal(logging::LogLevel::Warning, "Attempted to exit scope when no parent scope was available");
        return;
    }
    _currentScope = _currentScope->parent;
}

Symbol* SymbolTable::lookup(std::string_view name) noexcept {
    Scope* probe = _currentScope;
    while (probe) {
        Symbol* symbol = probe->lookup(name);
        if (symbol) { return symbol; }
        probe = probe->parent;
    }

    logging::logInternal(logging::LogLevel::Warning, "Symbol '{}' not found in any visible lexical scope.", name);
    return nullptr;
}

const Symbol* SymbolTable::lookup(std::string_view name) const noexcept {
    const Scope* probe = _currentScope;
    while (probe) {
        const Symbol* symbol = probe->lookup(name);
        if (symbol) { return symbol; }
        probe = probe->parent;
    }

    logging::logInternal(logging::LogLevel::Warning, "Symbol '{}' not found in any visible lexical scope.", name);
    return nullptr;
}

}  // namespace Manganese::semantic