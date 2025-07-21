#include <frontend/ast.h>
#include <frontend/parser.h>
#include <frontend/semantic.h>
#include <global_macros.h>

#include <iostream>

#include "testrunner.h"

//NOTE: For now, these tests will "always pass" (i.e., there's no automatic checking of the semantic analyzer's ouput)
// The tests just call dump() on the resulting ast nodes
// TODO: Implement more automated checking of the output

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

void runSemanticAnalysisTests(TestRunner& runner) {
    runner.runTest("Analyze Literals", analyzeLiterals);
}

}  // namespace tests

}  // namespace Manganese