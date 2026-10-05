#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

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

bool validateIRContains(const std::string& source, const std::initializer_list<std::string>& expectedSubstrings,
                        std::string_view testName) {
    parser::Parser parser(source, lexer::Mode::String, arena);
    std::vector<parser::ParsedFile> parsedFiles = {parser.parse()};

    semantic::SemanticAnalyzer semanticAnalyzer(parsedFiles, targetInfo, arena);
    if (semanticAnalyzer.analyze() != Result::Success) { return false; }

    cfg::ControlFlowAnalyzer cfa(parsedFiles, targetInfo, semanticAnalyzer.getSymbolTable());
    if (cfa.analyze() != Result::Success) { return false; }

    std::string irOutput;
    bool generationPassed = true;

    try {
        codegen::IRGenerator irGenerator("TestModule", parsedFiles, semanticAnalyzer, targetInfo);
        irGenerator.generate();

        if (auto* module = irGenerator.getModule()) {
            std::string tempStr;
            llvm::raw_string_ostream rso(tempStr);
            module->print(rso, nullptr);
            rso.flush();
            irOutput = tempStr;
        } else {
            generationPassed = false;
        }
    } catch (const std::exception& e) {
        generationPassed = false;
        irOutput = std::string("Exception: ") + e.what();
    } catch (...) {
        generationPassed = false;
        irOutput = "Unknown exception during IR generation.";
    }

    if (!generationPassed) {
        std::ofstream logFile(logFileName, std::ios::app);
        if (logFile) {
            logFile << "Test: " << testName << " failed during generation or module retrieval.\n";
            logFile << "---------------------\n";
        }
        std::cout << "Test: " << testName << " failed during generation or module retrieval.\n";
        return false;
    }

    bool allFound = true;
    std::vector<std::string> missingPatterns;
    for (const auto& pattern : expectedSubstrings) {
        if (irOutput.find(pattern) == std::string::npos) {
            allFound = false;
            missingPatterns.push_back(pattern);
        }
    }

    std::ofstream logFile(logFileName, std::ios::app);
    if (logFile) {
        logFile << "Test: " << testName << "\n";
        logFile << "Result: " << (allFound ? "Passed" : "Failed (Missing pattern)") << "\n";
        if (!allFound) {
            for (const auto& pattern : missingPatterns) { logFile << "Missing Substring: \"" << pattern << "\"\n"; }
        }
        logFile << "Generated IR\n" << irOutput << "\n---------------------\n";
        logFile.close();
    }

    if (!allFound) {
        std::cout << "Test: " << testName << "Failed. Missing patterns:\n";
        for (const auto& pattern : missingPatterns) { std::cout << "\tMissing Substring: \"" << pattern << "\"\n"; }
    }

    return allFound;
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
    return validateIRContains(source, {"define", "add"}, __func__);
}

bool testVariableAllocationAndAssignment() {
    const std::string source = R"(
        func compute() -> int32 {
            let mut x: int32 = 10;
            x = x + 32;
            return x;
        }
    )";
    return validateIRContains(source, {"compute", "alloca"}, __func__);
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
    return validateIRContains(source, {"max", "br"}, __func__);
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
    return validateIRContains(source, {"countdown", "br"}, __func__);
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
    return validateIRContains(source, {"origin", "Point"}, __func__);
}

bool testPointerDereferenceCodegen() {
    const std::string source = R"(
        func increment(ptrVal: ptr mut int32) {
            *ptrVal = *ptrVal + 1;
        }
    )";
    return validateIRContains(source, {"increment", "load", "store"}, __func__);
}

bool testBooleanLogic() {
    const std::string source = R"(
        func check(a: bool, b: bool) -> bool {
            return (a && b) || !a;
        }
    )";
    return validateIRContains(source, {"check", "and", "or"}, __func__);
}

bool testBinaryOperatorPrecedence() {
    const std::string source = R"(
        func evaluate() -> int32 {
            let x = 3;
            let y = 4;
            return 2 + x * 4 - 8 / y;
        }
    )";
    return validateIRContains(source, {"evaluate", "mul", "add", "sub", "fdiv"}, __func__);
}

