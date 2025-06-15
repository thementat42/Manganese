/**
 * @file manganese_tests.cpp
 * @brief Main file for running all tests.
 * @note Run CMake with -DBUILD_TESTS=ON
 */

// TODO: Organize the memory tracking code better, maybe put it in a separate file

#if MEMORY_TRACKING && DEBUG

#include <fstream>
#include <iostream>

size_t lifetimeBytesAllocated = 0;  // How much memory has been allocated in total (ignores deallocations)
#ifdef CONTINUOUS_MEMORY_TRACKING
size_t bytesCurrentlyAllocated = 0;  // How much memory is currently allocated (accounts for deallocations)
std::ofstream memoryLogFile("memory_tracking.log", std::ios::out | std::ios::trunc);
#endif  // CONTINUOUS_MEMORY_TRACKING

void* operator new(size_t size) {
    void* ptr = malloc(size);
    if (ptr == nullptr) {
        throw std::bad_alloc();
    }
    lifetimeBytesAllocated += size;
#ifdef CONTINUOUS_MEMORY_TRACKING
    bytesCurrentlyAllocated += size;
    if (memoryLogFile.is_open()) {
        memoryLogFile << "Allocated " << size << " bytes at " << ptr << " (estimation of total memory currently allocated: "
                      << bytesCurrentlyAllocated << " bytes)" << '\n';
    }
#endif  // CONTINUOUS_MEMORY_TRACKING
    return ptr;
}

void* operator new[](size_t size) {
    void* ptr = malloc(size);
    if (ptr == nullptr) {
        throw std::bad_alloc();
    }
    lifetimeBytesAllocated += size;
#ifdef CONTINUOUS_MEMORY_TRACKING
    bytesCurrentlyAllocated += size;
    if (memoryLogFile.is_open()) {
        memoryLogFile << "Allocated " << size << " bytes at " << ptr << " (estimation of total memory currently allocated: "
                      << bytesCurrentlyAllocated << " bytes)" << '\n';
    }
#endif  // CONTINUOUS_MEMORY_TRACKING
    return ptr;
}

void operator delete(void* ptr, size_t size) noexcept {
    if (ptr != nullptr) {
#ifdef CONTINUOUS_MEMORY_TRACKING
        bytesCurrentlyAllocated -= size;
        if (memoryLogFile.is_open()) {
            memoryLogFile << "Deallocated " << size << " bytes at " << ptr << " (estimation of total memory currently allocated: "
                          << bytesCurrentlyAllocated << " bytes)" << '\n';
        }
#endif  // CONTINUOUS_MEMORY_TRACKING
        free(ptr);
        (void)size;  // Avoid unused parameter warning
    }
}

void operator delete[](void* ptr, size_t size) noexcept {
    if (ptr != nullptr) {
#ifdef CONTINUOUS_MEMORY_TRACKING
        bytesCurrentlyAllocated -= size;
        if (memoryLogFile.is_open()) {
            memoryLogFile << "Deallocated " << size << " bytes at " << ptr << " (estimation of total memory currently allocated: "
                          << bytesCurrentlyAllocated << " bytes)" << '\n';
        }
#endif  // CONTINUOUS_MEMORY_TRACKING
        free(ptr);
        (void)size;  // Avoid unused parameter warning
    }
}

void operator delete(void* ptr) noexcept {
    if (ptr != nullptr) {
#ifdef CONTINUOUS_MEMORY_TRACKING
        if (memoryLogFile.is_open()) {
            memoryLogFile << "Deallocated (unknown size) at " << ptr
                          << " (cannot calculate number of bytes deallocated on a call to delete(void*))"
                          << '\n';
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
                          << " (cannot calculate number of bytes deallocated on a call to delete(void*))"
                          << '\n';
        }
#endif  // CONTINUOUS_MEMORY_TRACKING
        free(ptr);
    }
}

#endif  // MEMORY_TRACKING && DEBUG

// Includes for unity build
#include <string.h>

#include <frontend/lexer.h>
#include <frontend/token.h>
#include <global_macros.h>
#include <io/filereader.h>
#include <io/logging.h>
#include <io/stringreader.h>
#include "tests/testrunner.h"
#include "tests/tests.h"

bool strneq(const char* a, const char* b, size_t max_count) {
    return (strncmp(a, b, max_count) == 0) && (strlen(a) == strlen(b));
}

int main(int argc, char const* argv[]) {
    if (argc == 1) {
        fprintf(stderr, "Usage: %s [--lexer] [--parser] [--semantic] [--codegen] [--all]\n", argv[0]);
        return 1;
    }

    bool lexer = false;
    bool parser = false;
    bool semantic = false;
    bool codegen = false;

    // TODO: Replace this with proper argument parser later (when working on argparser for main executable)

    for (int i = 1; i < argc; ++i) {
        if (strneq(argv[i], "--lexer", 7)) {
            lexer = true;
        } else if (strneq(argv[i], "--parser", 8)) {
            parser = true;
        } else if (strneq(argv[i], "--semantic", 10)) {
            semantic = true;
        } else if (strneq(argv[i], "--codegen", 9)) {
            codegen = true;
        } else if (strneq(argv[i], "--all", 5)) {
            lexer = true;
            parser = true;
            semantic = true;
            codegen = true;
            break;
        } else {
            fprintf(stderr, "Skipping unknown argument: %s\n", argv[i]);
        }
    }

    using namespace Manganese::tests;
    TestRunner runner;

    if (lexer) {
        printf("%s=== Lexer Tests ===%s\n", PINK, RESET);
        runLexerTests(runner);
        printf("\n");
    }
    if (parser) {
        printf("%s=== Parser Tests ===%s\n", PINK, RESET);
        runParserTests(runner);
        printf("\n");
    }
    if (semantic) {
        printf("%s=== Semantic Analyzer Tests ===%s\n", PINK, RESET);
        // TODO: Add once semantic analyzer done
        printf("\n");
    }
    if (codegen) {
        printf("%s=== Codegen Tests ===%s\n", PINK, RESET);
        // TODO: Add once codegen done
        printf("\n");
    }

    runner.printSummary();

#if MEMORY_TRACKING && DEBUG
    std::cout << PINK << "Total memory allocated (over the course of the program): " << lifetimeBytesAllocated << " bytes" << RESET << '\n';
#endif  // MEMORY_TRACKING && DEBUG
    return runner.allTestsPassed() ? 0 : 1;
}
