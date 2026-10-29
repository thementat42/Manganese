#include "tests.hpp"

#include <backend/object_code_emitter.hpp>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <format>
#include <io/logging.hpp>
#include <iostream>
#include <string>

namespace Manganese::tests {
namespace {
bool strneq(const char* a, const char* b, std::size_t max_count) {
    return (strncmp(a, b, max_count) == 0) && (strlen(a) == strlen(b));
}

}  // namespace

void TestRunner::runTest(const std::string& testName, bool (*testFunction)()) {
    std::cout << "Running test: " << testName << "...\n";

    const bool result = testFunction();
    std::cout << (result ? ansi::GREEN : ansi::RED)
              << std::format("Test '{}' {}", testName, (result ? "PASSED" : "FAILED")) << ansi::RESET << "\n";

    if (result) {
        ++passed;
        passedTests += testName + '\n';
    } else {
        ++failed;
        failedTests += testName + '\n';
    }
}

void TestRunner::printSummary() const noexcept {
    int total = passed + failed;
    total = total == 0 ? 1 : total;  // avoid any division by 0 problems
    std::cout << ansi::PINK << "\nTest Results:" << ansi::RESET << '\n';

    if (passed != 0) {
        std::cout << ansi::GREEN << std::format("Tests Passed ({}):\n", passed) << passedTests << ansi::RESET << "\n";
    }

    if (failed != 0) {
        std::cout << ansi::RED << std::format("Tests Failed ({}):\n", failed) << failedTests << ansi::RESET << "\n";
    }

    const float percentPassed = static_cast<float>(passed) / static_cast<float>(total) * 100.0F;
    const float percentFailed = static_cast<float>(failed) / static_cast<float>(total) * 100.0F;

    std::cout << "\n----------\n" << ansi::PINK << "Test Summary: " << ansi::RESET << '\n';
    std::cout << ansi::GREEN << std::format("Passed: {}/{} ({:.2f}%)\n", passed, total, percentPassed) << ansi::RESET;
    std::cout << ansi::RED << std::format("Failed: {}/{} ({:.2f}%)\n", failed, total, percentFailed) << ansi::RESET;
    std::cout << ansi::PINK << "Total: " << total << ansi::RESET << '\n';
    if (failed == 0) { std::cout << ansi::GREEN << "All tests passed!" << ansi::RESET << '\n'; }
}

Result runTestSuite(int argc, const char** argv) {
    if (argc == 1) {
        std::cout << std::format(
            "Usage: {} [--lexer] [--parser] [--semantic] [--controlflow] [--codegen] [--execution] [--all]\n", argv[0]);
        return Result::Failure;
    }

    auto start = std::chrono::high_resolution_clock::now();

    bool lexer = false;
    bool parser = false;
    bool semantic = false;
    bool controlflow = false;
    bool codegen = false;
    bool execution = false;

    backend::initializeLLVM();

    for (int i = 1; i < argc; ++i) {
        if (strneq(argv[i], "--lexer", 7)) {
            lexer = true;
        } else if (strneq(argv[i], "--parser", 8)) {
            parser = true;
        } else if (strneq(argv[i], "--semantic", 10)) {
            semantic = true;
        } else if (strneq(argv[i], "--codegen", 9)) {
            codegen = true;
        } else if (strneq(argv[i], "--controlflow", 13)) {
            controlflow = true;
        } else if (strneq(argv[i], "--execution", 13)) {
            execution = true;
        } else if (strneq(argv[i], "--all", 5)) {
            lexer = true;
            parser = true;
            semantic = true;
            controlflow = true;
            codegen = true;
            execution = true;
            break;
        } else {
            std::cout << std::format("Skipping unknown argument: {}\n", argv[i]);
        }
    }
    std::filesystem::create_directories("logs");

    tests::TestRunner runner;

    if (lexer) {
        std::cout << ansi::PINK << "Lexer Tests\n" << ansi::RESET;
        tests::runLexerTests(runner);
        std::cout << "\n----------\n";
    }
    if (parser) {
        std::cout << ansi::PINK << "Parser Tests\n" << ansi::RESET;
        tests::runParserTests(runner);
        std::cout << "\n----------\n";
    }
    if (semantic) {
        std::cout << ansi::PINK << "Semantic Analyzer Tests\n" << ansi::RESET;
        tests::runSemanticAnalyzerTests(runner);
        std::cout << "\n----------\n";
    }
    if (controlflow) {
        std::cout << ansi::PINK << "Control Flow Analysis Tests\n" << ansi::RESET;
        tests::runControlFlowAnalyzerTests(runner);
        std::cout << "\n----------\n";
    }
    if (codegen) {
        std::cout << ansi::PINK << "Codegen Tests\n" << ansi::RESET;
        tests::runCodeGenerationTests(runner);
        std::cout << "\n----------\n";
    }
    if (execution) {
        std::cout << ansi::PINK << "Execution Tests\n" << ansi::RESET;
        tests::runExecutionTests(runner);
        std::cout << "\n----------\n";
    }

    runner.printSummary();
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << ansi::PINK << std::format("Elapsed Time: {:.3f} ms\n", static_cast<double>(duration.count()))
              << ansi::RESET;

    return runner.allTestsPassed() ? Result::Success : Result::Failure;
}

}  // namespace Manganese::tests
