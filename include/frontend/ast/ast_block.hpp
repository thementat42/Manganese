#ifndef MANGANESE_INCLUDE_FRONTEND_AST_AST_BLOCK_HPP
#define MANGANESE_INCLUDE_FRONTEND_AST_AST_BLOCK_HPP

#include <vector>

#include "ast_base.hpp"

namespace Manganese {
namespace ast {
class Block {
    using T = StatementUPtr_t;
    using size_type = std::vector<T>::size_type;
    public:
    std::vector<StatementUPtr_t> __block__;
    TypeSPtr_t blockType;

    // Wrappers around std::vector methods
    // This is partially for backwards compatibility, partially for ease of use
    // Use FORCE_INLINE since these are just wrappers and can very easily be substituted
    // Note: The commented out implementations are included for future use. They aren't currently used
    // but are available if need be

    //~ Element Access
    constexpr FORCE_INLINE T& operator[](size_type pos) { return __block__[pos]; }
    constexpr FORCE_INLINE const T& operator[](size_type pos) const { return __block__[pos]; }

    // constexpr FORCE_INLINE T& front() { return __block__.front(); }
    // constexpr FORCE_INLINE const T& front() const { return __block__.front(); }
    // constexpr FORCE_INLINE T& back() { return __block__.back(); }
    // constexpr FORCE_INLINE const T& back() const { return __block__.back(); }

    // constexpr FORCE_INLINE T* data() { return __block__.data(); }
    // constexpr FORCE_INLINE const T* data() const { return __block__.data(); }

    //~ Iterators
    constexpr FORCE_INLINE auto begin() noexcept { return __block__.begin(); }
    constexpr FORCE_INLINE auto begin() const noexcept { return __block__.begin(); }
    // constexpr FORCE_INLINE auto cbegin() const noexcept { return __block__.cbegin(); }

    constexpr FORCE_INLINE auto end() noexcept { return __block__.end(); }
    constexpr FORCE_INLINE auto end() const noexcept { return __block__.end(); }
    // constexpr FORCE_INLINE auto cend() const noexcept { return __block__.cend(); }

    constexpr FORCE_INLINE auto rbegin() noexcept { return __block__.rbegin(); }
    constexpr FORCE_INLINE auto rbegin() const noexcept { return __block__.rbegin(); }
    // constexpr FORCE_INLINE auto crbegin() const noexcept { return __block__.crbegin(); }

    constexpr FORCE_INLINE auto rend() noexcept { return __block__.rend(); }
    constexpr FORCE_INLINE auto rend() const noexcept { return __block__.rend(); }
    // constexpr FORCE_INLINE auto crend() const noexcept { return __block__.crend(); }

    // ~ Capacity
    constexpr FORCE_INLINE bool empty() const noexcept { return __block__.empty(); }
    constexpr FORCE_INLINE size_type size() const noexcept { return __block__.size(); }
    // constexpr FORCE_INLINE size_type max_size() const noexcept { return __block__.max_size(); }
    // constexpr FORCE_INLINE void reserve(size_type new_capacity) { __block__.reserve(new_capacity); }
    // constexpr FORCE_INLINE size_type capacity() const noexcept { return __block__.capacity(); }
    constexpr FORCE_INLINE void shrink_to_fit() { __block__.shrink_to_fit(); }

    //~ Modifiers
    // constexpr FORCE_INLINE void clear() noexcept { __block__.clear(); }
    constexpr FORCE_INLINE void push_back(const T& value) = delete;  // Can't copy a unique_ptr
    constexpr FORCE_INLINE void push_back(T&& value) { __block__.push_back(std::move(value)); }

    /*
    template <class ... Args>
    constexpr FORCE_INLINE T& emplace_back(Args&&... args) {
        return block.emplace_back(std::forward(args...));
    }
    */

    // constexpr FORCE_INLINE void pop_back() { __block__.pop_back(); }
};
}  // namespace ast

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_AST_BLOCK_HPP