#ifndef MANGANESE_INCLUDE_FRONTEND_AST_AST_BLOCK_HPP
#define MANGANESE_INCLUDE_FRONTEND_AST_AST_BLOCK_HPP

#include <vector>

#include "ast_base.hpp"

namespace Manganese {
namespace ast {

/**
 * Block represents a series of statements (such as the body of a while loop, an if statement, etc.)
 * Block provides a vector-like interface (with the same methods and member types)
 */
class Block final {
   private:
    using block_t = std::vector<StatementUPtr_t>;
    block_t block;

   public:
    using value_type = block_t::value_type;
    using allocator_type = block_t::allocator_type;
    using size_type = block_t::size_type;
    using difference_type = block_t::difference_type;
    using reference = block_t::reference;
    using const_reference = block_t::const_reference;
    using pointer = block_t::pointer;
    using const_pointer = block_t::const_pointer;
    using iterator = block_t::iterator;
    using const_iterator = block_t::const_iterator;
    using reverse_iterator = block_t::reverse_iterator;
    using const_reverse_iterator = block_t::const_reverse_iterator;

    TypeSPtr_t blockType;

   public:
    constexpr const block_t& get_block() const noexcept { return block; }
    constexpr block_t& get_block() noexcept { return block; }

    // Wrappers around std::vector methods
    // This is partially for backwards compatibility, partially for ease of use
    // Use FORCE_INLINE since these are just wrappers and can very easily be substituted

    //~ Element Access
    constexpr FORCE_INLINE reference operator[](size_type pos) { return block[pos]; }
    constexpr FORCE_INLINE const_reference operator[](size_type pos) const { return block[pos]; }

    constexpr FORCE_INLINE reference front() { return block.front(); }
    constexpr FORCE_INLINE const_reference front() const { return block.front(); }
    constexpr FORCE_INLINE reference back() { return block.back(); }
    constexpr FORCE_INLINE const_reference back() const { return block.back(); }

    constexpr FORCE_INLINE pointer data() { return block.data(); }
    constexpr FORCE_INLINE const_pointer data() const { return block.data(); }

    //~ Iterators
    constexpr FORCE_INLINE iterator begin() noexcept { return block.begin(); }
    constexpr FORCE_INLINE const_iterator begin() const noexcept { return block.begin(); }
    constexpr FORCE_INLINE const_iterator cbegin() const noexcept { return block.cbegin(); }

    constexpr FORCE_INLINE iterator end() noexcept { return block.end(); }
    constexpr FORCE_INLINE const_iterator end() const noexcept { return block.end(); }
    constexpr FORCE_INLINE const_iterator cend() const noexcept { return block.cend(); }

    constexpr FORCE_INLINE reverse_iterator rbegin() noexcept { return block.rbegin(); }
    constexpr FORCE_INLINE const_reverse_iterator rbegin() const noexcept { return block.rbegin(); }
    constexpr FORCE_INLINE const_reverse_iterator crbegin() const noexcept { return block.crbegin(); }

    constexpr FORCE_INLINE reverse_iterator rend() noexcept { return block.rend(); }
    constexpr FORCE_INLINE const_reverse_iterator rend() const noexcept { return block.rend(); }
    constexpr FORCE_INLINE const_reverse_iterator crend() const noexcept { return block.crend(); }

    // ~ Capacity
    constexpr FORCE_INLINE bool empty() const noexcept { return block.empty(); }
    constexpr FORCE_INLINE size_type size() const noexcept { return block.size(); }
    constexpr FORCE_INLINE size_type max_size() const noexcept { return block.max_size(); }
    constexpr FORCE_INLINE void reserve(size_type new_capacity) { block.reserve(new_capacity); }
    constexpr FORCE_INLINE size_type capacity() const noexcept { return block.capacity(); }
    constexpr FORCE_INLINE void shrink_to_fit() { block.shrink_to_fit(); }

    //~ Modifiers
    constexpr FORCE_INLINE void clear() noexcept { block.clear(); }
    constexpr FORCE_INLINE void push_back(const value_type& value) = delete;  // Can't copy a unique_ptr
    constexpr FORCE_INLINE void push_back(value_type&& value) { block.push_back(std::move(value)); }

    template <class... Args>
    constexpr FORCE_INLINE reference emplace_back(Args&&... args) {
        return block.emplace_back(std::forward<Args>(args)...);
    }
};
}  // namespace ast

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_AST_BLOCK_HPP