/* Copyright 2017 https://github.com/mandreyel
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies
 * or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef MN_MIO_MMAP_HEADER
#define MN_MIO_MMAP_HEADER

// #include "mio/page.hpp"
/* Copyright 2017 https://github.com/mandreyel
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies
 * or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iterator>
#include <system_error>
#include <utility>

#ifndef MN_MIO_PAGE_HEADER
#define MN_MIO_PAGE_HEADER

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

/*
 * This is a modified version of mandreyel's mio.hpp, stripped down for Manganese's use case and cleaned up for C++20
 * This only reads from files, it doesn't write to them (which eliminates the threading logic)
 * use std::filesystem::path, constexpr and requires
 * the original version is in mio.hpp
 */

namespace Manganese::io::mio {

/**
 * Determines the operating system's page allocation granularity.
 */
inline size_t page_size() {
    static const size_t page_size_val = [] {
#ifdef _WIN32
        SYSTEM_INFO system_info;
        GetSystemInfo(&system_info);
        return system_info.dwAllocationGranularity;
#else
        return sysconf(_SC_PAGE_SIZE);
#endif
    }();
    return page_size_val;
}

/**
 * Aligns `offset` to the operating system's page size.
 */
inline size_t make_offset_page_aligned(size_t offset) noexcept {
    const size_t page_sz = page_size();
    return offset / page_sz * page_sz;
}

}  // namespace Manganese::io::mio

#endif  // MN_MIO_PAGE_HEADER

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif  // WIN32_LEAN_AND_MEAN
#include <windows.h>
#else  // ifdef _WIN32
#define INVALID_HANDLE_VALUE -1
#endif  // ifdef _WIN32

namespace Manganese::io::mio {

// This value may be provided as the `length` parameter to the constructor or `map`.
constexpr inline size_t map_entire_file = 0;

#ifdef _WIN32
using file_handle_type = HANDLE;
#else
using file_handle_type = int;
#endif

const static file_handle_type invalid_handle = INVALID_HANDLE_VALUE;

namespace detail {
class FileHandle {
    file_handle_type _handle = invalid_handle;
    bool _auto_close = false;

   public:
    constexpr FileHandle() noexcept = default;
    explicit FileHandle(file_handle_type h, bool auto_close) noexcept : _handle(h), _auto_close(auto_close) {}

    ~FileHandle() { close(); }

    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;

    FileHandle(FileHandle&& other) noexcept :
        _handle(std::exchange(other._handle, invalid_handle)), _auto_close(std::exchange(other._auto_close, false)) {}

    FileHandle& operator=(FileHandle&& other) noexcept {
        if (this != &other) {
            close();
            _handle = std::exchange(other._handle, invalid_handle);
            _auto_close = std::exchange(other._auto_close, false);
        }
        return *this;
    }

    void reset(file_handle_type h = invalid_handle, bool auto_close = false) noexcept {
        close();
        _handle = h;
        _auto_close = auto_close;
    }

    file_handle_type get() const noexcept { return _handle; }
    bool is_valid() const noexcept { return _handle != invalid_handle; }

    void close() noexcept {
        if (is_valid() && _auto_close) {
#ifdef _WIN32
            ::CloseHandle(_handle);
#else
            ::close(_handle);
#endif
        }
        _handle = invalid_handle;
        _auto_close = false;
    }

