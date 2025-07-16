/**
 * @file symbol_table.h
 * @brief Contains the definition of the TypeInfo struct and PrimitiveType enum.
 *
 * Thanks to https://pgrandinetti.github.io/compilers/page/how-to-design-semantic-analysis/ for help on the high-level design of the semantic analysis phase.
 */

#ifndef MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SYMBOL_TABLE_H
#define MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SYMBOL_TABLE_H

#include <frontend/ast.h>
#include <io/logging.h>

#include <optional>
#include <vector>
#include <string>

namespace Manganese {
namespace semantic {

enum class PrimitiveType {
    Int,
    Uint,
    Float,
    Char,
    Bool,
    Bundle,
    Invalid = -1
};

struct TypeInfo {
    std::string name;  // e.g. "int", "float", "myBundle", etc.
    bool isConst = false;
    PrimitiveType primitiveType;
    int pointerDepth = 0;  // e.g. 1 = int*, 2 = int**, etc.
    bool isArray = false;
    std::optional<int> arraySize = std::nullopt;
    int bitWidth = 32;
};

enum class SymbolType {
    Variable,
    Constant,
    Parameter,
    Function,
    Enum,
    Bundle,
    TypeAlias,
};

struct Symbol {
    std::string name;
    SymbolType type;
    TypeInfo typeInfo;
    ast::Visibility visibility = ast::Visibility::Private;
    int scopeDepth;  // The depth of the scope in which this symbol is defined (0 = global)
    bool isInitialized = false;
    int line = -1, column = -1;
};

struct Scope {
    std::unordered_map<std::string, Symbol> symbols;

    inline bool insert(const Symbol& symbol) {
        return symbols.emplace(symbol.name, symbol).second;
    }

    Symbol* lookup(const std::string& name);

    inline Symbol* lookup(const char* name) {
        return lookup(std::string(name));
    }
};

class SymbolTable {
   private:
    std::vector<Scope> scopes;
    int scopeDepth = 0;

   public:
    SymbolTable() {
        enterScope();  // Start with a global scope
    }

    /**
     * @brief Create a new scope
     */
    void enterScope();

    /**
     * @brief Exit the current scope.
     */
    void exitScope();

    /**
     * @brief Declare a symbol in the current scope.
     */
    bool declare(Symbol symbol);

    /**
     * @brief Finds a symbol by name in the current scope and all higher scopes.
     * @return A pointer to the symbol if found, or nullptr if not found.
     */
    Symbol* lookup(const std::string& name);

    Symbol* currentScopeLookup(const std::string& name);

    int currentScopeDepth() const {
        return scopeDepth;
    }

    ~SymbolTable() = default;
};

}  // namespace semantic

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SYMBOL_TABLE_H