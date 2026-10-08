#ifndef MANGANESE_INCLUDE_FRONTEND_SEMANTIC_CLONE_CONTEXT_HPP
#define MANGANESE_INCLUDE_FRONTEND_SEMANTIC_CLONE_CONTEXT_HPP 1

#include <frontend/semantic/type_context.hpp>
#include <mnstl/chunk_allocator.hxx>
#include <unordered_map>
#include <utils/string_interner.hpp>

namespace Manganese {
namespace ast {
struct Declaration;
}

namespace semantic {

struct CloneContext {
    mnstl::chunk_allocator& arena;

    std::unordered_map<utils::StringID, const SemanticType*> substitutions;
    std::unordered_map<const ast::Declaration*, ast::Declaration*> declarationSubstitutions;
};
}  // namespace semantic

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_SEMANTIC_CLONE_CONTEXT_HPP