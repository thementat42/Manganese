#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>

#include <backend/ir_generator.hpp>
#include <backend/object_code_emitter.hpp>
#include <backend/optimizations.hpp>
#include <core.hpp>
#include <frontend/cfg/control_flow_analyzer.hpp>
#include <frontend/parser.hpp>
#include <frontend/semantic/semantic_analyzer.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include <utils/chunk_allocator.hpp>
#include <utils/string_interner.hpp>
#include <utils/target_info.hpp>

#include "tests.hpp"

namespace Manganese::tests {

namespace {

constexpr const char* objectCodeLogFileName = "logs/object_code_tests.log";

bool validateObjectCodeEmits(const std::string& source, std::string_view testName) {
    utils::chunk_allocator arena;
    utils::StringInterner interner;

    parser::Parser parser(source, lexer::Mode::String, arena, interner);
    std::vector<parser::ParsedFile> parsedFiles = {parser.parse()};

    semantic::SemanticAnalyzer semanticAnalyzer(parsedFiles, utils::getTargetInfo(), arena, interner);
    if (semanticAnalyzer.analyze() != Result::Success) { return false; }

    cfg::ControlFlowAnalyzer cfa(parsedFiles, utils::getTargetInfo(), semanticAnalyzer.getSymbolTable(), interner);
    if (cfa.analyze() != Result::Success) { return false; }

    std::string filename = "test_output_" + std::string(testName) + ".o";

    try {
        codegen::IRGenerator irGenerator("ObjectCodeTestModule", parsedFiles, semanticAnalyzer, interner,
                                         utils::getTargetInfo());
        irGenerator.generate();

        auto* module = irGenerator.getModule();
        if (module == nullptr) { return false; }

        if (llvm::verifyModule(*module, &llvm::errs())) { return false; }

        codegen::optimizeModule(module, llvm::OptimizationLevel::O2, utils::getTargetInfo().targetMachine.get());

        if (llvm::verifyModule(*module, &llvm::errs())) { return false; }

        // Emit object code
        Result result = backend::emitObjectCode(module, utils::getTargetInfo(), filename);
        if (result != Result::Success) {
            std::remove(filename.c_str());
            return false;
        }

        // Verify file exists and has content (size > 0)
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        bool fileExistsAndNotEmpty = file.is_open() && (file.tellg() > 0);
        file.close();

        std::remove(filename.c_str());

        std::ofstream logFile(objectCodeLogFileName, std::ios::app);
        if (logFile) {
            logFile << "Object Code Test: " << testName << "\n";
            logFile << "Result: " << (fileExistsAndNotEmpty ? "Passed" : "Failed (File empty or missing)") << "\n";
            logFile << "--------------------------------------------------\n";
            logFile.close();
        }

        return fileExistsAndNotEmpty;
    } catch (const std::exception& e) {
        std::remove(filename.c_str());
        std::ofstream logFile(objectCodeLogFileName, std::ios::app);
        if (logFile) {
            logFile << "Object Code Test: " << testName << " threw exception: " << e.what() << "\n";
            logFile << "--------------------------------------------------\n";
            logFile.close();
        }
        return false;
    } catch (...) {
        std::remove(filename.c_str());
        return false;
    }
}

}  // namespace

namespace object_code_tests {
namespace {

bool testEmitSimpleObjectCode() {
    const std::string source = R"(
        func add(a: int32, b: int32) -> int32 {
            return a + b;
        }
        func entryPoint() -> int32 {
            return add(40, 2);
        }
    )";
    return validateObjectCodeEmits(source, __func__);
}

bool testEmitArrayObjectCode() {
    const std::string source = R"(
        func getElement() -> int32 {
            let arr: int32[3] = [5, 10, 15];
            return arr[2];
        }
    )";
    return validateObjectCodeEmits(source, __func__);
}

}  // namespace
}  // namespace object_code_tests

void TestRunner::runObjectCodeTests() {
    std::ofstream logFile(objectCodeLogFileName, std::ios::trunc);
    logFile.close();

    runTest("Object Code: Simple Arithmetic Function", object_code_tests::testEmitSimpleObjectCode);
    runTest("Object Code: Array Access Function", object_code_tests::testEmitArrayObjectCode);
}

}  // namespace Manganese::tests