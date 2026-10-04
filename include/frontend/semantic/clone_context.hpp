#ifndef MANGANESE_INCLUDE_FRONTEND_SEMANTIC_CLONE_CONTEXT_HPP
#define MANGANESE_INCLUDE_FRONTEND_SEMANTIC_CLONE_CONTEXT_HPP 1

#include <frontend/semantic/type_context.hpp>
#include <functional>
#include <mnstl/chunk_allocator.hxx>
#include <unordered_map>

namespace Manganese::semantic {

struct CloneContext {
    mnstl::chunk_allocator& arena;

    // equal_to enables heterogeneous lookup
    std::unordered_map<std::string_view, const SemanticType*, std::hash<std::string_view>, std::equal_to<>>
        substitutions;
};

}  // namespace Manganese::semantic

#endif  // MANGANESE_INCLUDE_FRONTEND_SEMANTIC_CLONE_CONTEXT_HPP