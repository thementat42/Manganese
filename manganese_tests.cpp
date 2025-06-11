/**
 * @file tests-main.cpp
 * @brief Main file for running all tests.
 * @note Run CMake with -DBUILD_TESTS=ON
 */

// Includes for unity build
#include <string.h>

#include "src/global_macros.h"

#undef DEBUG
#define DEBUG 1  // Force debug mode on
#include "src/frontend/include/lexer.h"
#include "src/frontend/include/token.h"
#include "src/io/include/filereader.h"
#include "src/io/include/stringreader.h"
#include "tests/testrunner.h"
#include "tests/tests.h"

constexpr bool strneq(const char* a, const char* b,size_t max_count) {
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

    using namespace manganese::tests;
    TestRunner runner;
    const char* pink = "\033[95m";
    const char* reset = "\033[0m";

    if (lexer) {
        printf("%s=== Lexer Tests ===%s\n", pink, reset);
        runLexerTests(runner);
        printf("\n");
    }
    if (parser) {
        printf("%s=== Parser Tests ===%s\n", pink, reset);
        // TODO: Add once parser done
        printf("\n");
    }
    if (semantic) {
        printf("%s=== Semantic Analyzer Tests ===%s\n", pink, reset);
        // TODO: Add once semantic analyzer done
        printf("\n");
    }
    if (codegen) {
        printf("%s=== Codegen Tests ===%s\n", pink, reset);
        // TODO: Add once codegen done
        printf("\n");
    }

    runner.printSummary();
    return runner.allTestsPassed() ? 0 : 1;
}
