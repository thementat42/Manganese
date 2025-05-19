/**
 * @file tests-main.cpp
 * @brief Main file for running all tests.
 * @details Uses a unity build to compile all test files. Specific tests are run based on command line arguments.
 */

// Includes for unity build
#include <string.h>

#include "src/frontend/lexer/keywords.cpp"
#include "src/frontend/lexer/lexer.cpp"
#include "src/frontend/lexer/operators.cpp"
#include "src/frontend/lexer/token.cpp"
#include "src/io/filereader.cpp"
#include "src/io/stringreader.cpp"
#include "tests/lexer_tests.cpp"
#include "tests/testrunner.cpp"

int main(int argc, char const *argv[]) {
    if (argc == 1) {
        fprintf(stderr, "Usage: %s [--lexer] [--parser] [--semantic] [--codegen] [--all]\n", argv[0]);
        return 1;
    }

    bool lexer = false;
    bool parser = false;
    bool semantic = false;
    bool codegen = false;
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

    using namespace Manganese::tests;
    TestRunner runner;
    if (lexer) {
        printf("=== Lexer Tests ===\n");
        runLexerTests(runner);
    }
    if (parser) {
        printf("=== Parser Tests ===\n");
        // TODO: Add once parser done
    }
    if (semantic) {
        printf("=== Semantic Analyzer Tests ===\n");
        // TODO: Add once semantic analyzer done
    }
    if (codegen) {
        printf("=== Codegen Tests ===\n");
        // TODO: Add once codegen done
    }

    runner.printSummary();
    return runner.allTestsPassed() ? 0 : 1;
}
