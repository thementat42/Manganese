/**
 * @file manganese_tests.cpp
 * @brief Entry point for the test suit
 *
 * This file is responsible for calling all the test functions
 *
 * @note Run CMake with -DBUILD_TESTS=ON
 */

#include <frontend/lexer.hpp>
#include <global_macros.hpp>
#include <io/filereader.hpp>
#include <io/logging.hpp>
#include <io/stringreader.hpp>
#include <stdio.h>
#include <string.h>
#include <utils/memory_tracking.hpp>

#include <chrono>
#include <filesystem>

#include "tests/testrunner.hpp"
#include "tests/tests.hpp"

/**
 * @brief Checks if two C strings (a and b) are equal
 * @brief up to max_count characters and are the same length
 */
bool strneq(const char* a, const char* b, size_t max_count) {
    return (strncmp(a, b, max_count) == 0) && (strlen(a) == strlen(b));
}

int main(int argc, char const* argv[]) {
    if (argc == 1) {
        fprintf(stderr, "Usage: %s [--lexer] [--parser] [--semantic] [--codegen] [--all]\n", argv[0]);
        return 1;
    }

    auto start = std::chrono::high_resolution_clock::now();

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
    std::filesystem::create_directories("logs");

    Manganese::tests::TestRunner runner;

    if (lexer) {
        printf("%s=== Lexer Tests ===%s\n", PINK, RESET);
        Manganese::tests::runLexerTests(runner);
        printf("\n");
    }
    if (parser) {
        printf("%s=== Parser Tests ===%s\n", PINK, RESET);
        Manganese::tests::runParserTests(runner);
        printf("\n");
    }
    if (semantic) {
        printf("%s=== Semantic Analyzer Tests ===%s\n", PINK, RESET);
        Manganese::tests::runSemanticAnalysisTests(runner);
        printf("\n");
    }
    if (codegen) {
        printf("%s=== Codegen Tests ===%s\n", PINK, RESET);
        // TODO: Add once codegen done
        printf("To be implemented.\n");
        printf("\n");
    }

    logTotalAllocatedMemory();  // Only does something if memory tracking is enabled
    runner.printSummary();
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    printf("%sElapsed Time: %.3f ms%s\n", PINK, (double)duration.count(), RESET);

    return runner.allTestsPassed() ? 0 : 1;
}
