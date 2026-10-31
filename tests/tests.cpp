#include "tests.hpp"

#include <backend/object_code_emitter.hpp>
#include <chrono>
#include <filesystem>
#include <format>
#include <io/logging.hpp>
#include <iostream>
#include <string>

namespace Manganese::tests {
namespace {
struct Phase {
    std::string_view name;
    bool enabled;
    void (TestRunner::*run)();
};
}  // namespace

Result runTestSuite(int argc, const char** argv) {
    bool lexer = false;
    bool parser = false;
    bool semantic = false;
    bool controlflow = false;
    bool codegen = false;
    bool execution = false;
    bool objectcode = false;
    bool multifile = false;

    if (argc == 1) {
        lexer = parser = semantic = controlflow = codegen = execution = objectcode = multifile = true;
    } else {
        for (int i = 1; i < argc; ++i) {
            std::string_view arg{argv[i]};
            if (arg == "--lexer") {
                lexer = true;
            } else if (arg == "--parser") {
                parser = true;
            } else if (arg == "--semantic") {
                semantic = true;
            } else if (arg == "--codegen") {
                codegen = true;
            } else if (arg == "--controlflow") {
                controlflow = true;
            } else if (arg == "--execution") {
                execution = true;
            } else if (arg == "--objectcode") {
                objectcode = true;
            } else if (arg == "--multifile") {
                multifile = true;
            } else if (arg == "--all") {
                lexer = parser = semantic = controlflow = codegen = execution = objectcode = multifile = true;
                break;
            } else {
                std::cout << std::format("Skipping unknown argument: {}\n", argv[i]);
            }
        }
    }

    auto start = std::chrono::high_resolution_clock::now();

    backend::initializeLLVM();
    std::filesystem::create_directories("logs");

    tests::TestRunner runner;

    std::array suites{
        Phase{.name = "Lexer Tests", .enabled = lexer, .run = &TestRunner::runLexerTests},
        Phase{.name = "Parser Tests", .enabled = parser, .run = &TestRunner::runParserTests},
        Phase{.name = "Semantic Analyzer Tests", .enabled = semantic, .run = &TestRunner::runSemanticAnalyzerTests},
        Phase{.name = "Control Flow Tests", .enabled = controlflow, .run = &TestRunner::runControlFlowAnalyzerTests},
        Phase{.name = "Codegen Tests", .enabled = codegen, .run = &TestRunner::runCodeGenerationTests},
        Phase{.name = "Execution Tests", .enabled = execution, .run = &TestRunner::runExecutionTests},
        Phase{.name = "Object Code Tests", .enabled = objectcode, .run = &TestRunner::runObjectCodeTests},
        Phase{.name = "Multi-File Tests", .enabled = multifile, .run = &TestRunner::runMultiFileTests},
    };

    for (const auto& suite : suites) {
        if (!suite.enabled) { continue; }
        std::cout << ansi::PINK << suite.name << "\n" << ansi::RESET;
        (runner.*suite.run)();
        std::cout << "\n----------\n";
    }

    runner.printSummary();
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << ansi::PINK << std::format("Elapsed Time: {:.3f} ms\n", static_cast<double>(duration.count()))
              << ansi::RESET;

    return runner.allTestsPassed() ? Result::Success : Result::Failure;
}

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

}  // namespace Manganese::tests
