#ifndef MNSTL_CHUNK_ALLOCATOR
#define MNSTL_CHUNK_ALLOCATOR 1

#include <cstddef>
#include <frontend/ast.hpp>
#include <global_macros.hpp>
#include <memory>
#include <utility>
#include <vector>

namespace mnstl {

class chunk_allocator {
   private:
    constexpr static inline size_t _chunksize = 4096;
    struct chunk {
        std::unique_ptr<std::byte[]> data;
        size_t used, capacity;
    };
    constexpr static inline size_t _max(size_t a, size_t b) noexcept { return a > b ? a : b; }

    std::vector<chunk> _chunks;
    constexpr static uintptr_t align_up(uintptr_t ptr, uintptr_t alignment) noexcept {
        uintptr_t mask = alignment - 1;
        return (ptr + mask) & ~mask;
    }

    void add_chunk(size_t size = _chunksize) {
        _chunks.push_back(
            chunk{.data = std::make_unique_for_overwrite<std::byte[]>(size),  // avoids initialization of values
                  .used = 0,
                  .capacity = size});
    }

    FORCE_INLINE void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) {
    _do_allocation:
        chunk& c = _chunks[_chunks.size() - 1];
        uintptr_t current_position = reinterpret_cast<uintptr_t>(c.data.get() + c.used);
        // the next place we can safely construct a type, taking padding into account
        uintptr_t aligned_position = align_up(current_position, alignment);
        // how much room to leave before the next allocation
        uintptr_t adjustment = aligned_position - current_position;

        if (c.used + adjustment + size > c.capacity) {
            // can't fit data here anymore
            add_chunk(_max(_chunksize, size + alignment));
            goto _do_allocation;  // avoids recursion
        }
        c.used += adjustment;
        void* ptr = c.data.get() + c.used;
        c.used += size;

        return ptr;
    }

   public:
    chunk_allocator() { add_chunk(); }
    ~chunk_allocator() noexcept = default;

    template <class Node, class... Args>
        requires(std::is_convertible_v<Node*, Manganese::ast::ASTNode*> && std::is_constructible_v<Node, Args...>)
    Node* add_node(Args&&... args) {
        void* mem = allocate(sizeof(Node), alignof(Node));
        return new (mem) Node(std::forward<Args>(args)...);
    }
};

}  // namespace mnstl

#endif  // MNSTL_CHUNK_ALLOCATOR