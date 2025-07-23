#ifndef MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SYMBOL_TABLE_H
#define MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SYMBOL_TABLE_H

#include <frontend/ast.h>
#include <frontend/parser.h>
#include <global_macros.h>

#include <string>
#include <unordered_map>

namespace Manganese {

namespace semantic {

enum class SymbolKind {
    Variable,
    Constant,
    Function,
    Bundle,
    Enum,
    Module,
    Import,
    TypeAlias,
    GenericType,
    Invalid = -1
};

struct Symbol {
    // === Identity Info ===
    std::string name;  // e.g. "myInt", "myBundle"
    SymbolKind kind;
    // Note: This is just a reference to the the type node.
    // ! Symbols are not responsible for this memory and should never allocate or free memory
    ast::TypeSPtr_t type;  // Holds any type-specific info (e.g. function return type, array size)

    // === Source Info ===
    size_t line = 0, column = 0;
    ast::ASTNode* declarationNode = nullptr;
    
    // === Semantic Info ===
    bool isConstant;
    int64_t scopeDepth = 0;
    ast::Visibility visibility = ast::Visibility::Private;  // How the symbol can/can't be accessed outside the module
    
    // === Methods ===
    std::string toString() const noexcept;
};

struct Scope {
    std::unordered_map<std::string, Symbol> symbols;
    inline bool insert(const Symbol symbol) {
        return symbols.emplace(symbol.name, symbol).second;
    }

    Symbol* lookup(const std::string& name) {
        auto it = symbols.find(name);
        return (it != symbols.end()) ? &it->second : nullptr;
    }

    inline Symbol* lookup(const char* name) {
        return lookup(std::string(name));
    }
};

class SymbolTable {
   private:
    std::vector<Scope> scopes;
    int64_t scopeDepth = 0;

   public:
    SymbolTable() noexcept {
        enterScope();
    }

    void enterScope();

    void exitScope() noexcept;

    bool declare(Symbol symbol);

    Symbol* lookup(const std::string& name);
    inline Symbol* lookup(const char* name) { return lookup(std::string(name)); }
    Symbol* lookupInCurrentScope(const std::string& name);
    inline Symbol* lookupInCurrentScope(const char* name) { return lookupInCurrentScope(std::string(name)); };

    int64_t currentScopeDepth() const noexcept {
        return scopeDepth;
    }

    ~SymbolTable() = default;
};

}  // namespace semantic

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SYMBOL_TABLE_H