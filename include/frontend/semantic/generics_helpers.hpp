#ifndef MANGANESE_INCLUDE_FRONTEND_SEMANTIC_GENERICS_HELPERS_HPP
#define MANGANESE_INCLUDE_FRONTEND_SEMANTIC_GENERICS_HELPERS_HPP 1

#include <cstddef>
#include <frontend/ast/ast_base.hpp>
#include <frontend/semantic/type_context.hpp>
#include <functional>
#include <unordered_map>
#include <vector>

namespace Manganese::semantic {

struct generic_tag_t {
    constexpr explicit generic_tag_t(char) noexcept {}
};

constexpr inline generic_tag_t generic_tag{42};

struct InstantiationKey {
    const ast::ASTNode* declNode;
    std::vector<const SemanticType*> typeArgs;

    friend bool operator==(const InstantiationKey& a, const InstantiationKey& b) noexcept {
        return (a.declNode == b.declNode) && (a.typeArgs == b.typeArgs);
    }
};

struct InstantiationResult {
    ResolutionStatus state = ResolutionStatus::InProgress;
    const SemanticType* returnType = nullptr;
};

}  // namespace Manganese::semantic

namespace std {

template <>
struct hash<Manganese::semantic::InstantiationKey> {
    std::size_t operator()(const Manganese::semantic::InstantiationKey& key) const noexcept {
        std::size_t hash_value = std::hash<const Manganese::ast::ASTNode*>{}(key.declNode);
        for (const auto* type : key.typeArgs) {
            hash_value = Manganese::semantic::hash_combine(hash_value, std::hash<decltype(type)>{}(type));
        }
        return hash_value;
    }
};

}  // namespace std

namespace Manganese::semantic {

class InstantiationCache {
    std::unordered_map<InstantiationKey, InstantiationResult> _map;

   public:
    InstantiationCache() noexcept = default;

    const InstantiationResult* find(const InstantiationKey& key) const {
        if (auto it = _map.find(key); it != _map.end()) { return &(it->second); }
        return nullptr;
    }

    void markAsInProgress(const InstantiationKey& key) { _map.insert_or_assign(key, InstantiationResult{}); }
    void markAsSuccess(const InstantiationKey& key, const SemanticType* returnType) {
        _map.insert_or_assign(key, InstantiationResult{.state = ResolutionStatus::Success, .returnType = returnType});
    }
    void markAsFailure(const InstantiationKey& key) {
        _map.insert_or_assign(key, InstantiationResult{.state = ResolutionStatus::Failure, .returnType = nullptr});
    }

    bool contains(const InstantiationKey& key) const { return _map.contains(key); }
};

}  // namespace Manganese::semantic

#endif  // MANGANESE_INCLUDE_FRONTEND_SEMANTIC_GENERICS_HELPERS_HPP