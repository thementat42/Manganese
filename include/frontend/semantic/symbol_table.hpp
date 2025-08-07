#ifndef MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SYMBOL_TABLE_HPP
#define MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SYMBOL_TABLE_HPP

#include <frontend/ast.hpp>
#include <frontend/parser.hpp>
#include <global_macros.hpp>
#include <string>
#include <unordered_map>


namespace Manganese {

namespace semantic {

enum class SymbolKind {
    Variable,
    Constant,
    Function,
    FunctionParameter,
    ConstantFunctionParameter,
    Aggregate,
    Enum,
    Module,
    Import,
    TypeAlias,
    GenericType,
    Invalid = -1
};

struct Symbol {
    // === Identity Info ===
    std::string name;  // e.g. "myInt", "myAggregate"
    SymbolKind kind;
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
    inline bool insert(const Symbol symbol) noexcept { return symbols.emplace(symbol.name, symbol).second; }

    const Symbol* lookup(const std::string& name) const noexcept {
        auto it = symbols.find(name);
        return (it != symbols.end()) ? &it->second : nullptr;
    }

    inline const Symbol* lookup(const char* name) const noexcept { return lookup(std::string(name)); }
};

class SymbolTable {
   private:
    std::vector<Scope> scopes;
    int64_t scopeDepth = 0;

   public:
    SymbolTable() noexcept { enterScope(); }

    void enterScope();

    void exitScope() noexcept;

    bool declare(Symbol symbol);

    const Symbol* lookup(const std::string& name) const noexcept;
    inline const Symbol* lookup(const char* name) const noexcept { return lookup(std::string(name)); }
    const Symbol* lookupInCurrentScope(const std::string& name) const noexcept;
    inline const Symbol* lookupInCurrentScope(const char* name) const noexcept {
        return lookupInCurrentScope(std::string(name));
    };
    const Symbol* lookupAtDepth(const std::string& name, int64_t depth) const noexcept;
    inline const Symbol* lookupAtDepth(const char* name, int64_t depth) const noexcept {
        return lookupAtDepth(std::string(name), depth);
    }

    int64_t currentScopeDepth() const noexcept { return scopeDepth; }

    ~SymbolTable() = default;
};

}  // namespace semantic

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_SEMANTIC_SYMBOL_TABLE_HPP