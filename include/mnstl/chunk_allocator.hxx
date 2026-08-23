#ifndef MNSTL_CHUNK_ALLOCATOR
#define MNSTL_CHUNK_ALLOCATOR 1

#include <core.hpp>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace mnstl {

class chunk_allocator {
   private:
    constexpr static inline std::size_t _default_chunksize = 0x1000;
    struct chunk {
        std::unique_ptr<std::byte[]> data;
        std::size_t used, capacity;
    };
    constexpr static inline std::size_t _max(std::size_t a, std::size_t b) noexcept { return a > b ? a : b; }

    std::vector<chunk> _chunks;
    constexpr static std::uintptr_t align_up(std::uintptr_t ptr, std::uintptr_t alignment) noexcept {
        std::uintptr_t mask = alignment - 1;
        return (ptr + mask) & ~mask;
    }

    void add_chunk(std::size_t size = _default_chunksize) {
        _chunks.push_back(
            chunk{.data = std::make_unique_for_overwrite<std::byte[]>(size),  // avoids initialization of values
                  .used = 0,
                  .capacity = size});
    }

    FORCE_INLINE void* allocate(std::size_t size, std::size_t alignment) {
    _do_allocation:
        chunk& c = _chunks.back();
        const auto current_position = reinterpret_cast<std::uintptr_t>(c.data.get() + c.used);
        // the next place we can safely construct a type, taking padding into account
        const std::uintptr_t aligned_position = align_up(current_position, alignment);
        // how much room to leave before the next allocation
        const std::uintptr_t adjustment = aligned_position - current_position;

        if (c.used + adjustment + size > c.capacity) {
            // can't fit data here anymore
            add_chunk(_max(_default_chunksize, size + alignment));
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

    template <class T, class... Args>
        requires(std::is_constructible_v<T, Args...>)
    T* emplace(Args&&... args) {
        void* mem = allocate(sizeof(T), alignof(T));
        return new (mem) T(std::forward<Args>(args)...);
    }
};

}  // namespace mnstl

#endif  // MNSTL_CHUNK_ALLOCATOR