bool testNestedFunctionCalls() {
    const std::string source = R"(
        func square(x: int32) -> int32 {
            return x * x;
        }
        func computeNested(a: int32, b: int32) -> int32 {
            return square(square(a) + square(b));
        }
    )";
    return validateIRContains(source, {"computeNested", "square", "call"}, __func__);
}

bool testArrayIndexBasedAccess() {
    const std::string source = R"(
        func getElement() -> int32 {
            let arr: int32[3] = [10, 20, 30];
            return arr[1];
        }
    )";
    return validateIRContains(source, {"getElement", "getelementptr"}, __func__);
}

bool testGenericsCodegen() {
    const std::string source = R"(
        func identity[T](x: T) -> T {
            return x;
        }
        func run() -> int32 {
            identity@[string]("foo");
            identity@[bool](true);
            return identity@[int32](42);
        }
    )";
    return validateIRContains(source, {"run", "identity"}, __func__);
}

bool testArrayOfFunctionsIndexedAndCalled() {
    const std::string source = R"(
        func f1(a: int32, b: int32) -> int32 { return a + b; }
        func f2(a: int32, b: int32) -> int32 { return a - b; }
        
        func dispatch(y: int32) -> int32 {
            let funcs = [f1, f2];
            return funcs[y](10, 5);
        }
    )";
    return validateIRContains(source, {"dispatch", "call"}, __func__);
}

bool testArrayOfInstantiatedGenericFunctions() {
    const std::string source = R"(
        func foo[T](a: int32, b: int32) -> int32 { return a + b; }
        func blah[T](a: int32, b: int32) -> int32 { return a * b; }

        func testGenericArray(y: int32) -> int32 {
            let x = [foo@[int32], blah@[int32]];
            return x[y](1, 2);
        }
    )";
    return validateIRContains(source, {"testGenericArray", "call"}, __func__);
}

bool testGenericAggregateInstantiation() {
    const std::string source = R"(
        aggregate Wrapper[T] {
            inner: T;
        }
        func makeWrapper() -> int32 {
            let w = Wrapper@[int32] { inner = 100 };
            return w.inner;
        }
    )";
    return validateIRContains(source, {"makeWrapper", "Wrapper"}, __func__);
}

bool testMultiParamGenerics() {
    const std::string source = R"(
        func selectFirst[T, U](a: T, b: U) -> T {
            return a;
        }
        func testSelect() -> int32 {
            return selectFirst@[int32, bool](99, true);
        }
    )";
    return validateIRContains(source, {"selectFirst", "testSelect"}, __func__);
}

bool testArrayOfGenericAggregates() {
    const std::string source = R"(
        aggregate Pair[FirstType, SecondType] {
            first: FirstType;
            second: SecondType;
        }
        func getBoxVal() -> float64 {
            let pairs: Pair@[float64, string][] = [Pair@[float64, string]{first = 10.5, second = "ten and a half"}, Pair@[float64, string]{first = 20.5, second = "twenty and a half"}];
            return pairs[0].first;
        }
    )";
    return validateIRContains(source, {"getBoxVal", "Pair"}, __func__);
}

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

    runner.runTest("Boolean Logic Codegen", codegen_tests::testBooleanLogic);
    runner.runTest("Binary Operator Precedence", codegen_tests::testBinaryOperatorPrecedence);
    runner.runTest("Nested Function Calls", codegen_tests::testNestedFunctionCalls);
    runner.runTest("Array Index-Based Access", codegen_tests::testArrayIndexBasedAccess);
    runner.runTest("Generics Codegen", codegen_tests::testGenericsCodegen);
    runner.runTest("Array of Functions Indexed and Called", codegen_tests::testArrayOfFunctionsIndexedAndCalled);
    runner.runTest("Array of Instantiated Generic Functions", codegen_tests::testArrayOfInstantiatedGenericFunctions);

    runner.runTest("Generic Aggregate Instantiation", codegen_tests::testGenericAggregateInstantiation);
    runner.runTest("Multi-Parameter Generic Functions", codegen_tests::testMultiParamGenerics);
    runner.runTest("Array of Generic Aggregates", codegen_tests::testArrayOfGenericAggregates);
}

}  // namespace Manganese::tests