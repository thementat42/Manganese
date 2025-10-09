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
    using block_t_ = std::vector<StatementUPtr_t>;
    block_t_ block_;

   public:
    using value_type = block_t_::value_type;
    using allocator_type = block_t_::allocator_type;
    using size_type = block_t_::size_type;
    using difference_type = block_t_::difference_type;
    using reference = block_t_::reference;
    using const_reference = block_t_::const_reference;
    using pointer = block_t_::pointer;
    using const_pointer = block_t_::const_pointer;
    using iterator = block_t_::iterator;
    using const_iterator = block_t_::const_iterator;
    using reverse_iterator = block_t_::reverse_iterator;
    using const_reverse_iterator = block_t_::const_reverse_iterator;

    TypeSPtr_t blockType;

   public:
    constexpr const block_t_& get_block() const noexcept { return block_; }
    constexpr block_t_& get_block() noexcept { return block_; }
    
    // Wrappers around std::vector methods
    // This is partially for backwards compatibility, partially for ease of use
    // Use FORCE_INLINE since these are just wrappers and can very easily be substituted
    // Note: The commented out implementations are included for future use. They aren't currently used
    // but are available if need be

    //~ Element Access
    constexpr FORCE_INLINE reference operator[](size_type pos) { return block_[pos]; }
    constexpr FORCE_INLINE const_reference operator[](size_type pos) const { return block_[pos]; }

    constexpr FORCE_INLINE reference front() { return block_.front(); }
    constexpr FORCE_INLINE const_reference front() const { return block_.front(); }
    constexpr FORCE_INLINE reference back() { return block_.back(); }
    constexpr FORCE_INLINE const_reference back() const { return block_.back(); }

    constexpr FORCE_INLINE pointer data() { return block_.data(); }
    constexpr FORCE_INLINE const_pointer data() const { return block_.data(); }

    //~ Iterators
    constexpr FORCE_INLINE auto begin() noexcept { return block_.begin(); }
    constexpr FORCE_INLINE auto begin() const noexcept { return block_.begin(); }
    constexpr FORCE_INLINE auto cbegin() const noexcept { return block_.cbegin(); }

    constexpr FORCE_INLINE auto end() noexcept { return block_.end(); }
    constexpr FORCE_INLINE auto end() const noexcept { return block_.end(); }
    constexpr FORCE_INLINE auto cend() const noexcept { return block_.cend(); }

    constexpr FORCE_INLINE auto rbegin() noexcept { return block_.rbegin(); }
    constexpr FORCE_INLINE auto rbegin() const noexcept { return block_.rbegin(); }
    constexpr FORCE_INLINE auto crbegin() const noexcept { return block_.crbegin(); }

    constexpr FORCE_INLINE auto rend() noexcept { return block_.rend(); }
    constexpr FORCE_INLINE auto rend() const noexcept { return block_.rend(); }
    constexpr FORCE_INLINE auto crend() const noexcept { return block_.crend(); }

    // ~ Capacity
    constexpr FORCE_INLINE bool empty() const noexcept { return block_.empty(); }
    constexpr FORCE_INLINE size_type size() const noexcept { return block_.size(); }
    constexpr FORCE_INLINE size_type max_size() const noexcept { return block_.max_size(); }
    constexpr FORCE_INLINE void reserve(size_type new_capacity) { block_.reserve(new_capacity); }
    constexpr FORCE_INLINE size_type capacity() const noexcept { return block_.capacity(); }
    constexpr FORCE_INLINE void shrink_to_fit() { block_.shrink_to_fit(); }

    //~ Modifiers
    constexpr FORCE_INLINE void clear() noexcept { block_.clear(); }
    constexpr FORCE_INLINE void push_back(const value_type& value) = delete;  // Can't copy a unique_ptr
    constexpr FORCE_INLINE void push_back(value_type&& value) { block_.push_back(std::move(value)); }

    template <class... Args>
    constexpr FORCE_INLINE reference emplace_back(Args&&... args) {
        return block_.emplace_back(std::forward(args...));
    }

    constexpr FORCE_INLINE void pop_back() { block_.pop_back(); }
};
}  // namespace ast

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_AST_BLOCK_HPP