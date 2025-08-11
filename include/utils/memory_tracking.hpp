

/**
 * @file memory_tracking.hpp
 * @brief Provides memory allocation tracking utilities for debugging and profiling.
 *
 * This header overrides global new and delete operators to track memory allocations and deallocations.
 * When MEMORY_TRACKING and DEBUG are enabled (via CMake), the code tracks the total bytes allocated during the
 * program's lifetime If CONTINUOUS_MEMORY_TRACKING is defined as well, each allocation is logged to a file
 */
#ifndef MANGANESE_INCLUDE_UTILS_MEMORY_TRACKING_HPP
#define MANGANESE_INCLUDE_UTILS_MEMORY_TRACKING_HPP

#include <global_macros.hpp>
#include <io/logging.hpp>

// MEMORY_TRACKING is defined in CMakeLists.txt

#if MEMORY_TRACKING && DEBUG
#include <fstream>
#include <iostream>

size_t lifetimeBytesAllocated = 0;  // How much memory has been allocated in total (ignores deallocations)
#ifdef CONTINUOUS_MEMORY_TRACKING
size_t bytesCurrentlyAllocated = 0;  // How much memory is currently allocated (accounts for deallocations)
std::ofstream memoryLogFile("logs/memory_tracking.log", std::ios::out | std::ios::trunc);
#endif  // CONTINUOUS_MEMORY_TRACKING

void* operator new(size_t size) {
    void* ptr = malloc(size);
    if (ptr == nullptr) { throw std::bad_alloc(); }
    lifetimeBytesAllocated += size;
#ifdef CONTINUOUS_MEMORY_TRACKING
    bytesCurrentlyAllocated += size;
    if (memoryLogFile.is_open()) {
        memoryLogFile << "Allocated " << size << " bytes at " << ptr
                      << " (estimation of total memory currently allocated: " << bytesCurrentlyAllocated << " bytes)"
                      << '\n';
    }
#endif  // CONTINUOUS_MEMORY_TRACKING
    return ptr;
}

void* operator new[](size_t size) {
    void* ptr = malloc(size);
    if (ptr == nullptr) { throw std::bad_alloc(); }
    lifetimeBytesAllocated += size;
#ifdef CONTINUOUS_MEMORY_TRACKING
    bytesCurrentlyAllocated += size;
    if (memoryLogFile.is_open()) {
        memoryLogFile << "Allocated " << size << " bytes at " << ptr
                      << " (estimation of total memory currently allocated: " << bytesCurrentlyAllocated << " bytes)"
                      << '\n';
    }
#endif  // CONTINUOUS_MEMORY_TRACKING
    return ptr;
}

void operator delete(void* ptr, size_t size) noexcept {
    if (ptr != nullptr) {
#ifdef CONTINUOUS_MEMORY_TRACKING
        bytesCurrentlyAllocated -= size;
        if (memoryLogFile.is_open()) {
            memoryLogFile << "Deallocated " << size << " bytes at " << ptr
                          << " (estimation of total memory currently allocated: " << bytesCurrentlyAllocated
                          << " bytes)" << '\n';
        }
#endif  // CONTINUOUS_MEMORY_TRACKING
        free(ptr);
        DISCARD(size);  // Avoid unused parameter warning
    }
}

void operator delete[](void* ptr, size_t size) noexcept {
    if (ptr != nullptr) {
#ifdef CONTINUOUS_MEMORY_TRACKING
        bytesCurrentlyAllocated -= size;
        if (memoryLogFile.is_open()) {
            memoryLogFile << "Deallocated " << size << " bytes at " << ptr
                          << " (estimation of total memory currently allocated: " << bytesCurrentlyAllocated
                          << " bytes)" << '\n';
        }
#endif  // CONTINUOUS_MEMORY_TRACKING
        free(ptr);
        DISCARD(size);  // Avoid unused parameter warning
    }
}

void operator delete(void* ptr) noexcept {
    if (ptr != nullptr) {
#ifdef CONTINUOUS_MEMORY_TRACKING
        if (memoryLogFile.is_open()) {
            memoryLogFile << "Deallocated (unknown size) at " << ptr
                          << " (cannot calculate number of bytes deallocated on a call to delete(void*))" << '\n';
        }
#endif  // CONTINUOUS_MEMORY_TRACKING
        free(ptr);
    }
}

void operator delete[](void* ptr) noexcept {
    if (ptr != nullptr) {
#ifdef CONTINUOUS_MEMORY_TRACKING
        if (memoryLogFile.is_open()) {
            memoryLogFile << "Deallocated (unknown size) at " << ptr
                          << " (cannot calculate number of bytes deallocated on a call to delete(void*))" << '\n';
        }
#endif  // CONTINUOUS_MEMORY_TRACKING
        free(ptr);
    }
}
#endif  // MEMORY_TRACKING && DEBUG
inline void logTotalAllocatedMemory() {
#if MEMORY_TRACKING && DEBUG
    std::cout << PINK << "Total memory allocated (over the course of the program): " << lifetimeBytesAllocated
              << " bytes" << RESET << '\n';
#ifdef CONTINUOUS_MEMORY_TRACKING
    memoryLogFile.close();
#endif  // CONTINUOUS_MEMORY_TRACKING
#endif  // MEMORY_TRACKING && DEBUG
}

#endif  // MANGANESE_INCLUDE_UTILS_MEMORY_TRACKING_HPP