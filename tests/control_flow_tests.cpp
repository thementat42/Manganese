#include <core.hpp>
#include <filesystem>
#include <frontend/cfg/control_flow_analyzer.hpp>
#include <frontend/parser.hpp>
#include <frontend/semantic/semantic_analyzer.hpp>
#include <fstream>
#include <iostream>
#include <mnstl/chunk_allocator.hxx>
#include <string>
#include <utils/target_info.hpp>

#include "testrunner.hpp"
#include "tests.hpp"

namespace Manganese::tests {

namespace {

constexpr const char* logFileName = "logs/control_flow_tests.log";
mnstl::chunk_allocator arena;
utils::TargetInfo targetInfo = utils::TargetInfo::fromHostTriple();

bool analyzeControlFlow(const std::string& source, bool expectSuccess, std::string_view testName) {
    parser::Parser parser(source, lexer::Mode::String, arena);
    std::vector<parser::ParsedFile> parsedFiles = {parser.parse()};

    semantic::SemanticAnalyzer semanticAnalyzer(parsedFiles, targetInfo, arena);
    DISCARD(semanticAnalyzer.analyze());

    cfg::ControlFlowAnalyzer cfa(parsedFiles, targetInfo, semanticAnalyzer.getSymbolTable());
    Result result = cfa.analyze();

    std::ofstream logFile(logFileName, std::ios::app);
    if (logFile) {
        logFile << "Test: " << testName << "\n";
        logFile << "Expected Result: " << (expectSuccess ? "Valid" : "Invalid") << "\n";
        logFile << "Actual Result: " << (result == Result::Success ? "Valid" : "Invalid") << '\n';
        logFile << "---------------------\n";
        logFile.close();
    }

    return expectSuccess ? result == Result::Success : result == Result::Failure;
}

}  // namespace

namespace control_flow_tests {

namespace {

bool testUnreachableCode() {
    const std::string invalid = R"(
        func foo() -> int32 {
            return 42;
            let x: int32 = 10;
        }
    )";

    return analyzeControlFlow(invalid, true, __func__);
}

bool testMissingReturnPaths() {
    const std::string invalid = R"(
        func getValue() -> int32 {
            let x = 5;
        }
    )";

    return analyzeControlFlow(invalid, false, __func__);
}

bool testValidBranchingReturns() {
    const std::string valid = R"(
        func getCode() -> int32 {
            if (true) {
                return 1;
            } else {
                return 0;
            }
        }
    )";

    return analyzeControlFlow(valid, true, __func__);
}

bool testPartialReturnBranching() {
    const std::string invalid = R"(
        func checkPositive(x: int32) -> int32 {
            if (x > 0) { return 1; }
            let y = 0;
        }
    )";

    return analyzeControlFlow(invalid, false, __func__);
}

bool testControlFlowFromFile() {
    const std::filesystem::path fullPath = std::filesystem::current_path() / "tests/control_flow_tests.mn";
    mnstl::chunk_allocator file_allocator{};

    parser::Parser parser(fullPath.string(), lexer::Mode::File, file_allocator);
    std::vector<parser::ParsedFile> parsedFiles = {parser.parse()};

    semantic::SemanticAnalyzer semanticAnalyzer(parsedFiles, targetInfo, file_allocator);
    (void)semanticAnalyzer.analyze();

    cfg::ControlFlowAnalyzer cfa(parsedFiles, targetInfo, semanticAnalyzer.getSymbolTable());
    Result result = cfa.analyze();

    return result == Result::Success;
}

bool testUninitializedRead() {
    const std::string invalid = R"(
        func foo() -> int32 {
            let x: int32;
            return x;
        }
    )";
    return analyzeControlFlow(invalid, false, __func__);
}

bool testConditionalInitializationFailure() {
    const std::string invalid = R"(
        func foo(cond: bool) -> int32 {
            let x: int32;
            if (cond) {
                x = 10;
            }
            return x; // Fails: x is MaybeInitialized
        }
    )";
    return analyzeControlFlow(invalid, false, __func__);
}

bool testConditionalInitializationSuccess() {
    const std::string valid = R"(
        func foo(cond: bool) -> int32 {
            let x: int32;
            if (cond) {
                x = 10;
            } else {
                x = 20;
            }
            return x; // Succeeds: x is fully Initialized on all paths
        }
    )";
    return analyzeControlFlow(valid, true, __func__);
}

bool testLoopAssignmentSafety() {
    const std::string invalid = R"(
        func foo(n: int32) -> int32 {
            let x: int32;
            while (n > 0) {
                x = 5;
                n = n - 1;
            }
            return x; // Fails: runtime loop might execute 0 times
        }
    )";
    return analyzeControlFlow(invalid, false, __func__);
}

}  // namespace
}  // namespace control_flow_tests

void runControlFlowAnalyzerTests(TestRunner& runner) {
    // clear the log file
    std::ofstream logFile(logFileName, std::ios::trunc);
    logFile.close();

    runner.runTest("Unreachable Code Control Flow Analysis", control_flow_tests::testUnreachableCode);
    runner.runTest("Missing Return Paths Control Flow Analysis", control_flow_tests::testMissingReturnPaths);
    runner.runTest("Valid Branching Returns Control Flow Analysis", control_flow_tests::testValidBranchingReturns);
    runner.runTest("Partial Return Branching Control Flow Analysis", control_flow_tests::testPartialReturnBranching);
    runner.runTest("File Control Flow Analysis", control_flow_tests::testControlFlowFromFile);

    runner.runTest("Uninitialized Variable Read", control_flow_tests::testUninitializedRead);
    runner.runTest("Conditional Initialization Failure", control_flow_tests::testConditionalInitializationFailure);
    runner.runTest("Conditional Initialization Success", control_flow_tests::testConditionalInitializationSuccess);
    runner.runTest("Runtime Loop Assignment Safety", control_flow_tests::testLoopAssignmentSafety);
}

}  // namespace Manganese::tests