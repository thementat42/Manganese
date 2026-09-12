#include <core.hpp>
#include <frontend/parser.hpp>
#include <frontend/semantic/semantic_analyzer.hpp>
#include <frontend/cfg/control_flow_analyzer.hpp>
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

    cfg::ControlFlowAnalyzer cfa(parsedFiles, targetInfo);
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

    return analyzeControlFlow(invalid, false, __func__);
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

bool testEmptyReturnInNonVoid() {
    const std::string invalid = R"(
        func compute() -> int32 {
            return;
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

    runner.runTest("Unreachable Code Analysis", control_flow_tests::testUnreachableCode);
    runner.runTest("Missing Return Paths Analysis", control_flow_tests::testMissingReturnPaths);
    runner.runTest("Valid Branching Returns Analysis", control_flow_tests::testValidBranchingReturns);
    runner.runTest("Empty Return in Non-Void Function Analysis", control_flow_tests::testEmptyReturnInNonVoid);
}

}  // namespace Manganese::tests