    operator file_handle_type() const noexcept { return _handle; }
};

#ifdef _WIN32
namespace win {

inline DWORD int64_high(int64_t n) noexcept { return static_cast<DWORD>(n >> 32); }
inline DWORD int64_low(int64_t n) noexcept { return n & 0xffffffff; }

inline file_handle_type open_file_helper(const std::filesystem::path& path) {
    return ::CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL, nullptr);
}

}  // namespace win
#endif

inline std::error_code last_error() noexcept {
    std::error_code error;
#ifdef _WIN32
    error.assign(static_cast<int>(GetLastError()), std::system_category());
#else
    error.assign(errno, std::system_category());
#endif
    return error;
}

inline file_handle_type open_file(const std::filesystem::path& path, std::error_code& error) {
    error.clear();
    if (path.empty()) {
        error = std::make_error_code(std::errc::invalid_argument);
        return invalid_handle;
    }
#ifdef _WIN32
    auto* const handle = win::open_file_helper(path);
#else  // POSIX
    const auto handle = ::open(path.c_str(), O_RDONLY);
#endif
    if (handle == invalid_handle) { error = last_error(); }
    return handle;
}

inline size_t query_file_size(file_handle_type handle, std::error_code& error) {
    error.clear();
#ifdef _WIN32
    LARGE_INTEGER file_size;
    if (::GetFileSizeEx(handle, &file_size) == 0) {
        error = last_error();
        return 0;
    }
    return static_cast<size_t>(file_size.QuadPart);
#else  // POSIX
    struct stat sbuf;
    if (::fstat(handle, &sbuf) == -1) {
        error = last_error();
        return 0;
    }
    return sbuf.st_size;
#endif
}

struct mmap_context {
    char* data;
    int64_t length;
    int64_t mapped_length;
#ifdef _WIN32
    file_handle_type file_mapping_handle;
#endif
};

inline mmap_context memory_map(const file_handle_type file_handle, const int64_t offset, const int64_t length,
                               std::error_code& error) {
    const auto aligned_offset = static_cast<int64_t>(make_offset_page_aligned(static_cast<size_t>(offset)));
    const int64_t length_to_map = offset - aligned_offset + length;
#ifdef _WIN32
    const int64_t max_file_size = offset + length;
    auto* const file_mapping_handle = ::CreateFileMapping(
        file_handle, nullptr, PAGE_READONLY, win::int64_high(max_file_size), win::int64_low(max_file_size), nullptr);
    if (file_mapping_handle == invalid_handle) {
        error = last_error();
        return {};
    }
    char* mapping_start
        = static_cast<char*>(::MapViewOfFile(file_mapping_handle, FILE_MAP_READ, win::int64_high(aligned_offset),
                                             win::int64_low(aligned_offset), static_cast<SIZE_T>(length_to_map)));
    if (mapping_start == nullptr) {
        ::CloseHandle(file_mapping_handle);
        error = last_error();
        return {};
    }
#else  // POSIX
    char* mapping_start
        = static_cast<char*>(::mmap(0, length_to_map, PROT_READ, MAP_SHARED, file_handle, aligned_offset));
    if (mapping_start == MAP_FAILED) {
        error = last_error();
        return {};
    }
#endif
    mmap_context ctx;
    ctx.data = mapping_start + offset - aligned_offset;
    ctx.length = length;
    ctx.mapped_length = length_to_map;
#ifdef _WIN32
    ctx.file_mapping_handle = file_mapping_handle;
#endif

    return ctx;
}

#ifdef _WIN32
class FileMappingHandle {
    HANDLE _handle = INVALID_HANDLE_VALUE;

   public:
    constexpr FileMappingHandle() noexcept = default;
    explicit FileMappingHandle(HANDLE h) noexcept : _handle(h) {}

    ~FileMappingHandle() { close(); }

    FileMappingHandle(const FileMappingHandle&) = delete;
    FileMappingHandle& operator=(const FileMappingHandle&) = delete;

    FileMappingHandle(FileMappingHandle&& other) noexcept :
        _handle(std::exchange(other._handle, INVALID_HANDLE_VALUE)) {}

    FileMappingHandle& operator=(FileMappingHandle&& other) noexcept {
        if (this != &other) {
            close();
            _handle = std::exchange(other._handle, INVALID_HANDLE_VALUE);
        }
        return *this;
    }

    void reset(HANDLE h = INVALID_HANDLE_VALUE) noexcept {
        close();
        _handle = h;
    }

    [[nodiscard]] HANDLE get() const noexcept { return _handle; }
    [[nodiscard]] bool is_valid() const noexcept { return _handle != INVALID_HANDLE_VALUE; }

    void close() noexcept {
        if (is_valid()) { ::CloseHandle(_handle); }
        _handle = INVALID_HANDLE_VALUE;
    }

    operator HANDLE() const noexcept { return _handle; }
};
#endif

}  // namespace detail

template <class ByteT>
    requires(std::is_trivial_v<ByteT> && (sizeof(ByteT) == sizeof(char)))
struct basic_mmap {
    using value_type = ByteT;
    using size_type = size_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using difference_type = std::ptrdiff_t;
    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using iterator_category = std::random_access_iterator_tag;
    using handle_type = file_handle_type;

   private:
    pointer _data = nullptr;
    size_type _length = 0;
    size_type _mapped_length = 0;
    detail::FileHandle _file_handle;
#ifdef _WIN32
    detail::FileMappingHandle _file_mapping_handle;
#endif

   public:
    basic_mmap() noexcept = default;

#ifdef __cpp_exceptions
    explicit basic_mmap(const std::filesystem::path& path, const size_type offset = 0,
                        const size_type length = map_entire_file) {
        std::error_code error;
        map(path, offset, length, error);
        if (error) [[unlikely]] { throw std::system_error(error); }
    }

    explicit basic_mmap(const handle_type handle, const size_type offset = 0,
                        const size_type length = map_entire_file) {
        std::error_code error;
        map(handle, offset, length, error);
        if (error) [[unlikely]] { throw std::system_error(error); }
    }
#endif  // __cpp_exceptions

    basic_mmap(const basic_mmap&) = delete;
    basic_mmap& operator=(const basic_mmap&) = delete;
    basic_mmap(basic_mmap&&) noexcept = default;
    basic_mmap& operator=(basic_mmap&&) noexcept = default;

