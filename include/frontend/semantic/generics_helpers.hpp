#ifndef MANGANESE_INCLUDE_FRONTEND_SEMANTIC_GENERICS_HELPERS_HPP
#define MANGANESE_INCLUDE_FRONTEND_SEMANTIC_GENERICS_HELPERS_HPP 1

#include <cstddef>
#include <frontend/ast/ast_base.hpp>
#include <frontend/semantic/type_context.hpp>
#include <functional>
#include <unordered_map>
#include <utils/resolution_status.hpp>
#include <utils/string_interner.hpp>

namespace Manganese::semantic {
struct Scope;

struct InstantiationKey {
    const ast::ASTNode* declNode;
    TypeList typeArgs;

    friend constexpr bool operator==(const InstantiationKey&, const InstantiationKey&) noexcept = default;
};

struct InstantiationResult {
    ResolutionStatus state = ResolutionStatus::InProgress;
    const SemanticType* semanticType = nullptr;  // for functions
    utils::StringID mangledName;
    ast::Statement* clonedNode;
};

struct _insthasher {
    constexpr std::size_t operator()(const InstantiationKey& key) const noexcept {
        std::size_t hash_value = std::hash<const ast::ASTNode*>{}(key.declNode);
        for (const auto* type : key.typeArgs) {
            hash_value = hash_combine(hash_value, std::hash<decltype(type)>{}(type));
        }
        return hash_value;
    }
};

class InstantiationCache {
    std::unordered_map<InstantiationKey, InstantiationResult, _insthasher> _map;

   public:
    InstantiationCache() noexcept = default;

    const InstantiationResult* find(const InstantiationKey& key) const {
        if (auto it = _map.find(key); it != _map.end()) { return &(it->second); }
        return nullptr;
    }

    void markAsInProgress(const InstantiationKey& key) { _map.insert_or_assign(key, InstantiationResult{}); }
    void markAsSuccess(const InstantiationKey& key, const SemanticType* semanticType, utils::StringID mangledName,
                       ast::Statement* clonedNode) {
        _map.insert_or_assign(key,
                              InstantiationResult{.state = ResolutionStatus::Success,
                                                  .semanticType = semanticType,
                                                  .mangledName = mangledName,
                                                  .clonedNode = clonedNode});
    }
    void markAsFailure(const InstantiationKey& key) {
        _map.insert_or_assign(
            key,
            InstantiationResult{
                .state = ResolutionStatus::Failure, .semanticType = nullptr, .mangledName = {}, .clonedNode = nullptr});
    }

    bool contains(const InstantiationKey& key) const { return _map.contains(key); }
};

}  // namespace Manganese::semantic

#endif  // MANGANESE_INCLUDE_FRONTEND_SEMANTIC_GENERICS_HELPERS_HPP