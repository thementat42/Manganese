#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/Cloning.h>

#include <core.hpp>
#include <frontend/cfg/control_flow_analyzer.hpp>
#include <frontend/parser.hpp>
#include <frontend/semantic/semantic_analyzer.hpp>
#include <fstream>
#include <iostream>
#include <backend/ir_generator.hpp>
#include <backend/optimizations.hpp>
#include <string>
#include <utils/chunk_allocator.hpp>
#include <utils/string_interner.hpp>
#include <utils/target_info.hpp>

#include "tests.hpp"

namespace Manganese::tests {

namespace {

constexpr const char* executionLogFileName = "logs/execution_tests.log";

template <typename FuncType, typename ExpectedType>
bool validateIRExecutes(const std::string& source, const std::string& functionName, ExpectedType expectedValue,
                        std::string_view testName) {
    utils::chunk_allocator arena;
    utils::StringInterner interner;

    parser::Parser parser(source, lexer::Mode::String, arena, interner);
    std::vector<parser::ParsedFile> parsedFiles = {parser.parse()};

    semantic::SemanticAnalyzer semanticAnalyzer(parsedFiles, utils::getTargetInfo(), arena, interner);
    if (semanticAnalyzer.analyze() != Result::Success) { return false; }

    cfg::ControlFlowAnalyzer cfa(parsedFiles, utils::getTargetInfo(), semanticAnalyzer.getSymbolTable(), interner);
    if (cfa.analyze() != Result::Success) { return false; }

    try {
        codegen::IRGenerator irGenerator("ExecutionTestModule", parsedFiles, semanticAnalyzer, interner,
                                         utils::getTargetInfo());
        irGenerator.generate();

        auto* module = irGenerator.getModule();
        if (module == nullptr) { return false; }

        if (llvm::verifyModule(*module, &llvm::errs())) { return false; }

        // Run the optimization pipeline
        codegen::optimizeModule(module, llvm::OptimizationLevel::O2, utils::getTargetInfo().targetMachine.get());

        if (llvm::verifyModule(*module, &llvm::errs())) { return false; }

        // Clone the optimized module for JIT compilation
        auto clonedModule = llvm::CloneModule(*module);
        if (!clonedModule) { return false; }

        // Initialize LLVM ORC JIT
        auto jitOrErr = llvm::orc::LLJITBuilder().create();
        if (!jitOrErr) {
            llvm::consumeError(jitOrErr.takeError());
            return false;
        }
        auto jit = std::move(*jitOrErr);

        // Add module to JIT session
        auto err = jit->addIRModule(
            llvm::orc::ThreadSafeModule(std::move(clonedModule), std::make_unique<llvm::LLVMContext>()));
        if (err) {
            llvm::consumeError(std::move(err));
            return false;
        }

        // Look up the compiled function symbol
        auto symOrErr = jit->lookup(functionName);
        if (!symOrErr) {
            llvm::consumeError(symOrErr.takeError());
            return false;
        }

        // Cast to native function pointer and execute
        auto* funcPtr = symOrErr->toPtr<FuncType>();
        ExpectedType actualValue = funcPtr();

        bool success = (actualValue == expectedValue);

        // Log results (now logs expected and got regardless of pass/fail)
        std::ofstream logFile(executionLogFileName, std::ios::app);
        if (logFile) {
            logFile << "Execution Test: " << testName << "\n";
            logFile << "Result:   " << (success ? "Passed" : "Failed (Value mismatch)") << "\n";
            logFile << "  Expected: " << expectedValue << "\n";
            logFile << "  Got:      " << actualValue << "\n";
            logFile << "--------------------------------------------------\n";
            logFile.close();
        }

        return success;
    } catch (const std::exception& e) {
        std::ofstream logFile(executionLogFileName, std::ios::app);
        if (logFile) {
            logFile << "Execution Test: " << testName << " threw exception: " << e.what() << "\n";
            logFile << "--------------------------------------------------\n";
            logFile.close();
        }
        return false;
    } catch (...) { return false; }
}

}  // namespace

namespace execution_tests {
namespace {

bool testExecutionArithmeticAndLoops() {
    const std::string source = R"(
        func computeTotal(n: int32) -> int32 {
            let mut sum: int32 = 0;
            let mut i: int32 = 1;
            while (i <= n) {
                sum = sum + i;
                i = i + 1;
            }
            return sum;
        }
        func entryPoint() -> int32 {
            return computeTotal(5);
        }
    )";
    // sum(5) = 1 + 2 + 3 + 4 + 5 = 15
    return validateIRExecutes<int32_t (*)(), int32_t>(source, "entryPoint", 15, __func__);
}

bool testExecutionArrayAccess() {
    const std::string source = R"(
        func getElement() -> int32 {
            let arr: int32[3] = [10, 20, 30];
            return arr[1];
        }
    )";
    return validateIRExecutes<int32_t (*)(), int32_t>(source, "getElement", 20, __func__);
}

bool testExecutionGenericAggregateSlice() {
    const std::string source = R"(
        aggregate Pair[FirstType, SecondType] {
            first: FirstType;
            second: SecondType;
        }
        func getBoxVal() -> float64 {
            let pairs: Pair@[float64, string][] = [
                Pair@[float64, string]{first = 10.5, second = "ten and a half"}, 
                Pair@[float64, string]{first = 20.5, second = "twenty and a half"}
            ];
            return pairs[0].first;
        }
    )";
    return validateIRExecutes<double (*)(), double>(source, "getBoxVal", 10.5, __func__);
}

bool testExecutionSecondGenericAggregateSlice() {
    const std::string source = R"(
        aggregate Pair[FirstType, SecondType] {
            first: FirstType;
            second: SecondType;
        }
        func getBoxValSecond() -> float64 {
            let pairs: Pair@[float64, string][] = [
                Pair@[float64, string]{first = 10.5, second = "ten and a half"}, 
                Pair@[float64, string]{first = 20.5, second = "twenty and a half"}
            ];
            return pairs[1].first;
        }
    )";
    return validateIRExecutes<double (*)(), double>(source, "getBoxValSecond", 20.5, __func__);
}

}  // namespace
}  // namespace execution_tests

void runExecutionTests(TestRunner& runner) {
    std::ofstream logFile(executionLogFileName, std::ios::trunc);
    logFile.close();

    runner.runTest("Execution: Arithmetic & Loops", execution_tests::testExecutionArithmeticAndLoops);
    runner.runTest("Execution: Array Indexing", execution_tests::testExecutionArrayAccess);
    runner.runTest("Execution: Generic Aggregate Slice Index 0", execution_tests::testExecutionGenericAggregateSlice);
    runner.runTest("Execution: Generic Aggregate Slice Index 1",
                   execution_tests::testExecutionSecondGenericAggregateSlice);
}

}  // namespace Manganese::tests