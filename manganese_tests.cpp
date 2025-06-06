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
#include "src/frontend/include/token.h"
#include "src/frontend/include/lexer.h"
#include "src/io/include/filereader.h"
#include "src/io/include/stringreader.h"
#include "tests/testrunner.h"
#include "tests/tests.h"

int main(int argc, char const *argv[]) {
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
        if (strcmp(argv[i], "--lexer") == 0) {
            lexer = true;
        } else if (strcmp(argv[i], "--parser") == 0) {
            parser = true;
        } else if (strcmp(argv[i], "--semantic") == 0) {
            semantic = true;
        } else if (strcmp(argv[i], "--codegen") == 0) {
            codegen = true;
        } else if (strcmp(argv[i], "--all") == 0) {
            lexer = true;
            parser = true;
            semantic = true;
            codegen = true;
            break;
        }
    }

    using namespace manganese::tests;
    TestRunner runner;
    if (lexer) {
        printf("=== Lexer Tests ===\n");
        runLexerTests(runner);
        printf("\n");
    }
    if (parser) {
        printf("=== Parser Tests ===\n");
        // TODO: Add once parser done
        printf("\n");
    }
    if (semantic) {
        printf("=== Semantic Analyzer Tests ===\n");
        // TODO: Add once semantic analyzer done
        printf("\n");
    }
    if (codegen) {
        printf("=== Codegen Tests ===\n");
        // TODO: Add once codegen done
        printf("\n");
    }

    runner.printSummary();
    return runner.allTestsPassed() ? 0 : 1;
}
