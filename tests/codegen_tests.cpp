#include <core.hpp>
#include <filesystem>
#include <frontend/cfg/control_flow_analyzer.hpp>
#include <frontend/parser.hpp>
#include <frontend/semantic/semantic_analyzer.hpp>
#include <fstream>
#include <iostream>
#include <middleend/codegen/ir_generator.hpp>
#include <mnstl/chunk_allocator.hxx>
#include <string>
#include <utils/target_info.hpp>

#include "testrunner.hpp"
#include "tests.hpp"


namespace Manganese::tests {

namespace {

constexpr const char* logFileName = "logs/codegen_tests.log";
mnstl::chunk_allocator arena;
utils::TargetInfo targetInfo = utils::TargetInfo::fromHostTriple();

bool generateCode(const std::string& source, bool expectSuccess, std::string_view testName) {
    parser::Parser parser(source, lexer::Mode::String, arena);
    std::vector<parser::ParsedFile> parsedFiles = {parser.parse()};

    semantic::SemanticAnalyzer semanticAnalyzer(parsedFiles, targetInfo, arena);
    if (semanticAnalyzer.analyze() != Result::Success) { return !expectSuccess; }

    cfg::ControlFlowAnalyzer cfa(parsedFiles, targetInfo, semanticAnalyzer.getSymbolTable());
    if (cfa.analyze() != Result::Success) { return !expectSuccess; }

    bool irGenSucceeded = true;
    try {
        codegen::IRGenerator irGenerator("TestModule", parsedFiles, semanticAnalyzer, targetInfo);
        irGenerator.generate();
    } catch (...) { irGenSucceeded = false; }

    std::ofstream logFile(logFileName, std::ios::app);
    if (logFile) {
        logFile << "Test: " << testName << "\n";
        logFile << "Expected Result: " << (expectSuccess ? "Success" : "Failure") << "\n";
        logFile << "Actual Result: " << (irGenSucceeded && expectSuccess ? "Success" : "Failure") << '\n';
        logFile << "---------------------\n";
        logFile.close();
    }

    return expectSuccess ? irGenSucceeded : !irGenSucceeded;
}

}  // namespace

namespace codegen_tests {

namespace {

bool testBasicFunctionEmission() {
    const std::string source = R"(
        func add(a: int32, b: int32) -> int32 {
            return a + b;
        }
    )";
    return generateCode(source, true, __func__);
}

bool testVariableAllocationAndAssignment() {
    const std::string source = R"(
        func compute() -> int32 {
            let mut x: int32 = 10;
            x = x + 32;
            return x;
        }
    )";
    return generateCode(source, true, __func__);
}

bool testControlFlowLoweringIfElse() {
    const std::string source = R"(
        func max(a: int32, b: int32) -> int32 {
            if (a > b) {
                return a;
            } else {
                return b;
            }
        }
    )";
    return generateCode(source, true, __func__);
}

bool testWhileLoopLowering() {
    const std::string source = R"(
        func countdown(n: int32) -> int32 {
            let mut sum: int32 = 0;
            let mut i: int32 = n;
            while (i > 0) {
                sum = sum + i;
                i = i - 1;
            }
            return sum;
        }
    )";
    return generateCode(source, true, __func__);
}

bool testAggregateLayoutEmission() {
    const std::string source = R"(
        aggregate Point {
            x: int32;
            y: int32;
        }
        func origin() -> Point {
            let p: Point = Point { x = 0, y = 0 };
            return p;
        }
    )";
    return generateCode(source, true, __func__);
}

bool testPointerDereferenceCodegen() {
    const std::string source = R"(
        func increment(ptrVal: ptr mut int32) {
            *ptrVal = *ptrVal + 1;
        }
    )";
    return generateCode(source, true, __func__);
}

// bool testCodegenFromFile() {
//     const std::filesystem::path fullPath = std::filesystem::current_path() / "tests/codegen_tests.mn";
//     mnstl::chunk_allocator file_allocator{};

//     parser::Parser parser(fullPath.string(), lexer::Mode::File, file_allocator);
//     std::vector<parser::ParsedFile> parsedFiles = {parser.parse()};

//     semantic::SemanticAnalyzer semanticAnalyzer(parsedFiles, targetInfo, file_allocator);
//     if (semanticAnalyzer.analyze() != Result::Success) { return false; }

//     cfg::ControlFlowAnalyzer cfa(parsedFiles, targetInfo, semanticAnalyzer.getSymbolTable());
//     if (cfa.analyze() != Result::Success) { return false; }

//     try {
//         codegen::IRGenerator irGenerator("FileModule", parsedFiles, semanticAnalyzer, targetInfo);
//         irGenerator.generate();
//         return true;
//     } catch (...) { return false; }
// }

}  // namespace
}  // namespace codegen_tests

void runCodeGenerationTests(TestRunner& runner) {
    std::ofstream logFile(logFileName, std::ios::trunc);
    logFile.close();

    runner.runTest("Basic Function Emission", codegen_tests::testBasicFunctionEmission);
    runner.runTest("Variable Allocation and Assignment", codegen_tests::testVariableAllocationAndAssignment);
    runner.runTest("Control Flow Lowering (If/Else)", codegen_tests::testControlFlowLoweringIfElse);
    runner.runTest("While Loop Lowering", codegen_tests::testWhileLoopLowering);
    runner.runTest("Aggregate Layout Emission", codegen_tests::testAggregateLayoutEmission);
    runner.runTest("Pointer Dereference Codegen", codegen_tests::testPointerDereferenceCodegen);
    // runner.runTest("File Code Generation", codegen_tests::testCodegenFromFile);
}

}  // namespace Manganese::tests