    ~basic_mmap() { unmap(); }

    handle_type file_handle() const noexcept { return _file_handle.get(); }

    handle_type mapping_handle() const noexcept {
#ifdef _WIN32
        return _file_mapping_handle.get();
#else
        return _file_handle;
#endif
    }

    [[nodiscard]] bool is_open() const noexcept { return _file_handle.is_valid(); }
    [[nodiscard]] bool empty() const noexcept { return length() == 0; }

    [[nodiscard]] bool is_mapped() const noexcept {
#ifdef _WIN32
        return _file_mapping_handle.is_valid();
#else
        return is_open();
#endif
    }

    [[nodiscard]] size_type size() const noexcept { return length(); }
    [[nodiscard]] size_type length() const noexcept { return _length; }
    [[nodiscard]] size_type mapped_length() const noexcept { return _mapped_length; }
    [[nodiscard]] size_type mapping_offset() const noexcept { return _mapped_length - _length; }

    const_pointer data() const noexcept { return _data; }

    const_iterator begin() const noexcept { return data(); }
    const_iterator cbegin() const noexcept { return data(); }

    const_iterator end() const noexcept { return data() + length(); }
    const_iterator cend() const noexcept { return data() + length(); }
    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }

    const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }

    reference operator[](const size_type i) noexcept { return _data[i]; }
    const_reference operator[](const size_type i) const noexcept { return _data[i]; }

    void map(const std::filesystem::path& path, size_type offset, size_type length, std::error_code& error) {
        error.clear();
        if (path.empty()) {
            error = std::make_error_code(std::errc::invalid_argument);
            return;
        }
        file_handle_type raw_handle = detail::open_file(path, error);
        if (error) [[unlikely]] { return; }

        map(raw_handle, offset, length, error);
        if (!error) {
            _file_handle.reset(raw_handle, true);
        } else {
#ifdef _WIN32
            ::CloseHandle(raw_handle);
#else
            ::close(raw_handle);
#endif
        }
    }

    void map(const std::filesystem::path& path, std::error_code& error) { map(path, 0, map_entire_file, error); }

    void map(const handle_type handle, size_type offset, size_type length, std::error_code& error) {
        error.clear();
        if (handle == invalid_handle) {
            error = std::make_error_code(std::errc::bad_file_descriptor);
            return;
        }

        const auto file_size = detail::query_file_size(handle, error);
        if (error) [[unlikely]] { return; }

        if (offset + length > file_size) {
            error = std::make_error_code(std::errc::invalid_argument);
            return;
        }

        const auto ctx = detail::memory_map(
            handle, static_cast<int64_t>(offset),
            length == map_entire_file ? static_cast<int64_t>(file_size - offset) : static_cast<int64_t>(length), error);
        if (!error) {
            unmap();
            _file_handle.reset(handle, false);
            _data = reinterpret_cast<pointer>(ctx.data);
            _length = ctx.length;
            _mapped_length = ctx.mapped_length;
#ifdef _WIN32
            _file_mapping_handle.reset(ctx.file_mapping_handle);
#endif
        }
    }

    void map(const handle_type handle, std::error_code& error) { map(handle, 0, map_entire_file, error); }

    void unmap() {
        if (!is_open()) { return; }
#ifdef _WIN32
        if (is_mapped()) {
            ::UnmapViewOfFile(get_mapping_start());
            _file_mapping_handle.close();  // Cleanly closes and resets via RAII
        }
#else
        if (_data) { ::munmap(const_cast<pointer>(get_mapping_start()), _mapped_length); }
#endif

        _file_handle.reset();

        _data = nullptr;
        _length = _mapped_length = 0;
    }

    void swap(basic_mmap& other) noexcept {
        if (this != &other) {
            using std::swap;
            swap(_data, other._data);
            swap(_file_handle, other._file_handle);
#ifdef _WIN32
            swap(_file_mapping_handle, other._file_mapping_handle);
#endif
            swap(_length, other._length);
            swap(_mapped_length, other._mapped_length);
        }
    }

    const_pointer get_mapping_start() const noexcept { return !data() ? nullptr : data() - mapping_offset(); }

    friend bool operator==(const basic_mmap& a, const basic_mmap& b) {
        return a.data() == b.data() && a.size() == b.size();
    }

    friend std::strong_ordering operator<=>(const basic_mmap& a, const basic_mmap& b) {
        if (a.data() != b.data()) { return a.data() <=> b.data(); }
        return a.size() <=> b.size();
    }
};

template <class MMap, class MappingToken>
[[nodiscard]] MMap make_mmap(const MappingToken& token, int64_t offset, int64_t length, std::error_code& error) {
    MMap mmap;
    mmap.map(token, offset, length, error);
    return mmap;
}

}  // namespace Manganese::io::mio

#endif  // MN_MIO_MMAP_HEADER