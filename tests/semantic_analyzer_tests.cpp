/**
 * @file semantic_analyzer_tests.cpp
 * @brief Unit tests for the semantic_analyzer.
 *
 * @see include/frontend/semantic_analyzer.h
 * @see testrunner.h
 */

#include <frontend/ast.h>
#include <frontend/parser.h>
#include <frontend/semantic.h>
#include <global_macros.h>

#include <iostream>

#include "testrunner.h"

//  NOTE: For now, these tests will "always pass" (i.e., there's no automatic checking of the semantic analyzer's ouput)
//  The tests just call dump() on the resulting ast nodes
//  TODO: Implement more automated checking of the output

namespace Manganese {

namespace tests {

parser::ParsedFile parse(const std::string& source, lexer::Mode mode = lexer::Mode::String) {
    parser::Parser parser(source, mode);
    return parser.parse();
}

bool analyzeLiterals() {
    semantic::SemanticAnalyzer analyzer;
    parser::ParsedFile file = parse("true; \"asdf\"; [1i64, 2, 3, 4, true]; 100; 'a';");
    analyzer.analyze(file);
    const auto& program = file.program;
    if (program.size() != 5) {
        std::cerr << "Expected 5 statements, got " << program.size() << "\n";
        return false;
    }
    for (size_t i = 0; i < 5; ++i) {
        program[i]->dump(std::cerr);
    }

    return true;
}

bool analyzeSimpleVariableDeclaration() {
    semantic::SemanticAnalyzer analyzer;
    parser::ParsedFile file = parse(
        "let x: int64 = 10; let x: int64 = 20;"
        "let y: string = \"hello\";"
        "const z = x; z = 3;"
        "let a = [[1, 2], [3, 4]]; let b = a[0]; let c = b[1];");
    analyzer.analyze(file);
    const auto& program = file.program;
    if (program.size() != 8) {
        std::cerr << "Expected 8 statements, got " << program.size() << "\n";
        return false;
    }
    for (size_t i = 0; i < 8; ++i) {
        // std::cout << program[i]->toString() << "\n";
        program[i]->dump(std::cerr);
    }
    return true;
}

void runSemanticAnalysisTests(TestRunner& runner) {
    runner.runTest("Analyze Literals", analyzeLiterals);
    runner.runTest("Analyze Simple Variable Declaration", analyzeSimpleVariableDeclaration);
}

}  // namespace tests

}  // namespace Manganese