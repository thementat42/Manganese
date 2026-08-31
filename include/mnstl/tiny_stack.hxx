#ifndef MNSTL_TINY_STACK
#define MNSTL_TINY_STACK 1

#include <cassert>
#include <cstddef>
#include <utility>
#include <vector>

namespace mnstl {

template <class T>
class tiny_stack {
   private:
    std::vector<T> _data;

   public:
    void push(const T&) = delete;

    void push(T&& element) { _data.push_back(std::move(element)); }

    void pop() noexcept {
        if (_data.empty()) { return; }  // for safety
        _data.pop_back();
    }

    [[nodiscard]] T& top() & noexcept {
        assert(!_data.empty() && "Attempted to access element of an empty mnstl::tiny_stack");
        return _data.back();
    }
    [[nodiscard]] const T& top() const& noexcept {
        assert(!_data.empty() && "Attempted to access element of an empty mnstl::tiny_stack");
        return _data.back();
    }

    [[nodiscard]] T&& top() && noexcept {
        assert(!_data.empty() && "Attempted to access element of an empty mnstl::tiny_stack");
        return std::move(_data.back());
    }
    [[nodiscard]] const T&& top() const&& noexcept {
        assert(!_data.empty() && "Attempted to access element of an empty mnstl::tiny_stack");
        return std::move(_data.back());
    }

    void reserve(std::size_t new_capacity) { _data.reserve(new_capacity); }
    [[nodiscard]] bool is_empty() const noexcept { return _data.empty(); }

};  // class tiny_stack

}  // namespace mnstl

#endif  // MNSTL_TINY_STACK