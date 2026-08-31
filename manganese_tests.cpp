#include <chrono>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <frontend/lexer.hpp>
#include <io/filereader.hpp>
#include <io/logging.hpp>
#include <io/stringreader.hpp>
#include <iostream>
#include <utils/memory_tracking.hpp>

#include "tests/testrunner.hpp"
#include "tests/tests.hpp"

static bool strneq(const char* a, const char* b, std::size_t max_count) {
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
            std::cerr << std::format("Skipping unknown argument: {}\n", argv[i]);
        }
    }
    std::filesystem::create_directories("logs");

    Manganese::tests::TestRunner runner;

    if (lexer) {
        std::cout << ansi::PINK << "Lexer Tests\n" << ansi::RESET;
        Manganese::tests::runLexerTests(runner);
        std::cout << ("\n----------\n");
    }
    if (parser) {
        std::cout << ansi::PINK << "Parser Tests\n" << ansi::RESET;
        Manganese::tests::runParserTests(runner);
        std::cout << ("\n----------\n");
    }
    if (semantic) {
        std::cout << ansi::PINK << "Semantic Analyzer Tests\n" << ansi::RESET;
        Manganese::tests::runAnalyzerTests(runner);
        std::cout << ("\n----------\n");
    }
    if (codegen) {
        std::cout << ansi::PINK << "Codegen Tests\n" << ansi::RESET;
        // TODO: Add once codegen has progress
        std::cout << ("To be implemented.\n");
        std::cout << ("\n----------\n");
    }

    logTotalAllocatedMemory();  // Only does something if memory tracking is enabled
    runner.printSummary();
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << ansi::PINK << std::format("Elapsed Time: {:.3f} ms\n", (double)duration.count()) << ansi::RESET;

    return runner.allTestsPassed() ? 0 : 1;
}
